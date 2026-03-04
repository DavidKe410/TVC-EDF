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
last_cmd_ms = 0
last_status_ms = 0

def current_ms():
    return int(time.time() * 1000)


if __name__ == '__main__':
    try:
        serialTransfer = txfer.SerialTransfer('COM7')
        
        serialTransfer.open()
        time.sleep(2) # allow some time for the Arduino to completely reset
        while True:
            if serialTransfer.available():
                if serialTransfer.id_byte == ds.PacketType.TelemetryPk:
                    raw_payload = bytes(serialTransfer.rx_buff[:serialTransfer.bytes_read])
                    #g_packed_data = ds.PackedDataStruct.from_buffer_copy(raw_payload)
                    ctypes.memmove(ctypes.addressof(g_packed_data), raw_payload, ctypes.sizeof(ds.PackedDataStruct))
                    print(g_packed_data.accel_z)
                elif serialTransfer.id_byte == ds.PacketType.StatusPk:
                    raw_status = bytes(serialTransfer.rx_buff[:serialTransfer.bytes_read])
                    ctypes.memmove(ctypes.addressof(g_system_status), raw_status, ctypes.sizeof(ds.StatusStruct)-ctypes.sizeof(ds.StatusStruct.laptopStatus))

            elif serialTransfer.status.value <= 0:
                if serialTransfer.status == Status.CRC_ERROR:
                    print('ERROR: CRC_ERROR')
                elif serialTransfer.status == Status.PAYLOAD_ERROR:
                    print('ERROR: PAYLOAD_ERROR')
                elif serialTransfer.status == Status.STOP_BYTE_ERROR:
                    print('ERROR: STOP_BYTE_ERROR')
                else:
                    print('ERROR: {}'.format(serialTransfer.status.name))
            
            current_time = current_ms()
            
            if (current_time - last_cmd_ms >= CMD_RATE):
                g_command.overall_time = int(current_time/1000)
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
                    last_cmd_ms += STATUS_RATE     

            g_system_status.laptop_status.laptop_state = 1

            if (current_time - last_status_ms >= STATUS_RATE):
                print("sending status")
                serialTransfer.tx_struct_obj(val_bytes=bytes(g_system_status.laptop_status))
                serialTransfer.send(ctypes.sizeof(g_system_status.laptop_status), packet_id=2)

                if (current_time - last_status_ms > 5 * STATUS_RATE):
                    last_status_ms = current_time # reset if behind
                else:
                    last_status_ms += STATUS_RATE


            # TODO: heartbeat status tracking implement
            
    
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