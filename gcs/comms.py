import time
import copy
import ctypes
from PyQt6.QtCore import QThread, pyqtSignal
from pySerialTransfer import pySerialTransfer as txfer
import data_structs as ds
from logger import LoggerThread

class CommThread(QThread):
    # Define signals to send data back to the GUI thread safely
    telemetry_received = pyqtSignal(object)
    status_received = pyqtSignal(object)
    connection_error = pyqtSignal(str)

    def __init__(self, port='COM7', logger=None, serialTransfer=None, parent=None):
        super().__init__(parent)
        self.running = True
        self.serial_enabled = False
        self.port = port
        self.logger = logger
        self.serialTransfer = serialTransfer
        self.resending_cmd = None
        
        # Instantiate structures locally
        self.system_status = ds.StatusStruct()
        self.packed_data = ds.PackedDataStruct()
        self.command = ds.CommandStruct()
        # individually initialize sub-structs to trigger their __init__ defaults
        self.system_status.teensy_status = ds.teensyStatus()
        self.system_status.esp_ac_status = ds.espACStatus()
        self.system_status.esp_gcs_status = ds.espGCSStatus()
        self.system_status.laptop_status = ds.laptopStatus()

        # for record keeping / acknowledgements
        self.previous_cmds = []

        # Timing constants
        self.CMD_INTERVAL_MS = 10
        self.STATUS_INTERVAL_MS = 500
        self.heartbeat_timeout = 3 * self.STATUS_INTERVAL_MS
        
        # State tracking
        self.last_cmd_ms = 0
        self.last_status_ms = 0
        self.last_espGCS_ms = 0

    def run(self):
        while self.running:
            try:
                if (self.serial_enabled and self.serialTransfer is None):
                    self.connect_serial()
                elif (self.serial_enabled and self.serialTransfer is not None):
                    self.system_status.laptop_status.laptop_state = 1
                    self.receive_data()
                    self.send_data()
                    self.heartbeat_check()
                elif (not self.serial_enabled and self.serialTransfer is not None):
                    self.disconnect_serial()
                
                time.sleep(0.001)  # Small sleep to yield to other threads, adjust as needed for 100Hz
                    
            except Exception as e:
                if self.serialTransfer is not None:
                    self.disconnect_serial()
                self.connection_error.emit(str(e))


    def stop(self):
        self.running = False
        self.wait() # Wait for thread to safely exit

    def connect_serial(self):
        self.serialTransfer = txfer.SerialTransfer(self.port)
        self.serialTransfer.open()
        time.sleep(1.5)

    def disconnect_serial(self):
        print(f"Serial connection disconnected.")
        self.serialTransfer.close()
        self.serialTransfer = None
        self.system_status.laptop_status.laptop_state = -2

    def current_ms(self):
        return int(time.perf_counter() * 1000)

    def receive_data(self):
        if self.serialTransfer.available():
            if self.serialTransfer.id_byte == ds.PacketType.TelemetryPk:
                raw_payload = bytes(self.serialTransfer.rx_buff[:self.serialTransfer.bytes_read])
                ctypes.memmove(ctypes.addressof(self.packed_data), raw_payload, ctypes.sizeof(ds.PackedDataStruct))
                
                data_copy = copy.copy(self.packed_data) #apparently possible that gui reads it while we write to it
                if self.logger.logging_enabled == True:
                    self.logger.log(ds.PacketType.TelemetryPk, data_copy)
                self.telemetry_received.emit(data_copy)
            
            elif self.serialTransfer.id_byte == ds.PacketType.StatusPk:
                raw_status = bytes(self.serialTransfer.rx_buff[:self.serialTransfer.bytes_read])
                ctypes.memmove(ctypes.addressof(self.system_status), raw_status, ctypes.sizeof(ds.StatusStruct)-ctypes.sizeof(ds.laptopStatus))
                self.last_espGCS_ms = self.current_ms()
                
                status_copy = copy.copy(self.system_status)
                if self.logger.logging_enabled == True:
                    self.logger.log(ds.PacketType.StatusPk, status_copy)
                self.status_received.emit(status_copy)
                self.ack_cmd_check()

        elif self.serialTransfer.status.value <= 0:
            print(f'SERIAL ERROR: {self.serialTransfer.status.name}')

    def send_data(self):
        current_time = self.current_ms()
                
        if (((current_time - self.last_cmd_ms) >= self.CMD_INTERVAL_MS) and (self.system_status.esp_gcs_status.esp_gcs_state > -1)):
            self.command.overall_time = current_time
            # TODO: Pull these values from the GUI later instead of hardcoding
            if self.resending_cmd is None:
                self.command.servo1, self.command.servo2, self.command.servo3, self.command.servo4, self.command.motor = 1500, 1600, 1700, 1800, 1000
            else:
                ctypes.pointer(self.command)[0] = self.resending_cmd
            self.serialTransfer.tx_struct_obj(val_bytes=bytes(self.command))
            self.serialTransfer.send(ctypes.sizeof(self.command), packet_id=ds.PacketType.CommandPk)
            if self.logger.logging_enabled == True:
                self.logger.log(ds.PacketType.CommandPk, self.command)
            self.record_cmd() # could make another flag to indicate new cmd isntead of going through the list each time but eh
            
            if self.resending_cmd is not None: # placeholder if we want to do more after resending it
                self.resending_cmd = None

            self.last_cmd_ms = current_time if (current_time - self.last_cmd_ms > 5 * self.CMD_INTERVAL_MS) else self.last_cmd_ms + self.CMD_INTERVAL_MS

        if (current_time - self.last_status_ms >= self.STATUS_INTERVAL_MS):
            self.serialTransfer.tx_struct_obj(val_bytes=bytes(self.system_status.laptop_status))
            self.serialTransfer.send(ctypes.sizeof(self.system_status.laptop_status), packet_id=ds.PacketType.StatusPk)

            # jsut testing cmd iteration
            if self.command.cmd_ID < 9:
                self.command.cmd_ID += 1   

            self.last_status_ms = current_time if (current_time - self.last_status_ms > 5 * self.STATUS_INTERVAL_MS) else self.last_status_ms + self.STATUS_INTERVAL_MS

    def record_cmd(self):
        for i in range(len(self.previous_cmds)):
            if self.command.cmd_ID == self.previous_cmds[i][0].cmd_ID:
                return True
        new_entry = [type(self.command).from_buffer_copy(self.command), time.time()]
        self.previous_cmds.append(new_entry)
        self.previous_cmds = self.previous_cmds[-25:] # limit history to just past 25 for now
        return False
        
    def ack_cmd_check(self):
        for i in range(len(self.previous_cmds)):
            if self.system_status.teensy_status.cmd_ack_ID == self.previous_cmds[i][0].cmd_ID:
                self.previous_cmds[i][1] = -1

    def resend_cmd(self, ind):
        self.resending_cmd = self.previous_cmds[ind][0]

    def heartbeat_check(self):
        if ((self.current_ms() - self.last_espGCS_ms) >= self.heartbeat_timeout):
            self.system_status.teensy_status.ac_state = -2 
            self.system_status.esp_ac_status.esp_ac_state = -2
            self.system_status.esp_gcs_status.esp_gcs_state = -2
