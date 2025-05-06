import markovify
import time
import os
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler

# File path to the text file for generating thoughts
file_path = r''

# Load the Markov chain model
def load_model():
    with open(file_path, encoding='utf-8') as f:
        text = f.read()
    return markovify.Text(text, state_size=2)

# Generate a thought from the model
def generate_thought(model):
    thought = model.make_short_sentence(max_chars=1080, tries=100)
    if thought:
        print("\nNew Thought:")
        for line in thought.split(". "):
            print(f"{line}.")
    else:
        print("Couldn't generate a thought.")

# Handler for monitoring file changes
class TopicChangeHandler(FileSystemEventHandler):
    def __init__(self):
        super().__init__()
        self.model = load_model()

    def on_modified(self, event):
        if event.src_path == file_path:
            print("\nTopic updated! Reloading new thoughts...")
            self.model = load_model()

# Main loop to stream consciousness
def stream_of_consciousness():
    event_handler = TopicChangeHandler()
    observer = Observer()
    observer.schedule(event_handler, path=os.path.dirname(file_path), recursive=False)
    observer.start()
    
    try:
        while True:
            generate_thought(event_handler.model)
            time.sleep(10)  # Adjust the interval here if you want more or less time between thoughts
    except KeyboardInterrupt:
        observer.stop()
    observer.join()

# Run the stream of consciousness
stream_of_consciousness()
