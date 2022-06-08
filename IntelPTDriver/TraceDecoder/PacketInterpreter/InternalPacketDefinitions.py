# constant definitions
import json

PACKET_TNT_TAKEN = 1
PACKET_TNT_NOT_TAKEN = 2
PACKET_FUP = 3
PACKET_TIP = 4
PACKET_TIP_PGE = 5
PACKET_TIP_PGD = 6
PACKET_CBR = 7
PACKET_MODE = 8
PACKET_PSB = 9
PACKET_PSBEND = 10
PACKET_TSC = 11
PACKET_UNDEFINED = 12

PACKET_ID_TO_STRING = {
    PACKET_TNT_TAKEN: "TAKEN",
    PACKET_TNT_NOT_TAKEN: "NOT_TAKEN",
    PACKET_FUP: "FUP",
    PACKET_TIP: "TIP",
    PACKET_TIP_PGE: "TIP_PGE",
    PACKET_TIP_PGD: "TIP_PGD",
    PACKET_CBR: "CBR",
    PACKET_MODE: "MODE",
    PACKET_PSB : "PSB",
    PACKET_PSBEND : "PSBEND",
    PACKET_TSC : "TSC",
    PACKET_UNDEFINED : "UNDEFINED"
}

class PacketBase:
    def __init__(self, packet_type : int, tsc : int = None) -> None:
        self.packet_id = packet_type
        self.tsc = tsc

    def __str__(self) -> str:
        return json.dumps(self.__dict__, indent=4)

class PacketTnt(PacketBase):
    def __init__(self, taken : bool, tsc : int) -> None:
        super().__init__(PACKET_TNT_TAKEN if taken else PACKET_TNT_NOT_TAKEN, tsc)
    
class PacketTip(PacketBase):
    def __init__(self, packet_type: int = PACKET_TIP, tsc: int = None) -> None:
        super().__init__(packet_type, tsc)
        self.type = 0
        self.address = None

    # def __setattr__(self, __name: str, __value: int) -> None:
    #     if __name == "type":
    #         self.type = __value
    #     elif __name == "address":
    #         self.address = __value
    #     else:
    #         raise AttributeError(f"Attribute {__name} not available")

    def __getattr__(self, __name: str) -> int:
        if __name not in self.__dict__.keys():
            raise AttributeError(f"Attribute {__name} not available")
        else:
            return self.__dict__[__name]

class PacketTipPge(PacketTip):
    def __init__(self, tsc) -> None:
        super().__init__(PACKET_TIP_PGE, tsc)

class PacketTipPgd(PacketTip):
    def __init__(self, tsc) -> None:
        super().__init__(PACKET_TIP_PGD, tsc)

class PacketFup(PacketTip):
    def __init__(self, tsc) -> None:
        super().__init__(PACKET_FUP, tsc)

class PacketTsc(PacketBase):
    def __init__(self, data) -> None:
        super().__init__(PACKET_TSC)

        packet_data = data[1:][::-1]
        self.tsc = 0
        for d in packet_data:
            self.tsc += d
            self.tsc <<= 8
        self.tsc >>= 8
    def __getattr__(self, __name: str):
        if __name not in self.__dict__.keys():
            raise AttributeError("No such attribute")
        return self.__dict__[__name]    
class PacketCbr(PacketBase):
    def __init__(self, data, tsc) -> None:
        super().__init__(PACKET_CBR, tsc)

        self.cbr = data[0]

    def __getattr__(self, __name: str):
        if __name not in self.__dict__.keys():
            raise AttributeError("No such attribute")
        return self.__dict__[__name]    

class PacketMode(PacketBase):
    MODE_PACKET_TYPES = {
        0b000 : "Exec",
        0b001 : "TSX"
    }
    
    def __init__(self, data, tsc) -> None:
        super().__init__(PACKET_MODE, tsc)
        
        type = data >> 5 & 0b0111

        if type not in PacketMode.MODE_PACKET_TYPES.keys():
            raise Exception(f"Type {type} for mode packet not available!") 
        
        self.type = PacketMode.MODE_PACKET_TYPES[type]

        if self.type == "Exec":
            self.IF = (data & (1 << 2)) != 0
            self.CSD = (data & (1 << 1)) != 0
            self.CSL_LMA = (data & 1) != 0

            if self.CSD and self.CSL_LMA:
                raise Exception("Accordin to intel manual both CSD and CSL_LMA should not be both 1! Trace is inconsistent!")

        else:
            self.TXAbort = (data & (1 << 1)) != 0
            self.InTX = (data & 1) != 0

    def __getattr__(self, __name: str):
        if __name in self.__dict__.keys():
            return self.__dict__[__name]
        if __name == "opmode":
            if self.type == "Exec":
                if self.CSL_LMA:
                    return "64"
                if self.CSD:
                    return "32"
                return "16"
        raise AttributeError("No such attribute")
