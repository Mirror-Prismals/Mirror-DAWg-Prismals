import os
import importlib.util

class BashDoubleMain:
    def __init__(self, base_dir):
        self.base_dir = base_dir
        self.modules = []

    def load_modules(self):
        for root, _, files in os.walk(self.base_dir):
            for file in files:
                if file.endswith(".py") and file != "bash_doublemain.py":
                    file_path = os.path.join(root, file)
                    module_name = os.path.relpath(file_path, self.base_dir).replace("/", ".").replace("\\", ".").rstrip(".py")
                    self.import_module(file_path, module_name)

    def import_module(self, file_path, module_name):
        spec = importlib.util.spec_from_file_location(module_name, file_path)
        if spec and spec.loader:
            module = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(module)
            self.modules.append(module)
            print(f"Loaded module: {module_name}")

    def execute_modules(self):
        for module in self.modules:
            if hasattr(module, 'main'):
                print(f"Executing main() in module: {module.__name__}")
                module.main()

    def run(self):
        self.load_modules()
        self.execute_modules()

if __name__ == "__main__":
    base_dir = os.path.dirname(__file__)  # Base directory to load the modules from
    bash_double_main = BashDoubleMain(base_dir)
    bash_double_main.run()
