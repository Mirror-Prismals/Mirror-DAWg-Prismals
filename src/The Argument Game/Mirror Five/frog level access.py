import tkinter as tk

class Password:
    def __init__(self, root, network):
        self.root = root
        self.network = network

        self.root.title("miA")
        self.root.attributes('-fullscreen', True)  # Set window to fullscreen
        self.root.configure(bg="#0800FF")

        self.create_password_entry()

    def create_password_entry(self):
        self.entry = tk.Entry(self.root, show='*', font=('Helvetica', 24))
        self.entry.pack(pady=20)

        self.submit_button = tk.Button(self.root, text="Unlock", command=self.check_password, font=('Helvetica', 24))
        self.submit_button.pack(pady=10)

        self.message_label = tk.Label(self.root, text="", font=('Helvetica', 24), bg="#0800FF", fg="white")
        self.message_label.pack(pady=20)

    def check_password(self):
        password = self.entry.get()
        self.unlock_server(password)

    def unlock_server(self, password):
        responses = {
            "mira": "hi",
            "mia": "i dont think so",
            "#00FF00FF00FF": "frog level: server on"
        }
        response = responses.get(password, "Incorrect password!")
        self.message_label.config(text=response)

# Mock network class for demonstration
class MockNetwork:
    class MockBadServer:
        def change_text(self, text):
            print(text)

    def __init__(self):
        self.bad_server = self.MockBadServer()

# Create the Tkinter window and run the Password class
if __name__ == "__main__":
    root = tk.Tk()
    network = MockNetwork()
    app = Password(root, network)
    root.mainloop()
