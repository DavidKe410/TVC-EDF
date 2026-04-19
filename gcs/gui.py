from PyQt6.QtWidgets import QMainWindow, QWidget, QVBoxLayout, QLabel, QPushButton, QTextEdit
from PyQt6.QtCore import pyqtSlot
from full_py_gui_V0 import Ui_MainWindow
from comms import CommThread
from logger import LoggerThread
import time

class GCSWindow(QMainWindow):
    def __init__(self):
        super().__init__()

        # Main Layout Setup
        self.ui = Ui_MainWindow()
        self.ui.setupUi(self)
        self.connect_buttons()

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

    def connect_buttons(self):
        self.ui.serial_conn_btn.clicked.connect(self.on_serial_toggle)
        self.ui.logger_btn.clicked.connect(self.on_log_toggle)
        self.ui.resend_cmd_btn.clicked.connect(self.on_resend_cmd)
        self.ui.manual_ctrl_btn.clicked.connect(self.on_manual_ctrl)


    @pyqtSlot(object)
    def update_telemetry(self, data):
        """Triggered automatically when the backend emits new telemetry."""
        # Example: Update label with acceleration data
        text = f"Accel X: {data.accel_x:.4f} | Accel Y: {data.accel_y:.4f} | Accel Z: {data.accel_z:.4f}"
        self.ui.raw_tele_line.setText(text)

    @pyqtSlot(object)
    def update_status(self, status):
        """Triggered automatically when the backend emits new status."""
        text = f"Teensy State: {status.teensy_status.ac_state} | ESP GCS Temp: {status.esp_gcs_status.temperature}"
        self.ui.lbl_status.setText(text)
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

        self.ui.cmds_list.setText(formated_cmds)

    @pyqtSlot(str)
    def update_serial_conn(self, error): # the enable flags should only ever be changed here, any other setters in the threads themselvs should jsut be reconfirming
        print(f"Serial connection lost: {error}")
        if self.comms_thread.serial_enabled == True:
            self.ui.serial_conn_btn.click() # now the serial_enabled is just reconfirmed on this end and button text back to correct msg
            self.ui.serial_conn_btn.setText("Serial Connection Lost: Device not found. Please try again.")
        if self.log_thread.logging_enabled == True:
            self.ui.logger_btn.click() # same as abv
            self.ui.logger_btn.setText("Serial disconnected. Please try again.")

    def on_log_toggle(self):
        if self.log_thread.logging_enabled == True:
            self.log_thread.logging_enabled = False
            self.ui.logger_btn.setText("Press to start logging.")
        elif self.log_thread.logging_enabled == False and self.comms_thread.serial_enabled == True:
            self.log_thread.logging_enabled = True
            self.ui.logger_btn.setText("Press to stop logging.")
        else: # covers when logging_enabled == False and serial_enabled == False
            self.ui.logger_btn.setText("Serial not connected. Please try again.")
            self.ui.logger_btn.setChecked(not self.ui.logger_btn.isChecked()) # may get rid of checkable option and just manually change bkgrnd color if possible

    def on_serial_toggle(self):
        if self.comms_thread.serial_enabled == True:
            self.comms_thread.serial_enabled = False
            self.ui.serial_conn_btn.setText("Press to connect serial.")
            if self.log_thread.logging_enabled == True: #auto shutoff logging if serial is disconnected
                self.ui.logger_btn.click() 
                self.ui.logger_btn.setText("Serial disconnected. Please try again.")
        else:
            self.comms_thread.serial_enabled = True # just need to switch this flag, the comms thread will automatically handle the connection function
            self.ui.serial_conn_btn.setText("Press to disconnect serial.")

    def on_resend_cmd(self):
        if self.comms_thread.serial_enabled == True:
            if self.comms_thread.resend_cmd(ind=(len(self.comms_thread.previous_cmds)-1)):
                self.ui.resend_cmd_btn.setText("Resend most recent command.")
            else:
                self.ui.resend_cmd_btn.setText("Invalid index. Try sending another command")
        else:
            self.ui.resend_cmd_btn.setText("Bruh, connect serial first")

    def on_manual_ctrl(self):
        if self.comms_thread.serial_enabled == True:
            if self.comms_thread.manual_ctrl:
                self.ui.manual_ctrl_btn.setText("Activate Manual Control.")
            else:
                self.ui.manual_ctrl_btn.setText("Manual Control Enabled. Press to deactivate manual control.")
            self.comms_thread.switch_manual_ctrl()
        else:
            self.ui.resend_cmd_btn.setText("Bruh, connect serial first")
    
    def closeEvent(self, event):
        if self.comms_thread:
            self.comms_thread.stop()
        if self.log_thread:
            self.log_thread.stop() # Ensures queue.join() runs
        return super().closeEvent(event)