import tkinter as tk
from tkinter import simpledialog, messagebox
from datetime import datetime
import json

def save_posts(posts, filename):
    # Function to save the posts to a JSON file
    with open(filename, "w") as f:
        json.dump(posts, f)

def load_posts(filename):
    # Function to load the posts from a JSON file
    try:
        with open(filename, "r") as f:
            posts_data = json.load(f)
            print("Posts data loaded:", posts_data)
            return posts_data
    except FileNotFoundError:
        return []

# Load the posts data
posts_catalog = load_posts("posts_catalog.json")
print("Loaded posts data:", posts_catalog)

# Load the archive data
archive_catalog = load_posts("archive_catalog.json")
print("Loaded archive data:", archive_catalog)

# Save loaded data back to files (in case they didn't exist before)
save_posts(posts_catalog, "posts_catalog.json")
save_posts(archive_catalog, "archive_catalog.json")

def add_post():
    # Function to add a new text post
    title = simpledialog.askstring("Input", "Enter the title for your post:")
    if title:
        content = simpledialog.askstring("Input", "Enter the content of your post:")
        if content:
            timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            posts_catalog.append({"title": title, "content": content, "timestamp": timestamp})
            view_posts()
            save_posts(posts_catalog, "posts_catalog.json")

def delete_post(index):
    # Function to delete a text post
    if 0 <= index < len(posts_catalog):
        deleted_post = posts_catalog.pop(index)
        deleted_post["archived"] = True
        archive_catalog.append(deleted_post)
        save_posts(posts_catalog, "posts_catalog.json")
        save_posts(archive_catalog, "archive_catalog.json")
        print("Post archived:", deleted_post)
        view_posts()
    else:
        print("Index out of range for posts_catalog")

def restore_post(index):
    # Function to restore a text post from the archive
    if 0 <= index < len(archive_catalog):
        restored_post = archive_catalog.pop(index)
        restored_post["archived"] = False
        posts_catalog.append(restored_post)
        save_posts(posts_catalog, "posts_catalog.json")
        save_posts(archive_catalog, "archive_catalog.json")
        print("Post restored:", restored_post)
        view_archive()
    else:
        print("Index out of range for archive_catalog")

def edit_post(index, catalog):
    # Function to edit a text post
    title = simpledialog.askstring("Edit Title", "Enter the new title for your post:", initialvalue=catalog[index]["title"])
    if title:
        content = simpledialog.askstring("Edit Content", "Enter the new content for your post:", initialvalue=catalog[index]["content"])
        if content:
            catalog[index]["title"] = title
            catalog[index]["content"] = content
            save_posts(posts_catalog, "posts_catalog.json")
            view_posts()

def delete_permanently(index):
    # Function to permanently delete a text post from the archive
    confirm = messagebox.askyesno("Confirmation", "Are you sure you want to permanently delete this post?")
    if confirm:
        if 0 <= index < len(archive_catalog):
            deleted_post = archive_catalog.pop(index)
            save_posts(archive_catalog, "archive_catalog.json")
            print("Post permanently deleted:", deleted_post)
            view_archive()
        else:
            print("Index out of range for archive_catalog")

def view_archive():
    # Function to view archived text posts
    # Destroy all existing widgets
    for widget in frame.winfo_children():
        widget.destroy()

    # Display archived posts
    for i, post in enumerate(archive_catalog):
        post_frame = tk.Frame(frame, relief=tk.GROOVE, borderwidth=2)
        post_frame.grid(row=i, column=0, padx=5, pady=5, sticky="nsew")

        title_label = tk.Label(post_frame, text=post["title"], font=("Helvetica", 16, "bold"))
        title_label.pack(padx=5, pady=5)

        content_label = tk.Label(post_frame, text=post["content"])
        content_label.pack(padx=5, pady=5)

        timestamp_label = tk.Label(post_frame, text=post["timestamp"], font=("Helvetica", 8))
        timestamp_label.pack(padx=5, pady=5)

        edit_button = tk.Button(post_frame, text="Edit", command=lambda idx=i, catalog=archive_catalog: edit_post(idx, catalog))
        edit_button.pack(side="left", padx=5, pady=5)

        restore_button = tk.Button(post_frame, text="Restore", command=lambda idx=i: restore_post(idx))
        restore_button.pack(side="left", padx=5, pady=5)

        delete_button = tk.Button(post_frame, text="Delete Permanently", command=lambda idx=i: delete_permanently(idx))
        delete_button.pack(side="left", padx=5, pady=5)

    # Add a "Back to Main View" button
    back_button = tk.Button(frame, text="Back to Main View", command=view_posts)
    back_button.grid(row=len(archive_catalog), column=0, padx=5, pady=5)

def view_posts():
    # Function to view all stored text posts
    # Destroy all existing widgets
    for widget in frame.winfo_children():
        widget.destroy()

    # Filter out archived posts
    active_posts = [post for post in posts_catalog if not post.get("archived", False)]

    # Display active posts
    for i, post in enumerate(active_posts):
        post_frame = tk.Frame(frame, relief=tk.GROOVE, borderwidth=2)
        post_frame.grid(row=i, column=0, padx=5, pady=5, sticky="nsew")

        title_label = tk.Label(post_frame, text=post["title"], font=("Helvetica", 16, "bold"))
        title_label.pack(padx=5, pady=5)

        content_label = tk.Label(post_frame, text=post["content"])
        content_label.pack(padx=5, pady=5)

        timestamp_label = tk.Label(post_frame, text=post["timestamp"], font=("Helvetica", 8))
        timestamp_label.pack(padx=5, pady=5)

        edit_button = tk.Button(post_frame, text="Edit", command=lambda idx=posts_catalog.index(post): edit_post(idx, posts_catalog))
        edit_button.pack(side="left", padx=5, pady=5)

        delete_button = tk.Button(post_frame, text="Delete", command=lambda idx=posts_catalog.index(post): delete_post(idx))
        delete_button.pack(side="left", padx=5, pady=5)

# Create the main application window
root = tk.Tk()
root.title("MirrorBLAWgIDE is an enigmatic tapestry")
root.attributes('-fullscreen', True)  # Set window to fullscreen
# Add a button to add a new post
add_button = tk.Button(root, text="Add Post", command=add_post)
add_button.pack()

# Add a button to view the archive
view_archive_button = tk.Button(root, text="View Archive", command=view_archive)
view_archive_button.pack()

# Create a frame to contain the posts
frame = tk.Frame(root)
frame.pack()

# Display the posts
view_posts()

# Function to handle closing of the application
def on_closing():
    # Save posts before closing
    save_posts(posts_catalog, "posts_catalog.json")
    save_posts(archive_catalog, "archive_catalog.json")
    root.destroy()

# Bind the closing event to the on_closing function
root.protocol("WM_DELETE_WINDOW", on_closing)

# Start the main event loop
root.mainloop()
