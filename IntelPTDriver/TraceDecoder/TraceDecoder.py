import sys
from matplotlib import pyplot as plt
from PacketInterpreter.PacketInterpreter import PacketInterpretor
import json

def placeholder_function(packet_data, packet_name):
    print(f"Packet {packet_name} handle with data {packet_data}")
    pass

def tip_fup_packet_handler(packet_data, packet_name):
    pass

def mode_packet_handler():
    pass

def cbr_packet_definition():
    pass



def parser(bytes, interpretor : PacketInterpretor):
    for byte in bytes:
        interpretor.process_byte(byte)
    
    # print(json.dumps(interpretor.get_succession(), indent = 4))
    for el in interpretor.get_succession():
        print(str(el))



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