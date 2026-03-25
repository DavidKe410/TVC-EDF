from PyQt6.QtWidgets import QMainWindow, QWidget, QVBoxLayout, QLabel, QPushButton, QTextEdit
from PyQt6.QtCore import pyqtSlot
from comms import CommThread
from logger import LoggerThread
import time

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
        
        self.serial_btn_toggle = QPushButton("Testing \n Press to connect serial.")
        self.serial_btn_toggle.setCheckable(True)
        self.serial_btn_toggle.clicked.connect(self.on_serial_toggle)
        
        self.log_btn_toggle = QPushButton("Press to start logging.")
        self.log_btn_toggle.setCheckable(True)
        self.log_btn_toggle.clicked.connect(self.on_log_toggle)

        self.lbl_commands_sent = QTextEdit()
        #self.text_edit.setPlainText()

        self.resend_cmd_btn = QPushButton("Resend most recent command.")
        self.resend_cmd_btn.clicked.connect(self.on_resend_cmd)

        self.manual_ctrl_btn = QPushButton("Activate Manual Control.")
        self.manual_ctrl_btn.clicked.connect(self.on_manual_ctrl)
        
        layout.addWidget(self.serial_btn_toggle)
        layout.addWidget(self.log_btn_toggle)
        layout.addWidget(self.lbl_status)
        layout.addWidget(self.lbl_telemetry)
        layout.addWidget(self.lbl_commands_sent)
        layout.addWidget(self.resend_cmd_btn)
        layout.addWidget(self.manual_ctrl_btn)

        # Threads
        self.log_thread = LoggerThread()
        self.comms_thread = CommThread(port='COM7', logger=self.log_thread)
        
        # 3. Connect Backend Signals to GUI Slots
        self.comms_thread.telemetry_received.connect(self.update_telemetry)
        self.comms_thread.status_received.connect(self.update_status)
        self.comms_thread.connection_error.connect(self.update_serial_conn)
        
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
        formated_cmds = ""

        for row in self.comms_thread.previous_cmds:
            cmd_obj = row[0]
            if row[1] != -1:
                time_diff = time.time() - row[1]
            else:
                time_diff = -1
            # 1. Extract values from the struct fields
            fields_str = ", ".join([f"{name}: {getattr(cmd_obj, name)}" for name, _ in cmd_obj._fields_])
            # 2. Format the time and combine
            formated_cmds += f"[{time_diff:.1f}s ago]      {fields_str}\n"

        self.lbl_commands_sent.setText(formated_cmds)

    @pyqtSlot(str)
    def update_serial_conn(self, error): # the enable flags should only ever be changed here, any other setters in the threads themselvs should jsut be reconfirming
        print(f"Serial connection lost: {error}")
        if self.comms_thread.serial_enabled == True:
            self.serial_btn_toggle.click() # now the serial_enabled is just reconfirmed on this end and button text back to correct msg
            self.serial_btn_toggle.setText("Serial Connection Lost: Device not found. Please try again.")
        if self.log_thread.logging_enabled == True:
            self.log_btn_toggle.click() # same as abv
            self.log_btn_toggle.setText("Serial disconnected. Please try again.")

    def on_log_toggle(self):
        if self.log_thread.logging_enabled == True:
            self.log_thread.logging_enabled = False
            self.log_btn_toggle.setText("Press to start logging.")
        elif self.log_thread.logging_enabled == False and self.comms_thread.serial_enabled == True:
            self.log_thread.logging_enabled = True
            self.log_btn_toggle.setText("Press to stop logging.")
        else: # covers when logging_enabled == False and serial_enabled == False
            self.log_btn_toggle.setText("Serial not connected. Please try again.")
            self.log_btn_toggle.setChecked(not self.log_btn_toggle.isChecked()) # may get rid of checkable option and just manually change bkgrnd color if possible

    def on_serial_toggle(self):
        if self.comms_thread.serial_enabled == True:
            self.comms_thread.serial_enabled = False
            self.serial_btn_toggle.setText("Press to connect serial.")
            if self.log_thread.logging_enabled == True: #auto shutoff logging if serial is disconnected
                self.log_btn_toggle.click() 
                self.log_btn_toggle.setText("Serial disconnected. Please try again.")
        else:
            self.comms_thread.serial_enabled = True # just need to switch this flag, the comms thread will automatically handle the connection function
            self.serial_btn_toggle.setText("Press to disconnect serial.")

    def on_resend_cmd(self):
        if self.comms_thread.serial_enabled == True:
            self.comms_thread.resend_cmd(ind=self.comms_thread.command.cmd_ID)
            self.resend_cmd_btn.setText("Resend most recent command.")
        else:
            self.resend_cmd_btn.setText("Bruh, connect serial first")

    def on_manual_ctrl(self):
        if self.comms_thread.manual_ctrl:
            self.comms_thread.manual_ctrl = False
            self.manual_ctrl_btn.setText("Activate Manual Control.")
        else:
            self.comms_thread.manual_ctrl = True
            self.manual_ctrl_btn.setText("Manual Control Enabled. Press to deactivate manual control.")
    
    def closeEvent(self, event):
        if self.comms_thread:
            self.comms_thread.stop()
        if self.log_thread:
            self.log_thread.stop() # Ensures queue.join() runs
        return super().closeEvent(event)