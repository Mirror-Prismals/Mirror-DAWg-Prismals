import os


os.environ["OPENAI_API_KEY"] = ""

import openai
import json
import tkinter as tk
from tkinter import scrolledtext, ttk

class Character:
    def __init__(self, name, tagline, description, greeting, definition):
        self.name = name
        self.tagline = tagline
        self.description = description
        self.greeting = greeting
        self.definition = definition
        self.definition_label(bg="teal")  # Set background to cyan
def generate_response(prompt):
    # Create a chat completion
    response = client.chat.completions.create(
        model="gpt-3.5-turbo",
        messages=prompt
    )
    return response.choices[0].message.content.strip()

class CharacterApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Character AI")
        root.attributes('-fullscreen', True)
        


        # Configure OpenAI client
        api_key = os.getenv("OPENAI_API_KEY")
        if api_key is None:
            raise ValueError("API key not found. Please set the OPENAI_API_KEY environment variable.")
        
        global client
        client = openai.Client(api_key=api_key)
        
        # Tabs
        self.tabControl = ttk.Notebook(root)
        self.tab1 = ttk.Frame(self.tabControl)
        self.tab2 = ttk.Frame(self.tabControl)
        self.tabControl.add(self.tab1, text='You Failed To Fetch Save')
        self.tabControl.add(self.tab2, text='Server Offline')
        self.tabControl.pack(expand=1, fill="both")
        
        # Character input fields
        self.name_label = tk.Label(self.tab1, text="Name")
        self.name_label.pack()
        self.name_entry = tk.Entry(self.tab1)
        self.name_entry.pack()
        
        self.tagline_label = tk.Label(self.tab1, text="Tagline")
        self.tagline_label.pack()
        self.tagline_entry = tk.Entry(self.tab1)
        self.tagline_entry.pack()
        
        self.description_label = tk.Label(self.tab1, text="Description")
        self.description_label.pack()
        self.description_entry = tk.Entry(self.tab1)
        self.description_entry.pack()
        
        self.greeting_label = tk.Label(self.tab1, text="Greeting")
        self.greeting_label.pack()
        self.greeting_entry = tk.Entry(self.tab1)
        self.greeting_entry.pack()
        
        self.definition_label = tk.Label(self.tab1, text="Definition")
        self.definition_label.pack()
        self.definition_text = scrolledtext.ScrolledText(self.tab1, wrap=tk.WORD, width=50, height=10)
        
        self.edit_button = tk.Button(self.tab1, text="Edit", command=self.edit_character)
        self.edit_button.pack()
        
        self.save_button = tk.Button(self.tab1, text="Save", command=self.save_character)
        self.save_button.pack()
        
        self.load_button = tk.Button(self.tab2, text="Load", command=self.load_character)
        self.load_button.pack()
        
        self.character_listbox = tk.Listbox(self.tab2, width=50, height=10)
        self.character_listbox.pack()
        
        self.chat_log = scrolledtext.ScrolledText(self.tab1, wrap=tk.WORD, width=50, height=10)
        self.chat_log.pack()
        
        self.message_label = tk.Label(self.tab1, text="Your Message")
        self.message_label.pack()
        self.message_entry = tk.Entry(self.tab1)
        self.message_entry.pack()
        
        self.send_button = tk.Button(self.tab1, text="Send", command=self.send_message)
        self.send_button.pack()
        
        # Character list
        self.character_list = []
        self.selected_character = None
        
        # Load characters from files
        self.load_characters_from_files()

        # Bind close event to save characters
        root.protocol("WM_DELETE_WINDOW", self.save_characters_on_close)

    def create_character(self):
        name = self.name_entry.get()
        tagline = self.tagline_entry.get()
        description = self.description_entry.get()
        greeting = self.greeting_entry.get()
        definition = self.definition_text.get("1.0", tk.END)
        
        character = Character(name, tagline, description, greeting, definition)
        self.character_list.append(character)
        self.update_character_listbox()
        self.clear_character_input_fields()
        
    def edit_character(self):
        if self.selected_character:
            # Clear input fields
            self.clear_character_input_fields()
            # Populate input fields with selected character's details
            self.name_entry.insert(0, self.selected_character.name)
            self.tagline_entry.insert(0, self.selected_character.tagline)
            self.description_entry.insert(0, self.selected_character.description)
            self.greeting_entry.insert(0, self.selected_character.greeting)
            self.definition_text.insert(tk.END, self.selected_character.definition)
        else:
            self.chat_log.insert(tk.END, "Please select a character to edit.\n")
        
    def save_character(self):
        if not self.selected_character:
            self.create_character()
            self.chat_log.insert(tk.END, "Character saved.\n")
        else:
            self.chat_log.insert(tk.END, "Character updated.\n")
            self.selected_character.name = self.name_entry.get()
            self.selected_character.tagline = self.tagline_entry.get()
            self.selected_character.description = self.description_entry.get()
            self.selected_character.greeting = self.greeting_entry.get()
            self.selected_character.definition = self.definition_text.get("1.0", tk.END)
        
    def load_character(self):
        character_index = self.character_listbox.curselection()
        if character_index:
            character_index = int(character_index[0])
            self.selected_character = self.character_list[character_index]
            self.chat_log.insert(tk.END, f"Loaded character {self.selected_character.name}\n")
            self.populate_character_fields()
        else:
            self.chat_log.insert(tk.END, "Please select a character to load.\n")
        
    def send_message(self):
        if not self.selected_character:
            self.chat_log.insert(tk.END, "Please select or create a character first.\n")
            return
        
        user_message = self.message_entry.get()
        self.chat_log.insert(tk.END, f"You: {user_message}\n")
        
        # Build the prompt with the character's persona
        prompt = [
            {"role": "system", "content": f"You are to play the character of:{self.selected_character.name}. {self.selected_character.definition}"},
            {"role": "user", "content": user_message}
        ]
        
        ai_response = generate_response(prompt)
        
        self.chat_log.insert(tk.END, f"{self.selected_character.name}: {ai_response}\n")
        self.message_entry.delete(0, tk.END)
    
    def update_character_listbox(self):
        self.character_listbox.delete(0, tk.END)
        for character in self.character_list:
            self.character_listbox.insert(tk.END, character.name)
            
    def clear_character_input_fields(self):
        self.name_entry.delete(0, tk.END)
        self.tagline_entry.delete(0, tk.END)
        self.description_entry.delete(0, tk.END)
        self.greeting_entry.delete(0, tk.END)
        self.definition_text.delete(1.0, tk.END)
        
    def populate_character_fields(self):
        self.clear_character_input_fields()
        self.name_entry.insert(0, self.selected_character.name)
        self.tagline_entry.insert(0, self.selected_character.tagline)
        self.description_entry.insert(0, self.selected_character.description)
        self.greeting_entry.insert(0, self.selected_character.greeting)
        self.definition_text.insert(tk.END, self.selected_character.definition)

    def load_characters_from_files(self):
        character_folder = "characters"
        if not os.path.exists(character_folder):
            os.makedirs(character_folder)
        
        for filename in os.listdir(character_folder):
            if filename.endswith(".json"):
                with open(os.path.join(character_folder, filename), "r") as file:
                    character_data = json.load(file)
                    character = Character(**character_data)
                    self.character_list.append(character)
        
        self.update_character_listbox()

    def save_character_to_file(self, character):
        character_folder = "characters"
        if not os.path.exists(character_folder):
            os.makedirs(character_folder)
        
        filename = f"{character.name}.json"
        filepath = os.path.join(character_folder, filename)
        with open(filepath, "w") as file:
            character_data = {
                "name": character.name,
                "tagline": character.tagline,
                "description": character.description,
                "greeting": character.greeting,
                "definition": character.definition
            }
            json.dump(character_data, file)

    def save_characters_to_files(self):
        for character in self.character_list:
            self.save_character_to_file(character)
            
    def save_characters_on_close(self):
        self.save_characters_to_files()
        self.root.destroy()

if __name__ == "__main__":
    root = tk.Tk()
    
    app = CharacterApp(root)
    root.mainloop()

