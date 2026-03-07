import sys
from PyQt6.QtWidgets import QApplication
from gui import GCSWindow
from comm_hub import TelemetryThread

def main():
    app = QApplication(sys.argv)
    
    # 1. Create the GUI
    window = GCSWindow()
    window.show()
    
    # 2. Create the Backend Thread
    comms_thread = TelemetryThread(port='COM7')
    
    # 3. Connect Backend Signals to GUI Slots
    comms_thread.telemetry_received.connect(window.update_telemetry)
    comms_thread.status_received.connect(window.update_status)
    
    # 4. Start the background loop
    comms_thread.start()
    
    # 5. Run the application and ensure clean exit
    exit_code = app.exec()
    
    print("Shutting down cleanly...")
    comms_thread.stop()
    sys.exit(exit_code)

if __name__ == '__main__':
    main()