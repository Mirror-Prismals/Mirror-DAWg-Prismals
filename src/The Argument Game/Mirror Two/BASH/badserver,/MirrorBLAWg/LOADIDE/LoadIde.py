import os
import glob

class LoadIDE:
    def __init__(self, directory="."):
        self.directory = directory

    def get_all_txt_files_content(self):
        txt_files = sorted(glob.glob(os.path.join(self.directory, "*.txt")), key=os.path.getmtime)
        all_content = ""
        for file_path in txt_files:
            with open(file_path, "r") as file:
                all_content += file.read() + "\n"
        return all_content

    def send_to_ide(self):
        from fake_ide import FakeIDE
        import tkinter as tk

        # Get the content of all .txt files
        content = self.get_all_txt_files_content()

        # Open the Fake IDE with the collected content
        root = tk.Tk()
        ide = FakeIDE(root)
        ide.display_content(content)
        root.mainloop()

if __name__ == "__main__":
    ide_loader = LoadIDE()
    ide_loader.send_to_ide()
