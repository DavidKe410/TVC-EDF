import time
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
                    g_packed_data = serialTransfer.rx_obj(obj_type=ds.PackedDataStruct)
                elif serialTransfer.id_byte == ds.PacketType.StatusPk:
                    g_system_status = serialTransfer.rx_obj(obj_type=ds.StatusStruct)
                recSize = 0

                
                arr = serialTransfer.rx_obj(obj_type=str,
                                  start_pos=recSize,
                                  obj_byte_size=5)
                recSize += len(arr)
                
                print('{}{} | {}'.format(testStruct.z, testStruct.y, arr))
                
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


            ###################################################################
            # Send a list
            ###################################################################
            list_ = [1, 3]
            list_size = serialTransfer.tx_obj(list_)
            send_size += list_size
            
            ###################################################################
            # Send a string
            ###################################################################
            str_ = 'hello'
            str_size = serialTransfer.tx_obj(str_, send_size) - send_size
            send_size += str_size
            
            ###################################################################
            # Send a float
            ###################################################################
            float_ = 5.234
            float_size = serialTransfer.tx_obj(float_, send_size) - send_size
            send_size += float_size
            
            ###################################################################
            # Transmit all the data to send in a single packet
            ###################################################################
            serialTransfer.send(send_size)
            
            ###################################################################
            # Wait for a response and report any errors while receiving packets
            ###################################################################
            while not serialTransfer.available():
                # A negative value for status indicates an error
                if serialTransfer.status.value < 0:
                    if serialTransfer.status == txfer.Status.CRC_ERROR:
                        print('ERROR: CRC_ERROR')
                    elif serialTransfer.status == txfer.Status.PAYLOAD_ERROR:
                        print('ERROR: PAYLOAD_ERROR')
                    elif serialTransfer.status == txfer.Status.STOP_BYTE_ERROR:
                        print('ERROR: STOP_BYTE_ERROR')
                    else:
                        print('ERROR: {}'.format(serialTransfer.status.name))
            
            ###################################################################
            # Parse response list
            ###################################################################
            rec_list_  = serialTransfer.rx_obj(obj_type=type(list_),
                                     obj_byte_size=list_size,
                                     list_format='i')
            
            ###################################################################
            # Parse response string
            ###################################################################
            rec_str_   = serialTransfer.rx_obj(obj_type=type(str_),
                                     obj_byte_size=str_size,
                                     start_pos=list_size)
            
            ###################################################################
            # Parse response float
            ###################################################################
            rec_float_ = serialTransfer.rx_obj(obj_type=type(float_),
                                     obj_byte_size=float_size,
                                     start_pos=(list_size + str_size))
            
            ###################################################################
            # Display the received data
            ###################################################################
            print('SENT: {} {} {}'.format(list_, str_, float_))
            print('RCVD: {} {} {}'.format(rec_list_, rec_str_, rec_float_))
            print(' ')
    
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