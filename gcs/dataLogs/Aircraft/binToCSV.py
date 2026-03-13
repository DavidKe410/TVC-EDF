import struct
import csv

# Format mapping based on your specific variable list
# I=uint32, h=int16, f=float, ?=bool etc
STRUCT_FORMAT = '<I b I f f f I f f f f b B b'
HEADERS = [
    'overall_time', 'state', 'accel_time', 
    'accel_x', 'accel_y', 'accel_z', 
    'orien_time', 'real', 'i', 'j', 'k', 
    'temp', 'new_accel', 'orien_cali_status'
]

STRUCT_SIZE = struct.calcsize(STRUCT_FORMAT)

currentfolder = 'C:/Users/david/Downloads/Personal/Projects/Github/TVC-EDF/TVC_EDF/src/datalogs/'
bin_path = currentfolder + 'FLIGHT1.bin'  # Update this path if your file is in a different location
csv_path = currentfolder + 'flight_telemetry1.csv'  # Desired output CSV file name

def convert_bin_to_csv():
    with open(bin_path, 'rb') as f_bin, open(csv_path, 'w', newline='') as f_csv:
        writer = csv.writer(f_csv)
        writer.writerow(HEADERS)
        
        while True:
            chunk = f_bin.read(STRUCT_SIZE)
            if len(chunk) < STRUCT_SIZE:
                break
            
            row = struct.unpack(STRUCT_FORMAT, chunk)
            writer.writerow(row)

if __name__ == "__main__":
    # If your file is in a folder on your PC, update this path
    convert_bin_to_csv()
    print("Done!")