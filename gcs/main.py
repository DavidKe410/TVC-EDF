import sys
from PyQt6.QtWidgets import QApplication
from gui import GCSWindow

def main():
    app = QApplication(sys.argv)
    
    # create the GUI
    window = GCSWindow()
    window.show()
    
    sys.exit(app.exec())

if __name__ == '__main__':
    main()