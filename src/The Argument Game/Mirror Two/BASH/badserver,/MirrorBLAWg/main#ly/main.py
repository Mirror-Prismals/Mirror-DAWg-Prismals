#ive made some aesthetic creative decisions in the code
#dont make my system code smaller, only bigger
import tkinter as tk
import os
import sys
import time

# Add the path to the parent directory of the 'badserver' module to sys.path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'badserver')))

from badserver.badserver import BadServer

class FakeCLI:
    def __init__(self, root, create_frog_button, update_bash_window):
        self.root = root
        self.create_frog_button = create_frog_button
        self.update_bash_window = update_bash_window

        

    def open_cli_popup(self):
        self.cli_popup = tk.Toplevel(self.root)
        self.cli_popup.title("z`~")
        self.cli_popup.geometry("800x400")
        self.cli_popup.protocol("WM_DELETE_WINDOW", self.on_zr_close)  # Register callback for window close
        
        self.cli_text = tk.Text(self.cli_popup, bg="black", fg="white")
        self.cli_text.pack(expand=True, fill='both')
        
        self.cli_input = tk.Entry(self.cli_popup, bg="black", fg="white")
        self.cli_input.bind("<Return>", self.process_cli_command)
        self.cli_input.pack(fill='x')

    def process_cli_command(self, event):
        command = self.cli_input.get().strip()
        if command == "z":
            self.save_to_file()
        elif command == "x":
            self.load_from_file()
        else:
            self.cli_text.insert(tk.END, f"> {command}\n")
        self.cli_input.delete(0, tk.END)
        self.cli_text.see(tk.END)

    def save_to_file(self):
        with open("zr_content.txt", "w") as file:
            file.write(self.cli_text.get("1.0", tk.END))

    def load_from_file(self):
        try:
            with open("zr_content.txt", "r") as file:
                content = file.read()
                self.cli_text.delete("1.0", tk.END)
                self.cli_text.insert(tk.END, content)
        except FileNotFoundError:
            pass

    def on_zr_close(self):
        # Save the contents of the CLI before closing
        self.save_to_file()
        self.cli_popup.destroy()  # Destroy the CLI popup window

    def open_password_popup(self):
        # Embedded password box within the main fullscreen window
        self.password_label = tk.Label(self.root, text="Hello", bg="black", fg="white")
        self.password_label.place(relx=0.5, rely=0.8, anchor=tk.CENTER)
        
        self.password_input = tk.Entry(self.root, show="*", bg="black", fg="white")
        self.password_input.bind("<Return>", self.submit_password)
        self.password_input.place(relx=0.5, rely=0.8, anchor=tk.CENTER)

def submit_password(self, event):
    password = self.password_input.get()
    if password == "zx":
        if self.cli_popup.winfo_exists():  # Check if the CLI window exists
            # If the window is open, save its contents to 'zx.txt'
            with open("zx.txt", "w") as file:
                file.write(self.cli_text.get("1.0", tk.END))
            self.cli_popup.destroy()  # Close the CLI popup window
        else:
            # If the window is not open, simulate opening the CLI window
            self.open_cli_popup()
        self.update_bash_window("Password accepted: zx", color="blue")
    elif password == "#00FF00FF00FF":
        self.update_bash_window("Invalid password: #00FF00FF00FF", color="red")
    else:
        self.update_bash_window(f"Invalid password: {password}", color="red")
    print(f"Password entered: {password}")  # Replace with actual password handling logic


    def load_zr_content(self):
        zr_content = ""
        try:
            with open("zr_content.txt", "r") as file:
                zr_content = file.read()
        except FileNotFoundError:
            pass
        return zr_content

    def save_zr_content(self):
        zr_content = self.cli_text.get(1.0, tk.END)
        with open("zr_content.txt", "w") as file:
            file.write(zr_content)

    def simulate_zr_click(self):
        # Simulate click on the "zr" button
        self.open_cli_popup()

class iA:
    def __init__(self, root, target_button_func):
        self.root = root
        self.target_button_func = target_button_func

    def simulate_button_click(self):
        # Simulate a button click after a delay
        self.root.after(1000, self.target_button_func)

class ZX:
    def __init__(self, root):
        self.root = root

    def open_cli_popup(self):

        self.cli_popup.protocol("WM_DELETE_WINDOW", self.on_zr_close)  # Register callback for window close
        
        self.cli_text = tk.Text(self.cli_popup, bg="black", fg="white")
        self.cli_text.pack(expand=True, fill='both')
        
        self.cli_input = tk.Entry(self.cli_popup, bg="black", fg="white")
        self.cli_input.bind("<Return>", self.process_cli_command)
        self.cli_input.pack(fill='x')

    def process_cli_command(self, event):
        command = self.cli_input.get().strip()
        if command == "z":
            self.save_to_file()
        elif command == "x":
            self.load_from_file()
        else:
            self.cli_text.insert(tk.END, f"> {command}\n")
        self.cli_input.delete(0, tk.END)
        self.cli_text.see(tk.END)

    def save_to_file(self):
        with open("zr.txt", "w") as file:
            file.write(self.cli_text.get("1.0", tk.END))

    def load_from_file(self):
        try:
            with open("zr.txt", "r") as file:
                content = file.read()
                self.cli_text.delete("1.0", tk.END)
                self.cli_text.insert(tk.END, content)
        except FileNotFoundError:
            pass

    def on_zr_close(self):
        # Save the contents of the CLI before closing
        self.save_to_file()
        self.cli_popup.destroy()  # Destroy the CLI popup window

class DawGame:
    def __init__(self):
        self.server_root = tk.Tk()
        self.server_root.title("Magenta And Lime")
        self.server_root.attributes('-fullscreen', True)  # Set window to fullscreen
        self.server_root.configure(bg="black")  # Set background to black

        self.canvas = tk.Canvas(self.server_root, bg="black", highlightthickness=0)
        self.canvas.pack(fill="both", expand=True)

        self.cli = FakeCLI(self.server_root, self.create_frog_button, self.update_bash_window)
        self.zx = ZX(self.server_root)
        self.bad_server = BadServer()
        self.ia = iA(self.server_root, self.cli.open_cli_popup)  # Initialize the intelligent automator

        self.create_widgets()
        self.lift_widgets()

    def create_widgets(self):
        # Create buttons to open popups
        self.cli_button = tk.Button(self.server_root, text=",,          ,,          ,,          ..;;[[//'']]..;;[[//'']]..;;[[//'']]zr..;;[[//'']]..;;[[//'']]                        ,,          ,,          ,,          ", command=self.zx.open_cli_popup)
        self.cli_button.place(relx=0.5, rely=0.99, anchor=tk.CENTER)
        
        self.password_button = tk.Button(self.server_root, text="                                                           ;                    ;          1          ;          2          ;          3          ;          1          ;B#A#S#H,,??##!!          ,'          ,'          ,'          ,'           ,'          ,'          ,'          ,'          ,'          ,'          ,'           ,'          ,'          ,'", command=self.cli.open_password_popup)
        self.password_button.place(relx=0.5, rely=0.01, anchor=tk.CENTER)

        self.bash_button = tk.Button(self.server_root, text='''           
                                     
                                     
                                     
                                                                        bash  ,,..;;[[//'']]
                                     
                                     
                                     
                                     ''', command=self.open_bash_window)
        self.bash_button.place(relx=0.8, rely=0.8, anchor=tk.CENTER)

        self.bash_text = tk.Text(self.server_root, bg="black", fg="white", height=10, width=50)
        self.bash_text.place(relx=0.5, rely=0.9, anchor=tk.CENTER)

    def open_bash_window(self):
        pass  # Not required since bash window will update when password is submitted

    def update_bash_window(self, text, color="white"):
        self.bash_text.config(state=tk.NORMAL)
        self.bash_text.insert(tk.END, f"{text}\n")
        self.bash_text.config(state=tk.DISABLED)
        self.bash_text.see(tk.END)
        self.bash_text.configure(bg=color)

        # Change background colors based on password
        if text == "#00FF00FF00FF":
            self.canvas.create_polygon(0, 0, self.server_root.winfo_width(), 0, 0, self.server_root.winfo_height() // 2, fill="lime", outline="")
            self.canvas.create_polygon(self.server_root.winfo_width(), self.server_root.winfo_height(), self.server_root.winfo_width(), self.server_root.winfo_height() // 2, 0, self.server_root.winfo_height(), fill="magenta", outline="")
        else:
            self.server_root.configure(bg="red")
           
    def create_frog_button(self):
        self.frog_button = tk.Button(self.server_root, text="Frog Level", command=self.frog_level)
        self.frog_button.place(relx=0.1, rely=0.1, anchor=tk.CENTER)

    def frog_level(self):
        # Placeholder for frog level functionality
        pass

    def lift_widgets(self):
        self.cli_button.lift()
        self.password_button.lift()

if __name__ == "__main__":
    game = DawGame()
    game.server_root.mainloop()
