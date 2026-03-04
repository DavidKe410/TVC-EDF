import time
import ctypes
import data_structs as ds
from pySerialTransfer import pySerialTransfer as txfer
from pySerialTransfer.pySerialTransfer import Status

g_system_status = ds.StatusStruct()
g_packed_data = ds.PackedDataStruct()
g_command = ds.CommandStruct()


if __name__ == '__main__':
    try:
        serialTransfer = txfer.SerialTransfer('COM7')
        
        serialTransfer.open()
        time.sleep(2) # allow some time for the Arduino to completely reset
        
        while True:
            if serialTransfer.available():
                if serialTransfer.id_byte == ds.PacketType.TelemetryPk:
                    ctypes.memmove(ctypes.addressof(g_packed_data), ctypes.addressof(serialTransfer.rx_buff), ctypes.sizeof(g_packed_data))
                elif serialTransfer.id_byte == ds.PacketType.StatusPk:
                    ctypes.memmove(ctypes.addressof(g_system_status), ctypes.addressof(serialTransfer.rx_buff), ctypes.sizeof(g_system_status))

            elif serialTransfer.status.value <= 0:
                if serialTransfer.status == Status.CRC_ERROR:
                    print('ERROR: CRC_ERROR')
                elif serialTransfer.status == Status.PAYLOAD_ERROR:
                    print('ERROR: PAYLOAD_ERROR')
                elif serialTransfer.status == Status.STOP_BYTE_ERROR:
                    print('ERROR: STOP_BYTE_ERROR')
                else:
                    print('ERROR: {}'.format(serialTransfer.status.name))
            send_size = 0
            
            # Sending it:
            # cmd = ControlCommand(cmd_type=1, value=45.0)
            # size = link.tx_obj(cmd)
            # link.send(size)

            
    
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