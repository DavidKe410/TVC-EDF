import ctypes
from enum import IntEnum

class PacketType(IntEnum):
    TelemetryPk = 0
    CommandPk = 1
    StatusPk = 2

class PackedDataStruct(ctypes.Structure):
    _pack_ = 1  # This is the same as __attribute__((packed))
    _fields_ = [
        ("packet_type",       ctypes.c_uint8),
        ("overall_time",      ctypes.c_uint32),
        ("state",             ctypes.c_int8),
        ("accel_time",        ctypes.c_uint32),
        ("accel_x",           ctypes.c_float),
        ("accel_y",           ctypes.c_float),
        ("accel_z",           ctypes.c_float),
        ("orien_time",        ctypes.c_uint32),
        ("real",              ctypes.c_float),
        ("i",                 ctypes.c_float),
        ("j",                 ctypes.c_float),
        ("k",                 ctypes.c_float),
        ("temp",              ctypes.c_int8),
        ("new_accel",         ctypes.c_uint8),
        ("orien_cali_status", ctypes.c_int8),
    ]
    def __init__(self, packet_type=PacketType.TelemetryPk):
        super().__init__(packet_type=packet_type)

class CommandStruct(ctypes.Structure):
    _pack_ = 1
    _fields_ = [
        ("packet_type", ctypes.c_uint8),
        ("type_cmd",     ctypes.c_uint8),
        ("state",       ctypes.c_int8),
        ("overall_time", ctypes.c_uint32),
        ("servo1",      ctypes.c_uint16),
        ("servo2",      ctypes.c_uint16),
        ("servo3",      ctypes.c_uint16),
        ("servo4",      ctypes.c_uint16),
        ("motor",       ctypes.c_uint16),
        ("cmd_ID",      ctypes.c_uint16),
    ]
    def __init__(self, packet_type=PacketType.CommandPk, type_cmd=0, state=0):
        super().__init__(packet_type=packet_type, type_cmd=type_cmd, state=state)

class teensyStatus(ctypes.Structure):
    _pack_ = 1
    _fields_ = [
        ("packet_type", ctypes.c_uint8),
        ("overall_time", ctypes.c_uint32),
        ("ac_state",     ctypes.c_int8),
        ("bno_state",    ctypes.c_int8),
        ("ism_state",    ctypes.c_int8),
        ("sd_state",     ctypes.c_int8),
        ("cmd_ack_ID",   ctypes.c_uint16),
    ]
    def __init__(self, packet_type=PacketType.StatusPk, ac_state=-1, bno_state=-1, ism_state=-1, sd_state=-1):
        super().__init__(packet_type=packet_type, ac_state=ac_state, bno_state=bno_state, ism_state=ism_state, sd_state=sd_state)

class espACStatus(ctypes.Structure):
    _pack_ = 1
    _fields_ = [
        ("packet_type", ctypes.c_uint8),
        ("esp_ac_state", ctypes.c_int8),
        ("temperature", ctypes.c_int8),
        ("RSSI", ctypes.c_int8),
    ]
    def __init__(self, packet_type=PacketType.StatusPk, esp_ac_state=-1, RSSI=-100):
        super().__init__(packet_type=packet_type, esp_ac_state=esp_ac_state, RSSI=RSSI)

class espGCSStatus(ctypes.Structure):
    _pack_ = 1
    _fields_ = [
        ("packet_type", ctypes.c_uint8),
        ("esp_gcs_state", ctypes.c_int8),
        ("temperature", ctypes.c_int8),
        ("RSSI", ctypes.c_int8), # RSSI doesn't work bu maybe in some future updates, we'll be able to implement
    ]
    def __init__(self, packet_type=PacketType.StatusPk, esp_gcs_state=-1, RSSI=-100):
        super().__init__(packet_type=packet_type, esp_gcs_state=esp_gcs_state, RSSI=RSSI)

class laptopStatus(ctypes.Structure):
    _pack_ = 1
    _fields_ = [
        ("packet_type", ctypes.c_uint8),
        ("laptop_state", ctypes.c_int8),
    ]
    def __init__(self, packet_type=PacketType.StatusPk, laptop_state=-1):
        super().__init__(packet_type=packet_type, laptop_state=laptop_state)

class StatusStruct(ctypes.Structure):
    _pack_ = 1
    _fields_ = [
        ("teensy_status", teensyStatus),
        ("esp_ac_status", espACStatus),
        ("esp_gcs_status", espGCSStatus),
        ("laptop_status", laptopStatus),
    ]
