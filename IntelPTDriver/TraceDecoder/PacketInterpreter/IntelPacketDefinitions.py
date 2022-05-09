from PacketInterpreter.PacketHandlers import *


unused_packet_bytes = [0x00, 0xFF]

ip_compression_packet_size = {
    0 : 0,
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

def placeholder_function(data, name):
    print(f"packet name {name} with data {data}")
    pass

packet_definitions = [
    {
        "packet_name" : "TNT",
        "packet_header" : [lambda data: True if data & 1 == 0 and data > 0x02 else 0],
        "packet_length" : 1,
        "handle" : tnt_packet_handler
    },
    {
        "packet_name" : "TNT Extended",
        "packet_header" : ["00000010", "10100011"],
        "packet_length" : 8,
        "handle" : tnt_packet_handler
    },
    {
        "packet_name" : "TIP",
        "packet_header" : ["...01101"],
        "packet_length" : lambda header : len(header) + ip_compression_packet_size[((header[0] >> 5) & 0b0111)],
        "handle" : fup_tip_packet_handler
    },
    {
        "packet_name" : "TIP PGE",
        "packet_header" : ["...10001"],
        "packet_length" : lambda header : len(header) + ip_compression_packet_size[((header[0] >> 5) & 0b0111)],  
        "handle" : fup_tip_packet_handler
    },
    {
        "packet_name" : "TIP PGD",
        "packet_header" : ["...00001"],
        "packet_length" : lambda header : len(header) + ip_compression_packet_size[((header[0] >> 5) & 0b0111)],
        "handle" : fup_tip_packet_handler
    },
    {
        "packet_name" : "FUP",
        "packet_header" : ["...11101"],
        "packet_length" : lambda header : len(header) + ip_compression_packet_size[((header[0] >> 5) & 0b0111)],
        "handle" : fup_tip_packet_handler
    },
    {
        "packet_name" : "PIP",
        "packet_header" : ["00000010", "01000011"],
        "packet_length" : 8,
        "handle" : placeholder_function
    },
    {
        "packet_name" : "MODE",
        "packet_header" : ["10011001"],
        "packet_length" : 2,
        "handle" : mode_packet_handler
    },
    {
        "packet_name" : "TRACE STOP",
        "packet_header" : ["00000010", "10000011"],
        "packet_length" : 2,
        "handle" : placeholder_function
    },
    {
        "packet_name" : "CBR",
        "packet_header" : ["00000010", "00000011"],
        "packet_length" : 4,
        "handle" : cbr_packet_handler
    },
    {
        "packet_name" : "TSC",
        "packet_header" : ["00011001"],
        "packet_length" : 8,
        "handle" : placeholder_function  
    },
    {
        "packet_name" : "MTC",
        "packet_header" : ["01011001"],
        "packet_length" : 2,
        "handle" : placeholder_function  
    },
    {
        "packet_name" : "TMA",
        "packet_header" : ["00000010", "01110011"],
        "packet_length" : 7,
        "handle" : placeholder_function  
    },
    # {
    #     "packet_name" : "CYC",
    #     "packet_header" : ["......11"],
    #     "repetitons" : 1,
    #     "handle" : placeholder_function
    # },
    {
        "packet_name" : "VMCS",
        "packet_header" : ["00000010", "11001000"],
        "packet_length" : 7,
        "handle" : placeholder_function
    },
    {
        "packet_name" : "OVF",
        "packet_header" : ["00000010", "11110011"],
        "packet_length" : 2,
        "handle" : placeholder_function
    },
    {
        "packet_name" : "PSB",
        "packet_header" : ["00000010", "10000010"] * 8,
        "packet_length" : 16,
        "handle" : placeholder_function  
    },
    {
        "packet_name" : "PSBEND",
        "packet_header" : ["00000010", "00100011"],
        "packet_length" : 2,
        "handle" : placeholder_function  
    },
    {
        "packet_name" : "MNT",
        "packet_header" : ["00000010", "11000011", "10001000"],
        "packet_length" : 11,
        "handle" : placeholder_function  
    },
    {
        "packet_name" : "PTW",
        "packet_header" : ["00000010", "...10010"],
        "packet_length" : lambda header : len(header) + ptw_packet_size[((header[1] >> 5) & 0b011)],
        "handle" : placeholder_function
    },
    {
        "packet_name" : "EXSTOP",
        "packet_header" : ["00000010", ".1100010"],
        "packet_length" : 2,
        "handle" : placeholder_function  
    },
    {
        "packet_name" : "MWAIT",
        "packet_header" : ["00000010", "11000010"],
        "packet_length" : 10,
        "handle" : placeholder_function  
    },
    {
        "packet_name" : "PWRE",
        "packet_header" : ["00000010", "00100010"],
        "packet_length" : 4,
        "handle" : placeholder_function  
    },
    {
        "packet_name" : "PWRX",
        "packet_header" : ["00000010", "10100010"],
        "packet_length" : 7,
        "handle" : placeholder_function  
    },
    {
        "packet_name" : "BBP",
        "packet_header" : ["00000010", "01100011"],
        "packet_length" : 3,
        "handle" : placeholder_function  
    },
    # {
    #     "packet_name" : "BIP",
    #     "packet_header" : [".....100", "01100011"],
    #     "packet_length" : None,
    #     "handle" : placeholder_function  
    # },
    {
        "packet_name" : "BEP",
        "packet_header" : ["00000010", ".0110011"],
        "packet_length" : 2,
        "handle" : placeholder_function  
    }
]
