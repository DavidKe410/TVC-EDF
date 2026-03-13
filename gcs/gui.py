from PyQt6.QtWidgets import QMainWindow, QWidget, QVBoxLayout, QLabel, QPushButton
from PyQt6.QtCore import pyqtSlot
from comms import CommThread
from logger import LoggerThread

class GCSWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("TVC EDF Ground Control Station V0")
        self.resize(1200, 800)

        # Main Layout Setup
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        layout = QVBoxLayout(central_widget)

        # UI Elements
        self.lbl_telemetry = QLabel("Waiting for telemetry...")
        self.lbl_status = QLabel("Status: Disconnected")
        
        
        self.log_btn_toggle = QPushButton("Press to start logging.")
        self.log_btn_toggle.setCheckable(True)
        self.log_btn_toggle.setChecked(True)
        self.log_btn_toggle.clicked.connect(self.on_log_toggle)
        
        layout.addWidget(self.log_btn_toggle)
        layout.addWidget(self.lbl_status)
        layout.addWidget(self.lbl_telemetry)

        # Threads
        self.log_thread = LoggerThread()
        self.comms_thread = CommThread(port='COM7', logger=self.log_thread)
        
        # 3. Connect Backend Signals to GUI Slots
        self.comms_thread.telemetry_received.connect(self.update_telemetry)
        self.comms_thread.status_received.connect(self.update_status)
        
        # 4. Start the background loops
        self.log_thread.start()
        self.comms_thread.start()

    @pyqtSlot(object)
    def update_telemetry(self, data):
        """Triggered automatically when the backend emits new telemetry."""
        # Example: Update label with acceleration data
        text = f"Accel X: {data.accel_x:.4f} | Accel Y: {data.accel_y:.4f} | Accel Z: {data.accel_z:.4f}"
        self.lbl_telemetry.setText(text)

    @pyqtSlot(object)
    def update_status(self, status):
        """Triggered automatically when the backend emits new status."""
        text = f"Teensy State: {status.teensy_status.ac_state} | ESP GCS Temp: {status.esp_gcs_status.temperature}"
        self.lbl_status.setText(text)

    def on_log_toggle(self):
        if self.log_btn_toggle.isChecked(): # start checked, so when we actually press the button, it toggles it off and starts the log
            self.log_thread.logging_enabled = False
            self.log_btn_toggle.setText("Press to start logging.")
        else:
            self.log_thread.logging_enabled = True
            self.log_btn_toggle.setText("Press to stop logging.")
    
    def closeEvent(self, event):
        if self.comms_thread:
            self.comms_thread.stop()
        if self.log_thread:
            self.log_thread.stop() # Ensures queue.join() runs
        return super().closeEvent(event)