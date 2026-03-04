import time
import ctypes
import data_structs as ds
from pySerialTransfer import pySerialTransfer as txfer
from pySerialTransfer.pySerialTransfer import Status

g_system_status = ds.StatusStruct()
g_packed_data = ds.PackedDataStruct()
g_command = ds.CommandStruct()

CMD_RATE = 10
STATUS_RATE = 500
heartbeat_timeout = 3 * STATUS_RATE
last_cmd_ms = 0
last_status_ms = 0
last_espGCS_ms = 0

def current_ms():
    return int(time.perf_counter() * 1000)


if __name__ == '__main__':
    try:
        serialTransfer = txfer.SerialTransfer('COM7')
        
        serialTransfer.open()
        time.sleep(2) # allow some time for the Arduino to completely reset
        while True:
            if serialTransfer.available():
                if serialTransfer.id_byte == ds.PacketType.TelemetryPk:
                    raw_payload = bytes(serialTransfer.rx_buff[:serialTransfer.bytes_read])
                    ctypes.memmove(ctypes.addressof(g_packed_data), raw_payload, ctypes.sizeof(ds.PackedDataStruct))
                elif serialTransfer.id_byte == ds.PacketType.StatusPk:
                    raw_status = bytes(serialTransfer.rx_buff[:serialTransfer.bytes_read])
                    ctypes.memmove(ctypes.addressof(g_system_status), raw_status, ctypes.sizeof(ds.StatusStruct)-ctypes.sizeof(ds.laptopStatus))
                    last_espGCS_ms = current_ms()

            elif serialTransfer.status.value <= 0:
                if serialTransfer.status == Status.CRC_ERROR:
                    print('ERROR: CRC_ERROR')
                elif serialTransfer.status == Status.PAYLOAD_ERROR:
                    print('ERROR: PAYLOAD_ERROR')
                elif serialTransfer.status == Status.STOP_BYTE_ERROR:
                    print('ERROR: STOP_BYTE_ERROR')
                else:
                    print('ERROR: {}'.format(serialTransfer.status.name))
            
            g_system_status.laptop_status.laptop_state = 1
            current_time = current_ms()
            
            if (((current_time - last_cmd_ms) >= CMD_RATE) and (g_system_status.esp_gcs_status.esp_gcs_state > -1)):
                g_command.overall_time = current_time
                g_command.servo1 = 1500
                g_command.servo2 = 1600
                g_command.servo3 = 1700
                g_command.servo4 = 1800
                g_command.motor  = 1000
                g_command.cmd_ID = 0
                serialTransfer.tx_struct_obj(val_bytes=bytes(g_command))
                serialTransfer.send(ctypes.sizeof(g_command), packet_id=1)
                if (current_time - last_cmd_ms > 5 * CMD_RATE):
                    last_cmd_ms = current_time # reset if behind
                else:
                    last_cmd_ms += CMD_RATE     

            if (current_time - last_status_ms >= STATUS_RATE):
                print("sending status")

                serialTransfer.tx_struct_obj(val_bytes=bytes(g_system_status.laptop_status))
                serialTransfer.send(ctypes.sizeof(g_system_status.laptop_status), packet_id=2)

                if (current_time - last_status_ms > 5 * STATUS_RATE):
                    last_status_ms = current_time # reset if behind
                else:
                    last_status_ms += STATUS_RATE

            if ((current_ms()-last_espGCS_ms) >= heartbeat_timeout):
                g_system_status.teensy_status.ac_state = -2; # mark as disconnected
                g_system_status.esp_ac_status.esp_ac_state = -2
                g_system_status.esp_gcs_status.esp_gcs_state = -2
            
    
    except KeyboardInterrupt:
        try:
            serialTransfer.close()
        except:
            pass
    
    except:
        import traceback
        traceback.print_exc()
        
        try:
            serialTransfer.close()
        except:
            pass