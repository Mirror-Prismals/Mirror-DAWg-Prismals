import tkinter as tk
from tkinter import scrolledtext
from transformers import GPT2LMHeadModel, GPT2Tokenizer
from PIL import Image, ImageTk  # For handling images

#-----------------------------------------------------------
#  STEP 1: Load the fine-tuned model and tokenizer
#-----------------------------------------------------------
model_path = r"M:\:3/"  # Replace with your own fine-tuned model directory
print("Loading the fine-tuned model...")
model = GPT2LMHeadModel.from_pretrained(model_path)
tokenizer = GPT2Tokenizer.from_pretrained(model_path)

# Ensure the tokenizer's padding token is set
tokenizer.pad_token = tokenizer.eos_token

#-----------------------------------------------------------
#  STEP 2: Define a function to generate text
#-----------------------------------------------------------
def generate_text(prompt, max_length=1024, temperature=0.7, top_k=50, top_p=0.95):
    inputs = tokenizer(prompt, return_tensors="pt", truncation=True, max_length=1024)
    outputs = model.generate(
        inputs["input_ids"],
        max_length=max_length,
        temperature=temperature,  # Controls randomness
        top_k=top_k,              # Filters unlikely words
        top_p=top_p,              # Nucleus sampling for diversity
        do_sample=True,           # Enable sampling for randomness
        num_return_sequences=1,   # Number of responses to generate
    )
    return tokenizer.decode(outputs[0], skip_special_tokens=True)

#-----------------------------------------------------------
#  STEP 3: Create a Jungle-Rainforest themed Tkinter Chatbot
#-----------------------------------------------------------
def main():
    root = tk.Tk()
    root.title("Mira's Rainforest Chatbot")
    root.attributes("-fullscreen", True)

    # OPTIONAL: Let ESC exit fullscreen
    def exit_fullscreen(event):
        root.attributes("-fullscreen", False)
    root.bind("<Escape>", exit_fullscreen)

    # Always define screen width & height BEFORE loading the image
    screen_width = root.winfo_screenwidth()
    screen_height = root.winfo_screenheight()

    # ============== Load / Prepare Background Image ==============
    try:
        bg_image = Image.open("rainforest_bg.jpg")
        # Resize the image to match screen dimensions
        bg_image = bg_image.resize((screen_width, screen_height), Image.LANCZOS)
        bg_photo = ImageTk.PhotoImage(bg_image)
    except Exception as e:
        print(f"Failed to load background image: {e}")
        bg_photo = None

    canvas = tk.Canvas(root, highlightthickness=0, bd=0)
    canvas.pack(fill=tk.BOTH, expand=True)

    if bg_photo:
        canvas_bg = canvas.create_image(0, 0, anchor=tk.NW, image=bg_photo)

    # A translucent overlay frame for the chat area
    overlay_frame = tk.Frame(root, bg="#000000", bd=0)
    # Now screen_width & screen_height exist regardless of the try/except
    overlay_frame.place(
        relx=0.5,
        rely=0.5,
        anchor="center",
        width=int(screen_width * 0.8),
        height=int(screen_height * 0.8)
    )

    # ============== Title Label ==============
    title_label = tk.Label(
        overlay_frame,
        text="Welcome to Mira's Rainforest!",
        font=("Papyrus", 40, "bold"),
        fg="#FFDD00",
        bg="#006400"  # Deep Jungle Green
    )
    title_label.pack(pady=20)

    # ============== Scrolled Text for Chat Display ==============
    chat_display = scrolledtext.ScrolledText(
        overlay_frame,
        wrap=tk.WORD,
        font=("Comic Sans MS", 16),
        bg="#0B3B0B",
        fg="#FFFFFF",
        width=80,
        height=20,
        bd=10,
        relief=tk.RIDGE
    )
    chat_display.insert(tk.END, "Mira is listening from deep within the jungle...\n\n")
    chat_display.config(state=tk.DISABLED)
    chat_display.pack(padx=10, pady=10, fill=tk.BOTH, expand=True)

    # ============== Input Frame ==============
    input_frame = tk.Frame(overlay_frame, bg="#006400")
    input_frame.pack(fill=tk.X, padx=10, pady=10)

    prompt_label = tk.Label(
        input_frame,
        text="Your Message:",
        font=("Verdana", 14, "bold"),
        fg="#FFFAFA",
        bg="#006400"
    )
    prompt_label.pack(side=tk.LEFT, padx=5)

    prompt_entry = tk.Entry(
        input_frame,
        font=("Verdana", 14),
        bg="#228B22",  # ForestGreen
        fg="#FFFFFF",
        bd=5,
        relief=tk.SUNKEN,
        width=50
    )
    prompt_entry.pack(side=tk.LEFT, expand=True, fill=tk.X, padx=5)

    # ============== Send Function ==============
    def send_prompt(event=None):
        user_prompt = prompt_entry.get().strip()
        if not user_prompt:
            return

        # If user types 'exit', close the app
        if user_prompt.lower() == "exit":
            root.destroy()
            return

        # Display user's message
        chat_display.config(state=tk.NORMAL)
        chat_display.insert(tk.END, f"You: {user_prompt}\n\n", "user")
        chat_display.config(state=tk.DISABLED)
        prompt_entry.delete(0, tk.END)

        # Generate and display chatbot response
        chatbot_response = generate_text(user_prompt)
        chat_display.config(state=tk.NORMAL)
        chat_display.insert(tk.END, f"Mira: {chatbot_response}\n\n", "bot")
        chat_display.config(state=tk.DISABLED)
        chat_display.see(tk.END)  # Auto-scroll

    # ============== Send Button ==============
    send_button = tk.Button(
        input_frame,
        text="Send to the Jungle",
        font=("Verdana", 14, "bold"),
        bg="#8B0000",   # Dark Red
        fg="#FFFFFF",
        command=send_prompt
    )
    send_button.pack(side=tk.LEFT, padx=5)

    # Bind Return key
    prompt_entry.bind("<Return>", send_prompt)

    # ============== Big 'Exit the Jungle' Button ==============
    exit_button = tk.Button(
        overlay_frame,
        text="Leave the Rainforest",
        font=("Verdana", 14, "bold"),
        bg="#8B4513",   # SaddleBrown
        fg="#FFFFFF",
        command=root.destroy
    )
    exit_button.pack(pady=10)

    root.mainloop()

if __name__ == "__main__":
    main()
