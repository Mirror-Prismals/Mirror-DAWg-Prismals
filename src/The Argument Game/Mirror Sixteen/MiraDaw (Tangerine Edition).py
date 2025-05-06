import os
import subprocess
import threading
import tkinter as tk
from tkinter import simpledialog, scrolledtext, ttk
from PIL import Image, ImageTk
import pyttsx3
import random
import nltk
from nltk.sentiment.vader import SentimentIntensityAnalyzer

# Download VADER lexicon if not already done
nltk.download('vader_lexicon')

# Initialize the sentiment intensity analyzer
sid = SentimentIntensityAnalyzer()

# Initialize text-to-speech engine
tts_engine = pyttsx3.init()
tts_engine.setProperty('rate', 150)  # Speed up the voice

# Define personas with colors
personas = {
    "Tangerine": {
        "greeting": "🍊",
        "responses": ["parents got confused n chose tangerine daughter 🔥🔥", "Can you buy me more tangerines?", "Yes 😔"],
        "color": "#ff4500"
    },
    "Lime": {
        "greeting": "🍋",
        "responses": ["Limes are so zesty!", "Lime juice is the best!", "Have you tried lime sorbet?", "I could use a bit of lime in my drink. 😉"],
        "color": "#8FF000"
    },
    "Grapes": {
        "greeting": "🍇",
        "responses": ["I bought you two bags on Wednesday.", "Fine.", "You're joking", "I got more yesterday"],
        "color": "#6b0463"
    }
}

# Define the main application class
class FunChatApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Fun Chat Bot")
        self.root.attributes('-fullscreen', True)
        self.root.configure(bg='#ff4500')

        self.current_persona = "Tangerine"

        self.tabControl = ttk.Notebook(root)
        self.chat_tab = ttk.Frame(self.tabControl, style="TFrame")
        self.story_tab = ttk.Frame(self.tabControl, style="TFrame")
        self.tabControl.add(self.chat_tab, text='Chat')
        self.tabControl.add(self.story_tab, text='Story')
        self.tabControl.pack(expand=1, fill="both")

        style = ttk.Style()
        style.configure("TFrame", background='#ff4500')
        style.configure("TLabel", background='#ff4500', foreground='white')
        style.configure("TButton", background='#ff4500', foreground='white')

        # Chat tab
        self.chat_box = scrolledtext.ScrolledText(self.chat_tab, wrap=tk.WORD, font=("Helvetica", 16), fg="white", bg="#ff4500")
        self.chat_box.pack(expand=True, fill='both')
        
        self.user_input = tk.Entry(self.chat_tab, font=("Helvetica", 16), fg="white", bg="#ff4500")
        self.user_input.pack(fill='x', side='bottom')
        self.user_input.bind("<Return>", self.get_response)
        
        self.root.bind("<Escape>", self.exit_fullscreen)
        self.chat_box.insert(tk.END, f"{personas[self.current_persona]['greeting']}\n")
        self.chat_box.configure(state='disabled')

        # Persona buttons
        self.button_frame = tk.Frame(self.chat_tab, bg="#ff4500")
        self.button_frame.pack(fill='x')
        self.tangerine_button = tk.Button(self.button_frame, text="Tangerine", command=lambda: self.switch_persona("Tangerine"), bg='#ff4500', fg='white')
        self.tangerine_button.pack(side='left', expand=True)
        self.lime_button = tk.Button(self.button_frame, text="Lime", command=lambda: self.switch_persona("Lime"), bg='#8FF000', fg='white')
        self.lime_button.pack(side='left', expand=True)
        self.berry_button = tk.Button(self.button_frame, text="Grapes", command=lambda: self.switch_persona("Grapes"), bg='#6b0463', fg='white')
        self.berry_button.pack(side='left', expand=True)

        # Story tab
        self.story_text = scrolledtext.ScrolledText(self.story_tab, wrap=tk.WORD, font=("Helvetica", 16), fg="white", bg="#ff4500")
        self.story_text.pack(expand=True, fill='both')
        self.story_text.insert(tk.END, "Once upon a time in a land full of fruits...\n")

        self.chat_box.configure(state='disabled')

        self.chuck_process = None
        self.processing_thread = None

    def switch_persona(self, persona):
        self.current_persona = persona
        self.update_colors(personas[persona]['color'])
        self.chat_box.configure(state='normal')
        self.chat_box.insert(tk.END, f"... {persona} ....\n")
        self.chat_box.insert(tk.END, f"{personas[persona]['greeting']}\n")
        self.chat_box.configure(state='disabled')

    def update_colors(self, color):
        self.root.configure(bg=color)
        self.chat_box.configure(bg=color)
        self.user_input.configure(bg=color)
        self.story_text.configure(bg=color)
        self.button_frame.configure(bg=color)
        self.tangerine_button.configure(bg=personas["Tangerine"]['color'])
        self.lime_button.configure(bg=personas["Lime"]['color'])
        self.berry_button.configure(bg=personas["Grapes"]['color'])

    def get_response(self, event):
        user_input = self.user_input.get()
        self.chat_box.configure(state='normal')
        self.chat_box.insert(tk.END, f"You: {user_input}\n")

        response = random.choice(personas[self.current_persona]['responses'])
        self.chat_box.insert(tk.END, f"{self.current_persona}: {response}\n")
        
        # Text-to-speech
        tts_engine.say(response)
        tts_engine.runAndWait()

        self.chat_box.configure(state='disabled')
        self.user_input.delete(0, tk.END)

    def exit_fullscreen(self, event):
        self.root.destroy()
        if self.chuck_process:
            self.chuck_process.terminate()
        if self.processing_thread:
            self.processing_thread.join()

def start_processing():
    def run_processing():
        processing_script = """
        import math

        def setup():
            fullScreen()
            colorMode(RGB, 255)
            global t, low_res_width, low_res_height
            t = 0
            low_res_width = width // 16
            low_res_height = height // 16
            frameRate(60)

        def draw():
            global t
            t += 0.1
            loadPixels()
            for y in range(low_res_height):
                for x in range(low_res_width):
                    r = int(255 * (x / low_res_width))
                    g = int(255 * ((y + t) % low_res_height / low_res_height))
                    b = int(255 * ((x + y + t) % (2 * low_res_width) / (2 * low_res_width)))
                    pixels[y * low_res_width + x] = color(r, g, b)
            updatePixels()

        def mousePressed():
            col = get(mouseX, mouseY)
            hex_color = '#{:02x}{:02x}{:02x}'.format(int(red(col)), int(green(col)), int(blue(col)))
            print('Hex Color:', hex_color)
            textAlign(CENTER, CENTER)
            fill(255)
            rect(0, 0, width, 50)
            fill(0)
            text('Hex Color: ' + hex_color, width / 2, 25)
        """
        with open("processing_script.pyde", "w") as f:
            f.write(processing_script)
        #subprocess.Popen(["processing-java", "--sketch=.", "--run"], cwd=os.path.dirname(os.path.abspath("processing_script.pyde")))
    
    app.processing_thread = threading.Thread(target=run_processing)
    app.processing_thread.start()

def start_chuck():
    def run_chuck():
        chuck_script = """

//Specials Audition

["sinewave", "ahh", "britestk", "doh", "eee", "fwavblnk", "halfwave", "impuls10", "impuls20", "impuls40", "mand1", "mandpluk", "marmstk1", "ooo", "peksblnk", "ppksblnk", "sineblnk", "sinewave", "snglpeak", "twopeaks", "glot_ahh", "glot_eee", "glot_ooo", "glot_pop" ] @=> string specials[];

SndBuf buffs[specials.size()];
Gain g => dac;
g.gain(0.8);

5::ms => dur separator;

for ( 0 => int i; i < specials.size(); i++ )
{
    buffs[i] => g;
    "special:" + specials[i] => buffs[i].read;
    0 => buffs[i].pos;
    <<< specials[i], buffs[i].length()  >>>;
    buffs[i].length() => now;
    separator => now;
}

        """
        with open("sound_script.ck", "w") as f:
            f.write(chuck_script)
        app.chuck_process = subprocess.Popen(["chuck", "sound_script.ck"])
    
    chuck_thread = threading.Thread(target=run_chuck)
    chuck_thread.start()

if __name__ == "__main__":
    # Start the Tkinter application
    root = tk.Tk()
    app = FunChatApp(root)
    # Start the Processing script
    start_processing()
    # Start the ChucK script
    start_chuck()
    root.mainloop()
