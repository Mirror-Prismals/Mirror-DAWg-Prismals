import sys
import json
from PyQt5.QtWidgets import (QApplication, QWidget, QLabel, QVBoxLayout, QLineEdit, QMessageBox,
                             QPushButton, QFormLayout, QDialog, QDialogButtonBox, QTextEdit,
                             QFileDialog, QComboBox)
from PyQt5.QtGui import QColor

class ProfileDialog(QDialog):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle("Create Profile")
        layout = QFormLayout(self)

        self.profile_name = QLineEdit(self)
        layout.addRow("Profile Name:", self.profile_name)

        self.channel_order = QLineEdit(self)
        self.channel_order.setPlaceholderText("e.g., RRGGBBYY")
        layout.addRow("Channel Order:", self.channel_order)

        self.channel_definitions = QTextEdit(self)
        self.channel_definitions.setPlaceholderText("e.g., RR=FF0000,GG=00FF00,BB=0000FF,YY=FFFF00")
        self.channel_definitions.setMinimumHeight(100)
        layout.addRow("Channel Definitions:", self.channel_definitions)

        buttons = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel, self)
        buttons.accepted.connect(self.accept)
        buttons.rejected.connect(self.reject)
        layout.addRow(buttons)

    def get_profile(self):
        return {
            'name': self.profile_name.text(),
            'channels': self.parse_channels(self.channel_order.text(), self.channel_definitions.toPlainText())
        }

    def parse_channels(self, order, definitions):
        channels = []
        order = order.upper()
        def_dict = self.parse_definitions(definitions)
        for i in range(0, len(order), 2):
            code = order[i:i+2]
            if code in def_dict:
                channels.append({
                    'code': code,
                    'name': f"Channel {code}",  # You can modify this if you want to include custom names
                    'color': def_dict[code]
                })
        return channels

    def parse_definitions(self, text):
        definitions = {}
        pairs = text.replace('\n', '').split(',')
        for pair in pairs:
            if '=' in pair:
                key, value = pair.strip().split('=')
                definitions[key.strip().upper()] = f"#{value.strip().upper()}"
        return definitions

    def accept(self):
        order = self.channel_order.text().upper()
        channels = self.parse_channels(order, self.channel_definitions.toPlainText())
        if len(channels) == len(order) // 2:
            super().accept()
        else:
            QMessageBox.warning(self, "Invalid Input", "Channel order and definitions do not match.")

class OpenColorFormat(QWidget):
    def __init__(self):
        super().__init__()
        self.profiles = {}
        self.current_profile = None
        self.initUI()

    def initUI(self):
        layout = QVBoxLayout()

        self.profile_label = QLabel("No profile loaded")
        layout.addWidget(self.profile_label)

        self.profile_combo = QComboBox(self)
        self.profile_combo.currentTextChanged.connect(self.change_profile)
        layout.addWidget(self.profile_combo)

        self.color_input = QLineEdit(self)
        self.color_input.setPlaceholderText("Enter color code")
        layout.addWidget(self.color_input)

        self.calc_button = QPushButton("Calculate Color", self)
        self.calc_button.clicked.connect(self.calculate_color)
        layout.addWidget(self.calc_button)

        self.result_label = QLabel()
        layout.addWidget(self.result_label)

        self.color_display = QLabel()
        self.color_display.setFixedSize(100, 100)
        self.color_display.setStyleSheet("border: 1px solid black;")
        layout.addWidget(self.color_display)

        self.create_profile_button = QPushButton("Create Profile", self)
        self.create_profile_button.clicked.connect(self.create_profile)
        layout.addWidget(self.create_profile_button)

        self.save_profile_button = QPushButton("Save Profiles", self)
        self.save_profile_button.clicked.connect(self.save_profiles)
        layout.addWidget(self.save_profile_button)

        self.load_profile_button = QPushButton("Load Profiles", self)
        self.load_profile_button.clicked.connect(self.load_profiles)
        layout.addWidget(self.load_profile_button)

        self.setLayout(layout)
        self.setWindowTitle('Open Color Format')
        self.setGeometry(300, 300, 300, 400)

    def change_profile(self, profile_name):
        if profile_name in self.profiles:
            self.current_profile = self.profiles[profile_name]
            self.profile_label.setText(f"Current Profile: {self.current_profile['name']}")
            print(f"Changed to profile: {profile_name}")
            print("Profile structure:", self.current_profile)
        else:
            print(f"Profile not found: {profile_name}")

    def create_profile(self):
        dialog = ProfileDialog(self)
        if dialog.exec_():
            new_profile = dialog.get_profile()
            self.profiles[new_profile['name']] = new_profile
            self.current_profile = new_profile
            self.profile_label.setText(f"Current Profile: {self.current_profile['name']}")
            self.profile_combo.addItem(new_profile['name'])
            self.profile_combo.setCurrentText(new_profile['name'])
            print("New profile created:", new_profile)

    def save_profiles(self):
        file_name, _ = QFileDialog.getSaveFileName(self, "Save Profiles", "", "JSON Files (*.json)")
        if file_name:
            with open(file_name, 'w') as f:
                json.dump(self.profiles, f, indent=2)
            QMessageBox.information(self, "Success", "Profiles saved successfully.")

    def load_profiles(self):
        file_name, _ = QFileDialog.getOpenFileName(self, "Load Profiles", "", "JSON Files (*.json)")
        if file_name:
            try:
                with open(file_name, 'r') as f:
                    self.profiles = json.load(f)
                if self.profiles:
                    self.profile_combo.clear()
                    self.profile_combo.addItems(self.profiles.keys())
                    first_profile = next(iter(self.profiles.values()))
                    self.current_profile = first_profile
                    self.profile_label.setText(f"Current Profile: {self.current_profile['name']}")
                    self.profile_combo.setCurrentText(self.current_profile['name'])
                    QMessageBox.information(self, "Success", "Profiles loaded successfully.")
                    print("Loaded profiles:", self.profiles)
                else:
                    QMessageBox.warning(self, "Warning", "No profiles found in the file.")
            except json.JSONDecodeError:
                QMessageBox.critical(self, "Error", "Invalid JSON file.")
            except Exception as e:
                QMessageBox.critical(self, "Error", f"An error occurred: {str(e)}")

    def calculate_color(self):
        if not self.current_profile:
            self.result_label.setText("Error: No profile loaded")
            return

        print("Current profile:", self.current_profile)

        if 'channels' not in self.current_profile:
            self.result_label.setText("Error: Invalid profile structure")
            return

        input_code = self.color_input.text().upper()
        if input_code.startswith('#'):
            input_code = input_code[1:]  # Remove the '#' if present

        channels = self.current_profile['channels']
        channel_order = ''.join(channel['code'] for channel in channels)
        
        print("Channel order:", channel_order)
        print("Input code:", input_code)

        if len(input_code) != len(channel_order):
            self.result_label.setText(f"Error: Input length ({len(input_code)}) doesn't match profile channel order length ({len(channel_order)})")
            return

        color_values = [0, 0, 0]
        for i, channel in enumerate(channels):
            channel_code = channel['code']
            channel_value = int(input_code[i*2:i*2+2], 16) / 255.0
            channel_color = QColor(channel['color'])
            color_values[0] += channel_color.redF() * channel_value
            color_values[1] += channel_color.greenF() * channel_value
            color_values[2] += channel_color.blueF() * channel_value

        final_color = QColor.fromRgbF(
            min(1.0, color_values[0]),
            min(1.0, color_values[1]),
            min(1.0, color_values[2])
        )

        hex_result = final_color.name().upper()
        self.result_label.setText(f"Calculated Color: {hex_result}")
        self.color_display.setStyleSheet(f"background-color: {hex_result}; border: 1px solid black;")

if __name__ == '__main__':
    app = QApplication(sys.argv)
    ex = OpenColorFormat()
    ex.show()
    sys.exit(app.exec_())
