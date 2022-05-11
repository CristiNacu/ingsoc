from PacketInterpreter.InternalPacketDefinitions import *

def placeholder_function(packet_data, packet_name, context):
    print(f"Intercepted {packet_name} with default handler")
    return PacketBase(PACKET_UNDEFINED)

def psb_packet_handler(packet_data, packet_name, context):
    print(f"Intercepted PSB")
    context["disable_interpreting"] = True
    return PacketBase(PACKET_PSB)

def psbend_packet_handler(packet_data, packet_name, context):
    print(f"Intercepted PSBEND")
    context["disable_interpreting"] = False
    return PacketBase(PACKET_PSBEND)

def tnt_packet_handler(packet_data, packet_name, context) -> PacketBase:
    # TODO: Support large TNT packets

    jump_sequence = []

    aux_data = packet_data[0]
    idx = 0

    while (aux_data & 1 << 8) == 0 and idx < 8:
        idx += 1
        aux_data <<= 1
    
    idx += 1
    aux_data <<= 1

    while idx < 8:
        
        if aux_data & 1 << 8:
            jump_sequence.append(PacketTnt(True))
        else:
            jump_sequence.append(PacketTnt(False))

        idx += 1
        aux_data <<= 1

    return jump_sequence

LAST_IP_MASK = {
    0b001: 0xFFFFFFFFFFFF0000,
    0b010: 0xFFFFFFFF00000000,
    0b100: 0xFFFF000000000000
}

SIGN_EXTEND_MASK = {
    0 : 0x0000FFFFFFFFFFFF,
    1 : 0xFFFF000000000000
}

def fup_tip_packet_handler(packet_data, packet_name, context) -> PacketBase:
    if packet_name == "FUP":
        packet = PacketFup()
    elif packet_name == "TIP":
        packet = PacketTip()
    elif packet_name == "TIP PGE":
        packet = PacketTipPge()
    elif packet_name == "TIP PGD":
        packet = PacketTipPgd()
    else:
        raise Exception("Unknown packet name")

    payload_type = packet_data[0]
    payload_type = payload_type >> 5 & 0b0111
    packet.type = payload_type

    if payload_type == 0b000:
        return packet

    addr = 0
    for data in packet_data[1:][::-1]:
        addr += data
        addr <<= 8
    addr >>= 8
    if payload_type == 0b001 or payload_type == 0b010 or payload_type == 0b100:
        if "LastIP" not in context.keys():
            raise Exception("LastIP not available in context. Trace is inconsistent!")
        last_ip = context["LastIP"]

        addr = last_ip & LAST_IP_MASK[payload_type] | addr & (~LAST_IP_MASK[payload_type])
    
    elif payload_type == 0b011:
        sign_extension = 1 if (addr & 1 << 47) != 0 else 0
        addr = (addr | SIGN_EXTEND_MASK[sign_extension]) if sign_extension else (addr & SIGN_EXTEND_MASK[sign_extension])

    context["LastIP"] = addr
    packet.address = addr
    
    return packet


def mode_packet_handler(packet_data, packet_name, context):
    packet = PacketMode(packet_data[1])
    if packet.type == "Exec":
        context["addressing_mode"] = packet.opmode
    return packet

def cbr_packet_handler(packet_data, packet_name, context):
    return PacketCbr(packet_data[2:]) 
