import sys
import random
from PyQt5.QtWidgets import QApplication, QWidget, QLabel, QVBoxLayout, QPushButton
from PyQt5.QtCore import Qt, pyqtSignal
from PyQt5.QtGui import QPainter, QPen
from colormath.color_objects import LabColor, sRGBColor
from colormath.color_conversions import convert_color

class LinearKnob(QWidget):
    valueChanged = pyqtSignal(float)

    def __init__(self, min_val, max_val, initial_val):
        super().__init__()
        self.min_val = min_val
        self.max_val = max_val
        self.value = initial_val
        self.setFixedSize(60, 200)
        self.setMouseTracking(True)
        self.setValue(initial_val)
        self.reactive_on_hover = True
        self.is_dragging = False

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setPen(QPen(Qt.black, 2))
        rect = self.rect().adjusted(10, 0, -10, 0)
        value_position = int(rect.height() - ((self.value - self.min_val) / (self.max_val - self.min_val) * rect.height()))
        painter.drawLine(rect.x(), value_position, rect.right(), value_position)

    def mousePressEvent(self, event):
        if not self.reactive_on_hover:
            self.is_dragging = True
            self._update_value(event.pos())

    def mouseReleaseEvent(self, event):
        if self.is_dragging:
            self.is_dragging = False

    def mouseMoveEvent(self, event):
        if self.reactive_on_hover or self.is_dragging:
            self._update_value(event.pos())

    def _update_value(self, pos):
        new_value = self.max_val - ((pos.y() / self.height()) * (self.max_val - self.min_val))
        new_value = max(self.min_val, min(self.max_val, new_value))
        if new_value != self.value:
            self.value = new_value
            self.update()
            self.valueChanged.emit(self.value)

    def setValue(self, value):
        self.value = max(self.min_val, min(self.max_val, value))
        self.update()

    def getValue(self):
        return self.value

    def set_reactive_on_hover(self, reactive):
        self.reactive_on_hover = reactive

class ColorDisplay(QWidget):
    def __init__(self):
        super().__init__()
        self.initUI()

    def initUI(self):
        self.l_knob = LinearKnob(min_val=0, max_val=100, initial_val=50)
        self.l_knob.valueChanged.connect(self.update_color)

        self.a_knob = LinearKnob(min_val=-128, max_val=127, initial_val=0)
        self.a_knob.valueChanged.connect(self.update_color)

        self.b_knob = LinearKnob(min_val=-128, max_val=127, initial_val=0)
        self.b_knob.valueChanged.connect(self.update_color)

        self.color_display = QLabel(self)
        self.color_display.setFixedSize(200, 200)

        self.lab_label = QLabel(self)
        self.hex_label = QLabel(self)

        self.randomize_button = QPushButton('Randomize', self)
        self.randomize_button.clicked.connect(self.randomize_knobs)

        self.magnet_button = QPushButton('Magnet: OFF', self)
        self.magnet_button.setCheckable(True)
        self.magnet_button.clicked.connect(self.toggle_magnet)

        layout = QVBoxLayout()
        layout.addWidget(QLabel("L* (0-100)"))
        layout.addWidget(self.l_knob)
        layout.addWidget(QLabel("a* (-128 to 127)"))
        layout.addWidget(self.a_knob)
        layout.addWidget(QLabel("b* (-128 to 127)"))
        layout.addWidget(self.b_knob)
        layout.addWidget(self.color_display)
        layout.addWidget(self.lab_label)
        layout.addWidget(self.hex_label)
        layout.addWidget(self.randomize_button)
        layout.addWidget(self.magnet_button)

        self.setLayout(layout)
        self.setWindowTitle('CIELAB Color Display with Knobs')
        self.setGeometry(100, 100, 250, 500)

        self.update_color()

    def update_color(self):
        try:
            L = self.l_knob.getValue()
            a = self.a_knob.getValue()
            b = self.b_knob.getValue()

            # Convert from Lab to RGB
            lab_color = LabColor(L, a, b)
            rgb_color = convert_color(lab_color, sRGBColor)

            # Use raw RGB values without clamping
            r = rgb_color.rgb_r * 255
            g = rgb_color.rgb_g * 255
            b = rgb_color.rgb_b * 255

            # Check for clipping and create HEX code
            if any(val < 0 or val > 255 for val in (r, g, b)):
                r_clamped = max(0, min(255, int(r)))
                g_clamped = max(0, min(255, int(g)))
                b_clamped = max(0, min(255, int(b)))

                hex_color = f"#{r_clamped:02X}{g_clamped:02X}{b_clamped:02X}"
                alpha_hex = f"{int(r) % 16:X}"  # Taking the last hex digit from raw RGB value

                # Convert hex digit to alpha value
                alpha_value = int(alpha_hex, 16) / 15.0  # Max alpha value is 15 in hex

                # Display color with alpha
                self.color_display.setStyleSheet(f'background-color: rgba({r_clamped}, {g_clamped}, {b_clamped}, {alpha_value});')

                # Display HEX and alpha value
                self.hex_label.setText(f"Hex: {hex_color}{alpha_hex} (Clipped, Alpha: {alpha_value:.2f})")
            else:
                # Normal color handling
                hex_color = f"#{int(r):02X}{int(g):02X}{int(b):02X}"
                self.color_display.setStyleSheet(f'background-color: rgb({int(r)}, {int(g)}, {int(b)});')
                self.hex_label.setText(f"Hex: {hex_color}")

            # Display Lab values
            self.lab_label.setText(f"L*: {L:.2f}, a*: {a:.2f}, b*: {b:.2f}")

        except ValueError:
            self.lab_label.setText("Invalid input. Please adjust the knobs.")
            self.hex_label.setText("Hex: Invalid")

    def randomize_knobs(self):
        l_value = random.uniform(0, 100)
        a_value = random.uniform(-128, 127)
        b_value = random.uniform(-128, 127)

        self.l_knob.setValue(l_value)
        self.a_knob.setValue(a_value)
        self.b_knob.setValue(b_value)

        self.update_color()

    def toggle_magnet(self):
        if self.magnet_button.isChecked():
            self.magnet_button.setText('Magnet: ON')
            self.l_knob.set_reactive_on_hover(True)
            self.a_knob.set_reactive_on_hover(True)
            self.b_knob.set_reactive_on_hover(True)
        else:
            self.magnet_button.setText('Magnet: OFF')
            self.l_knob.set_reactive_on_hover(False)
            self.a_knob.set_reactive_on_hover(False)
            self.b_knob.set_reactive_on_hover(False)

if __name__ == '__main__':
    app = QApplication(sys.argv)
    ex = ColorDisplay()
    ex.show()
    sys.exit(app.exec_())
