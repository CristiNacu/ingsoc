from distutils.command import clean
import sys
import re

def read_trace(file_path: str):
    with open(file_path, "rb") as f:
        bytes = f.read()
    
    packetStreamBoundary = re.compile(b'(\x02\x82){8}')
    packetPSBEND = re.compile(b'(\x02\x23)')
    packetTSC = re.compile(b'(\x19)')
    packetCBR = re.compile(b'(\x02\x03)')
    packetOVF = re.compile(b'(\x02\xF3)')

    packetTraceStop = re.compile(b'(\x01\x83)')

    psbStartOffsets = []
    psbEndOffsets = []

    basePackets = {
        {
            "packet_name" : "TIP",
            "base" : ["***01101"],
            "multiple_forms" : True,
            "repeats" : 1
        },
        {
            "packet_name" : "TIP_PGE",
            "base" : [0b00001101],
            "multiple_forms" : True,
            "repeats" : 1
        },
        {
            "packet_name" : "TIP_PGD",
            "base" : [0b00001101],
            "multiple_forms" : True,
            "repeats" : 1
        },
        {
            "packet_name" : "FUP",
            "base" : [0b00001101],
            "multiple_forms" : True,
            "repeats" : 1
        },
        {
            "packet_name" : "PIP",
            "base" : [0b00001101],
            "multiple_forms" : True,
            "repeats" : 1
        },
        {
            "packet_name" : "MODE",
            "base" : [0b00001101],
            "multiple_forms" : True,
            "repeats" : 1
        },
        {
            "packet_name" : "TRACE_STOP",
            "base" : [0b00001101],
            "multiple_forms" : True,
            "repeats" : 1
        },
        {
            "packet_name" : "CBR",
            "base" : [0b00001101],
            "multiple_forms" : True,
            "repeats" : 1
        },
        {
            "packet_name" : "TSC",
            "base" : [0b00001101],
            "multiple_forms" : True,
            "repeats" : 1
        },
        {
            "packet_name" : "MTC",
            "base" : [0b00001101],
            "multiple_forms" : True,
            "repeats" : 1
        },
        {
            "packet_name" : "OVF",
            "base" : [0b00001101],
            "multiple_forms" : True,
            "repeats" : 1
        },
        {
            "packet_name" : "PSB",
            "base" : [0b00001101],
            "multiple_forms" : True,
            "repeats" : 1
        },
        {
            "packet_name" : "PSBEND",
            "base" : [0b00001101],
            "multiple_forms" : True,
            "repeats" : 1
        },
        {
            "packet_name" : "BBP",
            "base" : [0b00001101],
            "multiple_forms" : True,
            "repeats" : 1
        },
        {
            "packet_name" : "BEP",
            "base" : [0b00001101],
            "multiple_forms" : True,
            "repeats" : 1
        },
    }

    for psbStart in packetStreamBoundary.finditer(bytes):
        psbStartOffsets.append(psbStart.start())

    
    for psbEnd in packetPSBEND.finditer(bytes):
        psbEndOffsets.append(psbEnd.end())

    print(f"")
    print(f"Packet Stream Boundary count: {len(psbStartOffsets)}")
    print(f"Packet PSBEND count: {len(psbEndOffsets)}")
    print(f"Packet TSC count: {len(packetTSC.findall(bytes))}")
    print(f"Packet CBR count: {len(packetCBR.findall(bytes))}")
    print(f"Packet OVF count: {len(packetOVF.findall(bytes))}")
    print(f"Packet Trace Stop count: {len(packetTraceStop.findall(bytes))}")


    print(f"PSB start offsets: {psbStartOffsets}")
    print(f"PSB end offsets: {psbEndOffsets}")

    cleanTrace = bytes[ : psbStartOffsets[0]] 

    for i in range(len(psbEndOffsets) - 1):
        cleanTrace += bytes[psbEndOffsets[i] : psbStartOffsets[i]]

    cleanTrace += bytes[psbEndOffsets[len(psbEndOffsets) - 1]: ]

    # print(cleanTrace.hex())

    with open("file.bin", 'wb') as f:
        f.write(cleanTrace)

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