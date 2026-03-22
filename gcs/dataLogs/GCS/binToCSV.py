import csv
import ctypes
import sys
from pathlib import Path

# 1. Get the absolute path of the directory containing THIS script
current_dir = Path(__file__).resolve().parent 

# 2. Go up two levels to reach the 'gcs' folder
gcs_folder = current_dir.parent.parent

# 3. Add that folder to the Python search path
sys.path.append(str(gcs_folder))

import data_structs as ds 

PACKET_MAP = {
    0: ds.PackedDataStruct, 
    1: ds.CommandStruct,
    2: ds.StatusStruct 
}

def flatten_struct(obj):
    """Recursively extracts all fields from a ctypes Structure."""
    output = []
    for field_name, field_type in obj._fields_:
        value = getattr(obj, field_name)
        
        # If the value is another ctypes Structure, recurse into it
        if isinstance(value, ctypes.Structure):
            output.extend(flatten_struct(value))
        # If it's a basic type (int, float, etc.), just add it
        else:
            output.append(value)
    return output

def parse_bin_to_single_csv(bin_filepath):
    out_name = bin_filepath.replace('.bin', '.csv')
    
    with open(bin_filepath, 'rb') as f_in, open(out_name, 'w', newline='') as f_out:
        writer = csv.writer(f_out)
        
        while True:
            # 1. Read ID
            header = f_in.read(1)
            if not header:
                break 
            
            packet_id = int.from_bytes(header, 'little')
            
            if packet_id not in PACKET_MAP:
                print(f"Unknown ID {packet_id}. Aborting.")
                break
                
            struct_class = PACKET_MAP[packet_id]
            size = ctypes.sizeof(struct_class)
            
            # 2. Read Payload
            payload = f_in.read(size)
            if len(payload) < size:
                break 
                
            # 3. Unpack and write
            packet = struct_class.from_buffer_copy(payload)
            
            # Create a list starting with the ID, followed by all the struct values
            row_data = [packet_id] + flatten_struct(packet)
            writer.writerow(row_data)

    print(f"Parsing complete. Saved to {out_name}")

if __name__ == "__main__":
    currentfolder = 'C:/Users/david/Downloads/Personal/Projects/Github/TVC-EDF/gcs/datalogs/GCS/'
    bin_path = currentfolder + 'flight_0.bin'  # Update this path if your file is in a different location
    parse_bin_to_single_csv(bin_path)