import sys
from matplotlib import pyplot as plt
from sympy import sequence
from PacketInterpreter.PacketInterpreter import PacketInterpretor
from PacketInterpreter.InternalPacketDefinitions import PACKET_ID_TO_STRING
import json
from kafka import KafkaConsumer
from struct import *
import numpy as np
from sklearn.neighbors.kde import KernelDensity

from PacketInterpreter.IntelPacketDefinitions import *

PACKET_HEADER_STRUCT_CONTROL_SUM = "=I" # unsigned HeaderSize

PACKET_HEADER_STRUCT = ("=I"  # unsigned HeaderSize                          4
                        "Q"  # unsigned long long PacketId                  8
                        "I"  # unsigned CpuId                               4
                        "I"  # unsigned SequenceId                          4
                        "H") # struct PacketInformation                     2
                             # {
                             #     short FirstPacket : 1;
                             #     short LastPacket : 1;
                             #     short ExecutableTrace : 1;
                             # } Options;

PACKET_PAYLOAD_STRUCT_FIRST = ( "=Q"     # PVOID ImageBaseAddress
                                "L"     # ULONG ImageSize
                                "Q")    # ULONGLONG ProcessorFrequency

PACKET_HEADER_STRUCT_SIZE               = calcsize(PACKET_HEADER_STRUCT)
PACKET_HEADER_STRUCT_CONTROL_SUM_SIZE   = calcsize(PACKET_HEADER_STRUCT_CONTROL_SUM)

PACKET_FIRST_MASK   = 0b001
PACKET_LAST_MASK    = 0b010

SEQUENCE_INTERPRETERS = {}
PACKET_ORDER_LISTS = {}

def parser(bytes, interpretor : PacketInterpretor, sequence_id: int , packet_id: int, cpu_id: int):
    if sequence_id not in PACKET_ORDER_LISTS.keys():
        PACKET_ORDER_LISTS[sequence_id] = {"last_packet_id": 0, "out_of_order_packets" : []}

    if packet_id != PACKET_ORDER_LISTS[sequence_id]["last_packet_id"] + 1:
        print(f"[INFO] Out of order packet received. Las packet ID {PACKET_ORDER_LISTS[sequence_id]['last_packet_id']}. Current packet ID {packet_id}. Caching packet...")
        PACKET_ORDER_LISTS[sequence_id]["out_of_order_packets"].append((bytes, packet_id, cpu_id))
        PACKET_ORDER_LISTS[sequence_id]["out_of_order_packets"].sort(key = lambda x: x[1])


    for byte in bytes:
        interpretor.process_byte(byte)

    PACKET_ORDER_LISTS[sequence_id]["last_packet_id"] = packet_id
    while   PACKET_ORDER_LISTS[sequence_id]["out_of_order_packets"] is not None and\
            len(PACKET_ORDER_LISTS[sequence_id]["out_of_order_packets"]) >= 1 and\
            PACKET_ORDER_LISTS[sequence_id]["out_of_order_packets"][0] == PACKET_ORDER_LISTS[sequence_id]["last_packet_id"] + 1:
        
        bytes = PACKET_ORDER_LISTS[sequence_id]["out_of_order_packets"][0][0]
        for byte in bytes:
            interpretor.process_byte(byte)
        
        PACKET_ORDER_LISTS[sequence_id]["last_packet_id"] = PACKET_ORDER_LISTS[sequence_id]["out_of_order_packets"][0][1]
        PACKET_ORDER_LISTS[sequence_id]["out_of_order_packets"] = PACKET_ORDER_LISTS[sequence_id]["out_of_order_packets"][1:]



def read_trace(file_path: str):
    interpretor = PacketInterpretor()
    gCrtPlt = None
    base_image_address = 0

    # data = b""

    # with open(file_path, "rzb") as f:
    #     data = f.read()
    
    consumer = KafkaConsumer("ana_are_mere", bootstrap_servers = "localhost:9092", auto_offset_reset = "earliest")
    for msg in consumer:
        
        msg = msg.value

        # Check the message control size against the defined header size
        control_sum_bytes = msg[:PACKET_HEADER_STRUCT_CONTROL_SUM_SIZE]
        control_sum = unpack(PACKET_HEADER_STRUCT_CONTROL_SUM, control_sum_bytes)[0]
        if control_sum != PACKET_HEADER_STRUCT_SIZE:
            print(f"[ERROR] Message header has size {control_sum}. Declared message header should have {PACKET_HEADER_STRUCT_SIZE}. The client/server structures are not in sync!")
            continue
    
        header_bytes = msg[:PACKET_HEADER_STRUCT_SIZE]
        header_values = unpack(PACKET_HEADER_STRUCT, header_bytes)
        
        # header_size = header_values[0]
        packet_id = header_values[1]
        cpu_id = header_values[2]
        sequence_id = header_values[3]
        options = header_values[4] 

        packet_type = "FIRST" if (options & PACKET_FIRST_MASK) != 0 else ("LAST" if (options & PACKET_LAST_MASK) != 0 else "DATA")

        if packet_type == "FIRST":
            
            first_packet_values = unpack(PACKET_PAYLOAD_STRUCT_FIRST, msg[PACKET_HEADER_STRUCT_SIZE:])
            
            image_base_address  = first_packet_values[0]
            image_size          = first_packet_values[1]
            processor_frequency = first_packet_values[2]

            if sequence_id in SEQUENCE_INTERPRETERS.keys():
                print(f"[WARNING] An interpreter for sequence ID {sequence_id} already exists. Overwriting...")
            SEQUENCE_INTERPRETERS[sequence_id] = PacketInterpretor(processor_frequency, image_base_address, image_size)

            print(f"Image base address {image_base_address} image size {image_size} processor frequency {processor_frequency}")
        else:
            parser(msg[PACKET_HEADER_STRUCT_SIZE:], interpretor, sequence_id, packet_id, cpu_id)

        # print(json.dumps(interpretor.get_succession(), indent = 4))
        packets = {}
        ips = []
        first_tsc = -1
        last_tsc = -1
        packets_by_time = {}

        for el in interpretor.get_succession():
            # print(str(el))
            if el.packet_id not in [None]:
                if PACKET_ID_TO_STRING[el.packet_id] in packets.keys():
                    packets[PACKET_ID_TO_STRING[el.packet_id]] += 1
                else:
                    packets[PACKET_ID_TO_STRING[el.packet_id]] = 1

            if el.packet_id in [PACKET_FUP, PACKET_TIP, PACKET_TIP_PGD, PACKET_TIP_PGE]:
                addr = el.address
                if addr is None:
                    continue
                ips.append(abs(addr - base_image_address))

            if el.packet_id is PACKET_TSC:
                if first_tsc == -1:
                    first_tsc = el.tsc
                last_tsc = el.tsc

            if el.tsc not in packets_by_time.keys():
                packets_by_time[el.tsc] = {}
            
            if PACKET_ID_TO_STRING[el.packet_id] not in packets_by_time[el.tsc].keys():
                packets_by_time[el.tsc][PACKET_ID_TO_STRING[el.packet_id]] = 0

            packets_by_time[el.tsc][PACKET_ID_TO_STRING[el.packet_id]] += 1




        if gCrtPlt is not None:
            plt.close(gCrtPlt)
        
        print(json.dumps(packets, indent=4))  
        print(json.dumps(packets_by_time, indent=4))  
        print(f"TRACED TIME {(last_tsc - first_tsc) / processor_frequency} SECONDS")

        plt.xticks(rotation=45)

        gCrtPlt, axis = plt.subplots(1, 2)
        gCrtPlt.set_size_inches(15.5, 9.5, forward=True)
        
        # For Sine Function
        axis[0].bar(packets.keys(), packets.values())
        axis[0].set_title("Packet distribution")

        if ips != []:
            x = np.array(ips).reshape(-1, 1)
            y = np.zeros_like(ips)
            
            axis[1].scatter(x, y)
            axis[1].set_title("Address distribution")
            plt.pause(1)

        if packet_type == "LAST":
            pass


def print_help():
    print("TraceDecoder.py <trace_file_path>")

def main():

    print(f"arg count: {len(sys.argv)}")

    if len(sys.argv) < 2:
        print_help()
        return
    
    print(f"file path: {sys.argv[1]}")
 
    read_trace(sys.argv[1])

    return


if __name__ == "__main__":
    main()