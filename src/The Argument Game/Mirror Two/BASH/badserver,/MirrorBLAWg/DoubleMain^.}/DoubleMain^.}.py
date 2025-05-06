import os
import subprocess
from bash_double_main import BashDoubleMain

class DoubleMain:
    def __init__(self, base_dir):
        self.base_dir = base_dir
        self.bash_double_main = BashDoubleMain(base_dir)

    def run_bash_scripts(self):
        self.bash_double_main.run_scripts()

    def hello_from_doublemain(self):
        print("Hello from DoubleMain}*^!")

    def run(self):
        self.hello_from_doublemain()
        self.run_bash_scripts()

if __name__ == "__main__":
    base_dir = os.path.dirname(os.path.abspath(__file__))  # Base directory to load the modules from
    double_main = DoubleMain(base_dir)
    double_main.run()
