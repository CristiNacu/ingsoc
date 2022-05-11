import sys
from matplotlib import pyplot as plt
from PacketInterpreter.PacketInterpreter import PacketInterpretor
from PacketInterpreter.InternalPacketDefinitions import PACKET_ID_TO_STRING
import json

def parser(bytes, interpretor : PacketInterpretor):
    for byte in bytes:
        interpretor.process_byte(byte)
    
    # print(json.dumps(interpretor.get_succession(), indent = 4))
    packets = {}
    for el in interpretor.get_succession():
        # print(str(el))
        if PACKET_ID_TO_STRING[el.pkacet_id] in packets.keys():
            packets[PACKET_ID_TO_STRING[el.pkacet_id]] += 1
        else:
            packets[PACKET_ID_TO_STRING[el.pkacet_id]] = 1

    print(json.dumps(packets, indent=4))    
    plt.xticks(rotation=45)
    plt.bar(packets.keys(), packets.values())
    plt.show()



def read_trace(file_path: str):
    interpretor = PacketInterpretor()

    data = b""

    with open(file_path, "rb") as f:
        data = f.read()
    
    parser(data, interpretor)

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