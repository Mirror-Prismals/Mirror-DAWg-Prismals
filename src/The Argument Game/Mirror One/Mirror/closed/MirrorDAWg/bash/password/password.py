import tkinter as tk

class Password:
    def __init__(self, root, network):
        self.root = root
        self.network = network

        self.create_password_entry()

    def create_password_entry(self):
        # Code for creating password entry window
        pass

    def unlock_server(self, password):
        # Check if password is correct
        if password == "mira":
            # Change GUI text to green and display message
            self.network.bad_server.change_text("Server unlocked!")
        else:
            # Display incorrect password message
            self.network.bad_server.change_text("Incorrect password!")
