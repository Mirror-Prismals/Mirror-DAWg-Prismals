import os
from tkinter import Tk
from tkinter.filedialog import askdirectory, asksaveasfilename

def merge_txt_files(source_dir, output_file):
    """
    Recursively merges all .txt files in a directory and its subdirectories
    into a single file.

    Parameters:
    source_dir (str): Path to the directory containing .txt files.
    output_file (str): Path to the output file where all content will be merged.
    """
    with open(output_file, 'w', encoding='utf-8') as outfile:
        for root, _, files in os.walk(source_dir):
            for file in files:
                if file.endswith('.txt'):
                    file_path = os.path.join(root, file)
                    with open(file_path, 'r', encoding='utf-8') as infile:
                        outfile.write(infile.read())
                        outfile.write("\n")  # Add a newline between files for clarity
                    print(f"Merged: {file_path}")

# Select input directory and output file
def main():
    # Initialize tkinter
    root = Tk()
    root.withdraw()  # Hide the main tkinter window
    
    print("Select the source directory containing .txt files.")
    source_directory = askdirectory(title="Select Source Directory")
    if not source_directory:
        print("No directory selected. Exiting.")
        return

    print("Select the output file to save the merged contents.")
    output_file_path = asksaveasfilename(
        title="Save Merged File As",
        defaultextension=".txt",
        filetypes=[("Text Files", "*.txt")]
    )
    if not output_file_path:
        print("No output file selected. Exiting.")
        return

    # Merge files
    merge_txt_files(source_directory, output_file_path)
    print(f"All .txt files have been merged into: {output_file_path}")

# Run the main function
if __name__ == "__main__":
    main()
