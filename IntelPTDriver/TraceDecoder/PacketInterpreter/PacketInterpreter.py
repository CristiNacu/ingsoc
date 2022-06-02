from typing import Any
from PacketInterpreter.IntelPacketDefinitions import packet_definitions, unused_packet_bytes


class PacketInterpretor:
    
    def __init__(self, processor_frequency: int = None, image_base: int = None, image_size: int = None) -> None:

        # Counter for the current byte
        self.__internal_byte_idx = 0

        # Bytes accumulator for the [potential] current packet
        self.__accumulator = []
        # Possible packet types the current accumulated bytes could lead to
        self.__current_candidates = packet_definitions
        # Index of the current header byte
        self.__header_index = 0

        # Decoded packets up untill the current point
        self.__succession = []

        # A header has been matched with a packet definition
        self.__in_packet = False
        # The current matched packet
        self.__current_packet = None
        # Number of bytes in the current packet
        self.__current_packet_length = 0
        # Context for hanlders to maintain data inside
        self.__context = {"disable_interpreting": False, "tsc": None}

        self.__proc_frequency = processor_frequency
        self.__img_base =  image_base
        self.__img_size = image_size

    def __validate_header_byte(self, data_byte : int, header_definition : Any) -> bool:
        """
        Small bitwise regex implementation for matching header definition.\n
        Returns true if the pattern matches the data_byte.\n
        Supports custom functions as header_definition 
        Operations supported 1 0 .\n
        TODO: implement additional operations\n
        """

        if callable(header_definition):
            return header_definition(data_byte)

        header_definition = header_definition[::-1]

        for bit_index in range(len(header_definition)):
            if header_definition[bit_index] == '.':
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

    def __search_header(self, byte : int):
        """
        Searches the byte in the list of potential packets header regex for a match.
        If more matches are available for a byte, the byte is accumulated until a single definitive match is found.\n

        When a packet is detected, it's length (either int or a method call is performed) to determine the number of bytes needed to be accumulated.
        All the accumulated bytes are sent to the handle and a packet definition is expected to be returned.\n

        The handle is provided with a context which can be used for storing / reading data and all the accumulated bytes\n 
        """
        # print(f"BYTE IDX {self.__internal_byte_idx} BYTE DATA {hex(byte)}")
        
        # List of new potential candidate apckets based on the current header matching
        new_candidates = []
        # Current candidate packet headers up untill the previous byte
        packet_candidates = self.__current_candidates

        # Itterate the list of candidates
        for packet_idx in range(len(packet_candidates)):

            # Cache packet and current header byte regex
            current_tested_packet = packet_candidates[packet_idx]
            packet_heders = current_tested_packet["packet_header"]

            # Test the current byte for a match with the current header byte regex
            if self.__validate_header_byte(byte, packet_heders[self.__header_index]):
                
                # If the entire header has been validated for a packet, mark it as the one in process
                if self.__header_index == len(packet_heders) - 1:

                    # Reset state
                    new_candidates = []
                    self.__header_index = 0
                    self.__in_packet = True
                    self.__current_packet = current_tested_packet
                    
                    # Get the length in bytes of the current packet
                    current_packet_length = current_tested_packet["packet_length"]
                    if callable(current_packet_length):
                        current_packet_length = current_packet_length(self.__accumulator)
                    self.__current_packet_length = current_packet_length

                    break

                # Otherwise the current packet is a potential candidate
                else:
                    new_candidates.append(current_tested_packet)
        
        # If a packet hasn't been decided upon, check the next header in the packet definition and
        # mark the next round of candidates
        if not self.__in_packet:
            self.__header_index += 1
            self.__current_candidates = new_candidates
    
    def process_byte(self, byte):
        """
        Entry point for the state machine. Receives IPT bytes as a stream, internally detects, buffers and tracks packets.\n
        """

        # Increment internal byte count
        self.__internal_byte_idx += 1

        # If no packet detected try detecting one using the current byte
        if not self.__in_packet:
            # If a restricted value was given whilst trying to detect a header, obviously the trace is flawed.
            # Acknowledge the error
            if byte in unused_packet_bytes:
                if len(self.__current_candidates) != len(packet_definitions) and len(self.__current_candidates) != 0:
                    print(f"[ERROR] while decoding bytes {self.__accumulator} detected inconsistent byte {byte} at index {self.__internal_byte_idx}")
                    self.__current_candidates = packet_definitions
                    self.__accumulator = []
                    self.__header_index = 0
                return

            # Buffer the current byte and try using it for header detection
            self.__accumulator.append(byte)
            self.__search_header(byte)
        else:
            # If a packet is currently buffering, add the new byte as data for this packet
            self.__accumulator.append(byte)
        
        # If the packet is fully buffered, process it and start detecting the next one
        if self.__current_packet is not None and len(self.__accumulator) >= self.__current_packet_length:
            
            packet_name = self.__current_packet["packet_name"]

            # Track the current packet
            # self.__succession.append([packet_name,  [hex(el) for el in self.__accumulator]])
            
            # Convert the packet from bytes to relevant internal data
            if "handle" in self.__current_packet.keys() and self.__current_packet["handle"] is not None:
                result = self.__current_packet["handle"](self.__accumulator, packet_name, self.__context)
            else:
                print(f"[WARNING] Packet type {packet_name} has no handle!")
            
            if self.__context["disable_interpreting"] is False:
                if isinstance(result, list):
                    self.__succession.extend(result)
                else:
                    self.__succession.append(result)
            else:
                print(f"dropped packet {packet_name} due to interpreter being disabled!")
            
            # Reset internal state
            self.__in_packet = False
            self.__accumulator = []
            self.__current_candidates = packet_definitions
            self.__header_index = 0

    def get_succession(self) -> list:
        return self.__succession
