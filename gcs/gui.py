from PyQt6.QtWidgets import QMainWindow, QWidget, QVBoxLayout, QLabel, QPushButton
from PyQt6.QtCore import pyqtSlot

class GCSWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("EDF Ground Control Station")
        self.resize(800, 600)

        # Main Layout Setup
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        layout = QVBoxLayout(central_widget)

        # Placeholder UI Elements
        self.lbl_telemetry = QLabel("Waiting for telemetry...")
        self.lbl_status = QLabel("Status: Disconnected")
        self.btn_connect = QPushButton("Connect / Disconnect (Placeholder)")
        
        layout.addWidget(self.btn_connect)
        layout.addWidget(self.lbl_status)
        layout.addWidget(self.lbl_telemetry)

    @pyqtSlot(object)
    def update_telemetry(self, data):
        """Triggered automatically when the backend emits new telemetry."""
        # Example: Update label with acceleration data
        text = f"Accel X: {data.accel_x:.2f} | Accel Y: {data.accel_y:.2f} | Accel Z: {data.accel_z:.2f}"
        self.lbl_telemetry.setText(text)

    @pyqtSlot(object)
    def update_status(self, status):
        """Triggered automatically when the backend emits new status."""
        text = f"Teensy State: {status.teensy_status.ac_state} | ESP GCS RSSI: {status.esp_gcs_status.RSSI}"
        self.lbl_status.setText(text)