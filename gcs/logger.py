import time
from pathlib import Path
from queue import Queue
from PyQt6.QtCore import QThread
import data_structs as ds

class LoggerThread(QThread):
    def __init__(self, base_name="flight"):
        super().__init__()
        self.queue = Queue() # A thread-safe FIFO buffer
        self.running = True
        self.base_name = base_name
        self.file_name = ""
        self.logging_enabled = False
        self.file = None

    def _generate_name(self, base):
        file_location = "gcs/datalogs/GCS/"
        increment = 0
        while Path(f"{file_location}{base}_{increment}.bin").is_file():
            increment += 1
        return f"{file_location}{base}_{increment}.bin"

    def log(self, packet_type, data_obj):
        """Called by other threads to 'submit' data for logging."""
        # We store as a tuple: (ID, binary_data)
        data_tuple = (packet_type, bytes(data_obj))
        self.queue.put(data_tuple)

    def run(self):
        while self.running or not self.queue.empty():

            if self.logging_enabled and self.file is None:
                self.file_name = self._generate_name(self.base_name)
                self.file = open(self.file_name, "ab", buffering=0)
                
            elif not self.logging_enabled and self.file is not None:
                self.file.close()
                self.file = None

            if not self.queue.empty() and self.file is not None:
                packet_id, data = self.queue.get()
                if self.file: # Only write if we have an active file
                    self.file.write(packet_id.to_bytes(1, 'little'))
                    self.file.write(data)
                self.queue.task_done()
            else:
                time.sleep(0.001)

    def stop(self):
        self.running = False
        self.queue.join()
        self.wait()