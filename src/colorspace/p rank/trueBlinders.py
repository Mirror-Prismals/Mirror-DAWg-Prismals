import tkinter as tk
from tkinter import filedialog, messagebox
import cv2
import numpy as np
import threading
import os

# Define colorblindness transformation matrices
COLORBLINDNESS_MATRICES = {
    "Protanopia": np.array([
        [0.567, 0.433, 0.000],
        [0.558, 0.442, 0.000],
        [0.000, 0.242, 0.758]
    ]),
    "Deuteranopia": np.array([
        [0.625, 0.375, 0.000],
        [0.700, 0.300, 0.000],
        [0.000, 0.300, 0.700]
    ]),
    "Tritanopia": np.array([
        [0.950, 0.050, 0.000],
        [0.000, 0.433, 0.567],
        [0.000, 0.475, 0.525]
    ])
}

def apply_colorblind_filter(frame, matrix):
    """
    Apply the colorblindness filter to a single frame.
    
    Parameters:
        frame (numpy.ndarray): The input video frame in BGR format.
        matrix (numpy.ndarray): The color transformation matrix.
        
    Returns:
        numpy.ndarray: The colorblindness filtered frame in BGR format.
    """
    # Convert BGR to RGB
    rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    
    # Apply the transformation matrix
    transformed = np.dot(rgb_frame, matrix.T)
    
    # Clip values to [0, 255] and convert to uint8
    transformed = np.clip(transformed, 0, 255).astype(np.uint8)
    
    # Convert back to BGR
    bgr_transformed = cv2.cvtColor(transformed, cv2.COLOR_RGB2BGR)
    
    return bgr_transformed

def process_video(input_path, output_path, filters, progress_callback):
    """
    Process the input video and apply the selected colorblindness filters in sequence.
    
    Parameters:
        input_path (str): Path to the input video.
        output_path (str): Path to save the output video.
        filters (list): List of filter types to apply in order.
        progress_callback (function): Function to update the progress bar.
    """
    try:
        # Open the input video
        cap = cv2.VideoCapture(input_path)
        if not cap.isOpened():
            messagebox.showerror("Error", f"Cannot open video file: {input_path}")
            return
        
        # Get video properties
        fps = cap.get(cv2.CAP_PROP_FPS)
        width  = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
        height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
        total_frames = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
        
        # Define the codec and create VideoWriter object
        fourcc = cv2.VideoWriter_fourcc(*'mp4v')
        out = cv2.VideoWriter(output_path, fourcc, fps, (width, height))
        
        # Get the transformation matrices
        matrices = []
        for filter_type in filters:
            matrix = COLORBLINDNESS_MATRICES.get(filter_type)
            if matrix is None:
                messagebox.showerror("Error", f"Unknown filter type: {filter_type}")
                cap.release()
                out.release()
                return
            matrices.append(matrix)
        
        # Process each frame
        frame_num = 0
        while True:
            ret, frame = cap.read()
            if not ret:
                break
            
            # Apply each selected filter in sequence
            filtered_frame = frame
            for matrix in matrices:
                filtered_frame = apply_colorblind_filter(filtered_frame, matrix)
            
            # Write the frame to the output video
            out.write(filtered_frame)
            
            frame_num += 1
            if frame_num % 10 == 0 or frame_num == total_frames:
                progress = (frame_num / total_frames) * 100
                progress_callback(progress)
        
        # Release resources
        cap.release()
        out.release()
        progress_callback(100)
        messagebox.showinfo("Success", f"Filtered video saved to:\n{output_path}")
    except Exception as e:
        messagebox.showerror("Error", f"An error occurred:\n{e}")

class ColorblindFilterApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Layered Colorblind Filter for Waveform Videos")
        self.root.geometry("700x400")
        self.root.resizable(False, False)
        
        # Initialize variables
        self.input_path = tk.StringVar()
        self.output_path = tk.StringVar()
        self.filter1 = tk.StringVar(value="Protanopia")
        self.filter2 = tk.StringVar(value="None")
        self.filter3 = tk.StringVar(value="None")
        
        # Create UI components
        self.create_widgets()
    
    def create_widgets(self):
        padding_options = {'padx': 10, 'pady': 10}
        
        # Input video selection
        input_label = tk.Label(self.root, text="Input Video (.mp4):")
        input_label.grid(row=0, column=0, sticky='e', **padding_options)
        
        input_entry = tk.Entry(self.root, textvariable=self.input_path, width=50)
        input_entry.grid(row=0, column=1, **padding_options)
        
        input_button = tk.Button(self.root, text="Browse", command=self.browse_input)
        input_button.grid(row=0, column=2, **padding_options)
        
        # Filter type selection - Filter 1
        filter1_label = tk.Label(self.root, text="First Colorblindness Type:")
        filter1_label.grid(row=1, column=0, sticky='e', **padding_options)
        
        filter_options = ["Protanopia", "Deuteranopia", "Tritanopia", "None"]
        filter1_menu = tk.OptionMenu(self.root, self.filter1, *filter_options)
        filter1_menu.grid(row=1, column=1, sticky='w', **padding_options)
        
        # Filter type selection - Filter 2
        filter2_label = tk.Label(self.root, text="Second Colorblindness Type:")
        filter2_label.grid(row=2, column=0, sticky='e', **padding_options)
        
        filter2_menu = tk.OptionMenu(self.root, self.filter2, *filter_options)
        filter2_menu.grid(row=2, column=1, sticky='w', **padding_options)
        
        # Filter type selection - Filter 3
        filter3_label = tk.Label(self.root, text="Third Colorblindness Type:")
        filter3_label.grid(row=3, column=0, sticky='e', **padding_options)
        
        filter3_menu = tk.OptionMenu(self.root, self.filter3, *filter_options)
        filter3_menu.grid(row=3, column=1, sticky='w', **padding_options)
        
        # Output video selection
        output_label = tk.Label(self.root, text="Output Video (.mp4):")
        output_label.grid(row=4, column=0, sticky='e', **padding_options)
        
        output_entry = tk.Entry(self.root, textvariable=self.output_path, width=50)
        output_entry.grid(row=4, column=1, **padding_options)
        
        output_button = tk.Button(self.root, text="Browse", command=self.browse_output)
        output_button.grid(row=4, column=2, **padding_options)
        
        # Start processing button
        start_button = tk.Button(self.root, text="Apply Filters", command=self.start_processing, bg='green', fg='white', font=('Helvetica', 12, 'bold'))
        start_button.grid(row=5, column=1, pady=20)
        
        # Progress bar
        self.progress = tk.DoubleVar()
        self.progress_bar = tk.Canvas(self.root, width=600, height=25, bg='white', highlightthickness=1, highlightbackground="black")
        self.progress_bar.grid(row=6, column=0, columnspan=3, pady=10)
        self.progress_rect = self.progress_bar.create_rectangle(0, 0, 0, 25, fill='blue')
        
        # Progress label
        self.progress_label = tk.Label(self.root, text="Progress: 0%")
        self.progress_label.grid(row=7, column=0, columnspan=3)
    
    def browse_input(self):
        file_path = filedialog.askopenfilename(
            title="Select Input Video",
            filetypes=[("MP4 Files", "*.mp4"), ("All Files", "*.*")]
        )
        if file_path:
            self.input_path.set(file_path)
    
    def browse_output(self):
        file_path = filedialog.asksaveasfilename(
            title="Save Output Video",
            defaultextension=".mp4",
            filetypes=[("MP4 Files", "*.mp4")]
        )
        if file_path:
            self.output_path.set(file_path)
    
    def start_processing(self):
        input_file = self.input_path.get()
        output_file = self.output_path.get()
        filters = []
        
        # Collect selected filters, excluding 'None'
        if self.filter1.get() != "None":
            filters.append(self.filter1.get())
        if self.filter2.get() != "None":
            filters.append(self.filter2.get())
        if self.filter3.get() != "None":
            filters.append(self.filter3.get())
        
        if not input_file:
            messagebox.showwarning("Input Required", "Please select an input video file.")
            return
        if not output_file:
            messagebox.showwarning("Output Required", "Please select an output video file path.")
            return
        if not os.path.isfile(input_file):
            messagebox.showerror("File Not Found", f"The input file does not exist:\n{input_file}")
            return
        if not output_file.lower().endswith('.mp4'):
            messagebox.showerror("Invalid Output", "The output file must have a .mp4 extension.")
            return
        if not filters:
            messagebox.showwarning("No Filters Selected", "Please select at least one filter to apply.")
            return
        if len(filters) > 3:
            messagebox.showwarning("Too Many Filters", "You can apply a maximum of three filters.")
            return
        
        # Disable the UI elements to prevent changes during processing
        self.disable_ui()
        
        # Start processing in a separate thread to keep the GUI responsive
        processing_thread = threading.Thread(
            target=process_video,
            args=(input_file, output_file, filters, self.update_progress)
        )
        processing_thread.start()
    
    def update_progress(self, progress):
        # Update the progress bar
        self.progress_bar.coords(self.progress_rect, 0, 0, 6 * progress, 25)  # 600px width
        self.progress_label.config(text=f"Progress: {int(progress)}%")
        self.root.update_idletasks()
        
        if progress >= 100:
            # Re-enable the UI elements
            self.enable_ui()
    
    def disable_ui(self):
        for child in self.root.winfo_children():
            if isinstance(child, tk.Button) or isinstance(child, tk.Entry) or isinstance(child, tk.OptionMenu):
                child.configure(state='disabled')
        self.progress_label.config(text="Processing...")
    
    def enable_ui(self):
        for child in self.root.winfo_children():
            if isinstance(child, tk.Button) or isinstance(child, tk.Entry) or isinstance(child, tk.OptionMenu):
                child.configure(state='normal')
        self.progress_label.config(text="Progress: 100%")

if __name__ == "__main__":
    root = tk.Tk()
    app = ColorblindFilterApp(root)
    root.mainloop()
