from distutils.command import clean
import sys
import re
import json

ip_compression_packet_size = {
    0 : 1,
    1 : 2,
    2 : 4,
    3 : 6,
    4 : 6,
    5 : None,
    6 : 8,
    7 : None
}

ptw_packet_size = {
    0 : 4,
    1 : 8,
    2 : None,
    3 : None
}

packet_definitions = [
    {
        "packet_name" : "TNT",
        "packet_header" : ["1******0"],
        "repetitons" : 1,
        "packet_length" : 1
    },
    {
        "packet_name" : "TNT Extended",
        "packet_header" : ["00000010", "10100011"],
        "repetitons" : 1,
        "packet_length" : 8  
    },
    {
        "packet_name" : "TIP",
        "packet_header" : ["***01101"],
        "repetitons" : 1,
        "packet_length" : lambda header : len(header) + ip_compression_packet_size[((header[0] >> 5) & 0b0111)]
    },
    {
        "packet_name" : "TIP PGE",
        "packet_header" : ["***10001"],
        "repetitons" : 1,
        "packet_length" : lambda header : len(header) + ip_compression_packet_size[((header[0] >> 5) & 0b0111)]  
    },
    {
        "packet_name" : "TIP PGD",
        "packet_header" : ["***00001"],
        "repetitons" : 1,
        "packet_length" : lambda header : len(header) + ip_compression_packet_size[((header[0] >> 5) & 0b0111)]  
    },
    {
        "packet_name" : "FUP",
        "packet_header" : ["***11101"],
        "repetitons" : 1,
        "packet_length" : lambda header : len(header) + ip_compression_packet_size[((header[0] >> 5) & 0b0111)]  
    },
    {
        "packet_name" : "PIP",
        "packet_header" : ["00000010", "01000011"],
        "repetitons" : 1,
        "packet_length" : 8
    },
    {
        "packet_name" : "MODE",
        "packet_header" : ["10011001"],
        "repetitons" : 1,
        "packet_length" : 2
    },
    {
        "packet_name" : "TRACE STOP",
        "packet_header" : ["00000010", "10000011"],
        "repetitons" : 1,
        "packet_length" : 2  
    },
    {
        "packet_name" : "CBR",
        "packet_header" : ["00000010", "00000011"],
        "repetitons" : 1,
        "packet_length" : 4  
    },
    {
        "packet_name" : "TSC",
        "packet_header" : ["00011001"],
        "repetitons" : 1,
        "packet_length" : 8  
    },
    {
        "packet_name" : "MTC",
        "packet_header" : ["01011001"],
        "repetitons" : 1,
        "packet_length" : 2  
    },
    {
        "packet_name" : "TMA",
        "packet_header" : ["00000010", "01110011"],
        "repetitons" : 1,
        "packet_length" : 7  
    },
    # {
    #     "packet_name" : "CYC",
    #     "packet_header" : ["******11"],
    #     "repetitons" : 1,
    #     "packet_length" : 1  
    # },
    {
        "packet_name" : "VMCS",
        "packet_header" : ["00000010", "11001000"],
        "repetitons" : 1,
        "packet_length" : 7  
    },
    {
        "packet_name" : "OVF",
        "packet_header" : ["00000010", "11110011"],
        "repetitons" : 1,
        "packet_length" : 2  
    },
    {
        "packet_name" : "PSB",
        "packet_header" : ["00000010", "10000010"] * 8,
        "repetitons" : 8,
        "packet_length" : 16  
    },
    {
        "packet_name" : "PSBEND",
        "packet_header" : ["00000010", "00100011"],
        "repetitons" : 1,
        "packet_length" : 2  
    },
    {
        "packet_name" : "MNT",
        "packet_header" : ["00000010", "11000011", "10001000"],
        "repetitons" : 1,
        "packet_length" : 11  
    },
    {
        "packet_name" : "PTW",
        "packet_header" : ["00000010", "***10010"],
        "repetitons" : 1,
        "packet_length" : lambda header : len(header) + ptw_packet_size[((header[1] >> 5) & 0b011)]  
    },
    {
        "packet_name" : "EXSTOP",
        "packet_header" : ["00000010", "*1100010"],
        "repetitons" : 1,
        "packet_length" : 2  
    },
    {
        "packet_name" : "MWAIT",
        "packet_header" : ["00000010", "11000010"],
        "repetitons" : 1,
        "packet_length" : 10  
    },
    {
        "packet_name" : "PWRE",
        "packet_header" : ["00000010", "00100010"],
        "repetitons" : 1,
        "packet_length" : 4  
    },
    {
        "packet_name" : "PWRX",
        "packet_header" : ["00000010", "10100010"],
        "repetitons" : 1,
        "packet_length" : 7  
    },
    {
        "packet_name" : "BBP",
        "packet_header" : ["00000010", "01100011"],
        "repetitons" : 1,
        "packet_length" : 3  
    },
    # {
    #     "packet_name" : "BIP",
    #     "packet_header" : ["*****100", "01100011"],
    #     "repetitons" : 1,
    #     "packet_length" : None  
    # },
    {
        "packet_name" : "BEP",
        "packet_header" : ["00000010", "*0110011"],
        "repetitons" : 1,
        "packet_length" : 2  
    }
]

unused_packet_bytes = [0x00, 0xFF]

class PacketInterpretor:
    
    def __init__(self) -> None:
        self.__internal_byte_idx = 0

        self.__accumulator = []
        self.__current_candidates = packet_definitions
        self.__header_index = 0

        self.__succession = []

        self.__in_packet = False
        self.__current_packet = None
        self.__current_packet_length = 0
        pass

    def __validate_heder_byte(self, data_byte, header_definition):

        header_definition = header_definition[::-1]

        for bit_index in range(len(header_definition)):
            if header_definition[bit_index] == '*':
                continue

            if header_definition[bit_index] == '1':
                if data_byte & (1 << bit_index) == 0:
                    return False
                else:
                    continue
            else:
                if data_byte & (1 << bit_index) != 0:
                    return False
                else:
                    continue
        return True

    def __search_header(self, byte):
        print(f"BYTE IDX {self.__internal_byte_idx} BYTE DATA {hex(byte)}")
        
        new_candidates = []

        packet_candidates = self.__current_candidates
        # if packet_candidates == []:
        #     packet_candidates = packet_definitions

        # while len(packet_candidates) != 0: # bine acolo 

        for packet_idx in range(len(packet_candidates)):
            current_tested_packet = packet_candidates[packet_idx]
            packet_heders = current_tested_packet["packet_header"]

            if self.__validate_heder_byte(byte, packet_heders[self.__header_index]):
                if self.__header_index == len(packet_heders) - 1:
                    new_candidates = []
                    self.__header_index = 0
                    self.__in_packet = True
                    self.__current_packet = current_tested_packet
                    
                    current_packet_length = current_tested_packet["packet_length"]
                    if callable(current_packet_length):
                        current_packet_length = current_packet_length(self.__accumulator)

                    self.__current_packet_length = current_packet_length
                    break
                else:
                    new_candidates.append(current_tested_packet)
        
        if not self.__in_packet:
            self.__header_index += 1
            self.__current_candidates = new_candidates
                
    def process_byte(self, byte):
        self.__internal_byte_idx += 1

        if not self.__in_packet:
            if byte in unused_packet_bytes:
                return
            self.__accumulator.append(byte)
            self.__search_header(byte)
        else:
            self.__accumulator.append(byte)
        
        if self.__current_packet is not None and len(self.__accumulator) >= self.__current_packet_length:
            self.__succession.append({self.__current_packet["packet_name"]: [hex(el) for el in self.__accumulator]})
            self.__in_packet = False
            self.__accumulator = []
            self.__current_candidates = packet_definitions
            self.__header_index = 0

    def get_succession(self):
        return self.__succession


def parser(bytes, interpretor : PacketInterpretor):
    for byte in bytes:
        interpretor.process_byte(byte)
    print(json.dumps(interpretor.get_succession(), indent = 4))

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