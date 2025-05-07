import cv2
from fer import FER
import tkinter as tk
from tkinter import filedialog, messagebox

def analyze_emotions(image_path):
    frame = cv2.imread(image_path)
    if frame is None:
        raise ValueError(f"Could not load image: {image_path}")
    detector = FER(mtcnn=False)
    results = detector.detect_emotions(frame)
    if not results:
        return None
    emotions = results[0]['emotions']
    emotions['contempt'] = 0.0
    ordered = {
        'anger': emotions.get('angry', 0.0),
        'contempt': emotions.get('contempt', 0.0),
        'disgust': emotions.get('disgust', 0.0),
        'fear': emotions.get('fear', 0.0),
        'happiness': emotions.get('happy', 0.0),
        'neutral': emotions.get('neutral', 0.0),
        'sadness': emotions.get('sad', 0.0),
        'surprise': emotions.get('surprise', 0.0)
    }
    return ordered

def select_image():
    file_path = filedialog.askopenfilename(
        filetypes=[("Image Files", "*.jpg *.jpeg *.png *.bmp *.gif")]
    )
    if not file_path:
        return
    try:
        emotions = analyze_emotions(file_path)
        if emotions is None:
            messagebox.showinfo("Result", "No face detected in the image.")
        else:
            result = "\n".join(f"{k}: {v:.2f}" for k, v in emotions.items())
            messagebox.showinfo("Emotion Levels", result)
    except Exception as e:
        messagebox.showerror("Error", str(e))

root = tk.Tk()
root.title("Quick Emotion Recognition")
root.geometry("300x120")
btn = tk.Button(root, text="Select Image", command=select_image, font=("Arial", 14))
btn.pack(expand=True, pady=30)
root.mainloop()
