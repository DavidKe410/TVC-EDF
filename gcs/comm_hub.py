import time
import ctypes
from PyQt6.QtCore import QThread, pyqtSignal
from pySerialTransfer import pySerialTransfer as txfer
from pySerialTransfer.pySerialTransfer import Status
import data_structs as ds

class TelemetryThread(QThread):
    # Define signals to send data back to the GUI thread safely
    telemetry_received = pyqtSignal(object)
    status_received = pyqtSignal(object)
    connection_error = pyqtSignal(str)

    def __init__(self, port='COM7', parent=None):
        super().__init__(parent)
        self.port = port
        self.running = False
        
        # Instantiate structures locally
        self.system_status = ds.StatusStruct()
        self.packed_data = ds.PackedDataStruct()
        self.command = ds.CommandStruct()

        # Timing constants
        self.CMD_RATE = 10
        self.STATUS_RATE = 500
        self.heartbeat_timeout = 3 * self.STATUS_RATE
        
        # State tracking
        self.last_cmd_ms = 0
        self.last_status_ms = 0
        self.last_espGCS_ms = 0

        self.serialTransfer = txfer.SerialTransfer(self.port)

    def current_ms(self):
        return int(time.perf_counter() * 1000)

    def run(self):
        """This replaces your while True loop and runs in the background."""
        self.running = True
        try:
            self.serialTransfer.open()
            time.sleep(2) # allow Arduino to reset
            
            while self.running:
                self.system_status.laptop_status.laptop_state = 1
                self.receive_data()
                self.send_data()
                self.heartbeat_check()
                
                # Small sleep to yield to other threads, adjust as needed for 100Hz
                time.sleep(0.001) 
                
        except Exception as e:
            self.connection_error.emit(str(e))
        finally:
            self.serialTransfer.close()

    def stop(self):
        self.running = False
        self.wait() # Wait for thread to safely exit

    def receive_data(self):
        if self.serialTransfer.available():
            if self.serialTransfer.id_byte == ds.PacketType.TelemetryPk:
                raw_payload = bytes(self.serialTransfer.rx_buff[:self.serialTransfer.bytes_read])
                ctypes.memmove(ctypes.addressof(self.packed_data), raw_payload, ctypes.sizeof(ds.PackedDataStruct))
                
                # Emit signal to GUI with a copy of the data
                self.telemetry_received.emit(self.packed_data)
            
            elif self.serialTransfer.id_byte == ds.PacketType.StatusPk:
                raw_status = bytes(self.serialTransfer.rx_buff[:self.serialTransfer.bytes_read])
                ctypes.memmove(ctypes.addressof(self.system_status), raw_status, ctypes.sizeof(ds.StatusStruct)-ctypes.sizeof(ds.laptopStatus))
                self.last_espGCS_ms = self.current_ms()
                
                self.status_received.emit(self.system_status)

        elif self.serialTransfer.status.value <= 0:
            if self.serialTransfer.status.name != "CRC_ERROR": # Filter spammy errors if needed
                 print(f'SERIAL ERROR: {self.serialTransfer.status.name}')

    def send_data(self):
        current_time = self.current_ms()
                
        if (((current_time - self.last_cmd_ms) >= self.CMD_RATE) and (self.system_status.esp_gcs_status.esp_gcs_state > -1)):
            self.command.overall_time = current_time
            # TODO: Pull these values from the GUI later instead of hardcoding
            self.command.servo1, self.command.servo2 = 1500, 1600
            self.command.servo3, self.command.servo4 = 1700, 1800
            self.command.motor = 1000
            self.command.cmd_ID = 0
            
            self.serialTransfer.tx_struct_obj(val_bytes=bytes(self.command))
            self.serialTransfer.send(ctypes.sizeof(self.command), packet_id=1)
            
            self.last_cmd_ms = current_time if (current_time - self.last_cmd_ms > 5 * self.CMD_RATE) else self.last_cmd_ms + self.CMD_RATE     

        if (current_time - self.last_status_ms >= self.STATUS_RATE):
            self.serialTransfer.tx_struct_obj(val_bytes=bytes(self.system_status.laptop_status))
            self.serialTransfer.send(ctypes.sizeof(self.system_status.laptop_status), packet_id=2)
            
            self.last_status_ms = current_time if (current_time - self.last_status_ms > 5 * self.STATUS_RATE) else self.last_status_ms + self.STATUS_RATE

    def heartbeat_check(self):
        if ((self.current_ms() - self.last_espGCS_ms) >= self.heartbeat_timeout):
            self.system_status.teensy_status.ac_state = -2 
            self.system_status.esp_ac_status.esp_ac_state = -2
            self.system_status.esp_gcs_status.esp_gcs_state = -2