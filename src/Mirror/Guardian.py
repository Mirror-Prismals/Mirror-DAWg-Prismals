from flask import Flask, request, session, redirect, url_for, render_template_string, send_file, jsonify
import os, time, threading, shutil, json
from datetime import datetime
from io import BytesIO

from PIL import Image, ImageOps, ImageEnhance
import numpy as np
import torch
from transformers import CLIPProcessor, CLIPModel

app = Flask(__name__)
app.secret_key = 'a-very-secret-key'  # Change this in production

# --- Passwords ---
PASSWORD = ""       # Login password
ADMIN_PASSWORD = ""  # Admin password for toggling censor and safe flag

# --- Global Toggle ---
censor_enabled = True  # Determines whether NSFW images are censored

# --- Directories ---
OUTPUT_DIR = r""
ALLOWED_EXTENSIONS = ('.png', '.jpg', '.jpeg', '.gif', '.bmp')

# --- Safe Flags Persistence ---
safe_flags_file = os.path.join(OUTPUT_DIR, "admin_safe_flags.json")
try:
    with open(safe_flags_file, "r") as f:
        safe_flags = json.load(f)
except Exception:
    safe_flags = {}

def save_safe_flags():
    with open(safe_flags_file, "w") as f:
        json.dump(safe_flags, f)

# --- Global Variables for Monitoring ---
last_new_image_time = time.time()
last_known_image_count = 0

# ------------------ CLIP Model Setup and Precompute Tag Features ------------------ #
device = "cuda" if torch.cuda.is_available() else "cpu"
model = CLIPModel.from_pretrained("openai/clip-vit-base-patch32").to(device)
processor = CLIPProcessor.from_pretrained("openai/clip-vit-base-patch32")

# Define NSFW and control tag pools.
nsfw_tags = ["", "", "", "", ""]
control_tags = [
    "person", "car", "dog", "cat", "keyboard", "mouse", "window", "button", "icon",
    "menu", "image", "video", "document", "spreadsheet", "code", "browser", "folder",
    "file", "application", "desktop", "text"
]

NSFW_THRESHOLD = 0.01  # If any NSFW tag’s similarity exceeds this, the image is flagged

with torch.no_grad():
    nsfw_text_inputs = processor(text=nsfw_tags, return_tensors="pt", padding=True).to(device)
    nsfw_text_features = model.get_text_features(**nsfw_text_inputs)
    nsfw_text_features = nsfw_text_features / nsfw_text_features.norm(dim=-1, keepdim=True)
    
    control_text_inputs = processor(text=control_tags, return_tensors="pt", padding=True).to(device)
    control_text_features = model.get_text_features(**control_text_inputs)
    control_text_features = control_text_features / control_text_features.norm(dim=-1, keepdim=True)

# ------------------ NSFW and Control Analysis ------------------ #
def analyze_image(image):
    """
    Processes the image with CLIP and computes cosine similarities between
    the image and each NSFW tag individually (using precomputed features).
    Also computes the highest control tag similarity.
    Returns:
      - nsfw_confidences: dict mapping each NSFW tag to its cosine similarity.
      - nsfw_flag: True if any NSFW tag’s similarity exceeds NSFW_THRESHOLD.
      - control_label: the control tag with the highest similarity.
      - control_score: its corresponding similarity.
    """
    inputs = processor(images=image, return_tensors="pt").to(device)
    with torch.no_grad():
        image_features = model.get_image_features(**inputs)
        image_features = image_features / image_features.norm(dim=-1, keepdim=True)
    nsfw_similarities = (image_features @ nsfw_text_features.T).squeeze(0)
    nsfw_confidences = {tag: sim.item() for tag, sim in zip(nsfw_tags, nsfw_similarities)}
    nsfw_flag = any(sim > NSFW_THRESHOLD for sim in nsfw_similarities)
    control_similarities = (image_features @ control_text_features.T).squeeze(0)
    max_control_score, max_control_idx = control_similarities.max(dim=0)
    control_label = control_tags[max_control_idx.item()]
    control_score = max_control_score.item()
    return nsfw_confidences, nsfw_flag, control_label, control_score

# ------------------ Censorship (Deep-Fry) Effect ------------------ #
def apply_strong_censorship(image):
    """
    Applies a strong censorship (deep-frying) effect to the image.
    This includes fixed pixelation, extreme JPEG compression,
    boosted contrast/saturation with added noise, and a final color inversion.
    All parameters are fixed and internal.
    If an error occurs during processing, the original image is returned.
    """
    try:
        PIXELATION_LEVEL = 8       # Fixed internal pixelation level
        COMPRESSION_QUALITY = 1     # Extreme compression (quality=1)

        # Pixelation
        original_size = image.size
        small = image.resize(
            (max(1, original_size[0] // PIXELATION_LEVEL),
             max(1, original_size[1] // PIXELATION_LEVEL)),
            resample=Image.BILINEAR
        )
        pixelated = small.resize(original_size, Image.NEAREST)
        
        # Simulated Extreme JPEG Compression
        buf = BytesIO()
        pixelated.save(buf, format="JPEG", quality=COMPRESSION_QUALITY)
        buf.seek(0)
        compressed = Image.open(buf).convert("RGB")
        
        # Deep-Fry Distortion
        enhancer = ImageEnhance.Contrast(compressed)
        high_contrast = enhancer.enhance(3.0)
        enhancer = ImageEnhance.Color(high_contrast)
        high_saturation = enhancer.enhance(3.0)
        arr = np.array(high_saturation).astype(np.int16)
        noise = np.random.randint(-80, 80, arr.shape, dtype=np.int16)
        noisy_arr = arr + noise
        noisy_arr = np.clip(noisy_arr, 0, 255).astype(np.uint8)
        noisy_image = Image.fromarray(noisy_arr)
        
        # Inversion
        inverted = noisy_image
        return inverted
    except Exception as e:
        print("Error in apply_strong_censorship:", e)
        return image  # fallback to original image if error occurs

# ------------------ Safe Removal Helpers ------------------ #
def safe_remove(path, timeout=5, interval=0.1):
    """Attempt to remove a file, retrying until successful or timeout."""
    start = time.time()
    while True:
        if not os.path.exists(path):
            return True
        try:
            os.remove(path)
            return True
        except (PermissionError, FileNotFoundError):
            if time.time() - start > timeout:
                print(f"Warning: could not remove file {path} after {timeout} seconds.")
                return False
            time.sleep(interval)

def safe_rmtree(path, timeout=5, interval=0.1):
    """Attempt to remove a directory tree, retrying until successful or timeout."""
    start = time.time()
    while os.path.exists(path):
        try:
            shutil.rmtree(path)
            return True
        except (PermissionError, FileNotFoundError):
            if time.time() - start > timeout:
                print(f"Warning: could not remove directory {path} after {timeout} seconds.")
                return False
            time.sleep(interval)

def safe_rename(src, dst, timeout=5, interval=0.1):
    """Attempt to rename a folder, retrying until successful or timeout."""
    start = time.time()
    while True:
        try:
            os.rename(src, dst)
            return True
        except (PermissionError, FileNotFoundError):
            if time.time() - start > timeout:
                print(f"Warning: could not rename {src} to {dst} after {timeout} seconds.")
                return False
            time.sleep(interval)

# ------------------ Image Directory Monitoring ------------------ #
def get_image_list():
    try:
        files = [f for f in os.listdir(OUTPUT_DIR) if f.lower().endswith(ALLOWED_EXTENSIONS)]
        files.sort(key=lambda f: os.path.getmtime(os.path.join(OUTPUT_DIR, f)))
        return files
    except Exception as e:
        print("Error reading image directory:", e)
        return []

def monitor_images():
    global last_new_image_time, last_known_image_count
    while True:
        try:
            images = get_image_list()
            count = len(images)
            if count > last_known_image_count:
                last_new_image_time = time.time()
                last_known_image_count = count
                print(f"[{datetime.now()}] New image detected. Total images: {count}. Timer reset.")
            elif count < last_known_image_count:
                last_known_image_count = count
            time.sleep(1)
        except Exception as e:
            print("Error in monitor_images:", e)
            time.sleep(1)

# ------------------ Admin Folder Cloning ------------------ #
def initialize_admin_folder():
    """
    On startup, if a current Admin folder exists, rename it to a log folder.
    Then create a new "current" folder.
    """
    admin_base = os.path.join(OUTPUT_DIR, "Admin")
    current_folder = os.path.join(admin_base, "current")
    if os.path.exists(current_folder):
        new_name = "log-" + str(int(time.time()))
        new_path = os.path.join(admin_base, new_name)
        safe_rename(current_folder, new_path)
    os.makedirs(os.path.join(OUTPUT_DIR, "Admin", "current"), exist_ok=True)

def clone_admin_folder():
    """
    Clones the OUTPUT_DIR (excluding the Admin folder) into the Admin/current folder.
    For each image, if NSFW and if censoring is enabled (unless the file is flagged safe),
    applies the censorship effect.
    """
    admin_base = os.path.join(OUTPUT_DIR, "Admin")
    current_folder = os.path.join(admin_base, "current")
    os.makedirs(current_folder, exist_ok=True)
    # Safely clear the current folder.
    for f in os.listdir(current_folder):
        path = os.path.join(current_folder, f)
        if os.path.isfile(path):
            safe_remove(path)
        elif os.path.isdir(path):
            safe_rmtree(path)
    # Copy images from OUTPUT_DIR (excluding the Admin folder).
    for f in os.listdir(OUTPUT_DIR):
        if f.lower().endswith(ALLOWED_EXTENSIONS) and f != "Admin":
            src_path = os.path.join(OUTPUT_DIR, f)
            try:
                image = Image.open(src_path).convert("RGB")
            except Exception:
                continue
            nsfw_confidences, nsfw_flag, control_label, control_score = analyze_image(image)
            if censor_enabled and nsfw_flag and not safe_flags.get(f, False):
                image = apply_strong_censorship(image)
            if image is None:
                continue
            dest_path = os.path.join(current_folder, f)
            try:
                image.save(dest_path)
            except Exception as e:
                print(f"Warning: could not save {dest_path}: {e}")

def admin_folder_updater():
    while True:
        clone_admin_folder()
        time.sleep(5)

# ------------------ Toggle Censor Endpoint ------------------ #
@app.route('/toggle', methods=['POST'])
def toggle():
    global censor_enabled
    admin_pass = request.form.get('password')
    if admin_pass != ADMIN_PASSWORD:
        return jsonify({"success": False, "error": "Invalid admin password."}), 403
    censor_enabled = not censor_enabled
    return jsonify({"success": True, "censor_enabled": censor_enabled})

# ------------------ Toggle Safe Flag Endpoint ------------------ #
@app.route('/toggle_flag', methods=['POST'])
def toggle_flag():
    admin_pass = request.form.get('password')
    if admin_pass != ADMIN_PASSWORD:
        return jsonify({"success": False, "error": "Invalid admin password."}), 403
    file_name = request.form.get('filename')
    if not file_name:
        return jsonify({"success": False, "error": "No file specified."}), 400
    current_status = safe_flags.get(file_name, False)
    new_status = not current_status
    safe_flags[file_name] = new_status
    save_safe_flags()
    return jsonify({"success": True, "safe_flag": new_status})

# ------------------ Gallery Endpoint ------------------ #
@app.route('/gallery')
def gallery():
    if not session.get('logged_in'):
        return redirect(url_for('login_view'))
    mode = request.args.get('mode', 'blacklist')
    images = get_image_list()
    if mode == 'whitelist':
        images = [img for img in images if safe_flags.get(img, False)]
    return render_template_string(GALLERY_TEMPLATE, images=images, mode=mode)

GALLERY_TEMPLATE = '''
<!DOCTYPE html>
<html>
<head>
    <title>Gallery</title>
    <style>
        body { background-color: #000; color: #fff; font-family: Arial, sans-serif; text-align: center; }
        .gallery-container { display: flex; flex-wrap: wrap; justify-content: center; }
        .gallery-item { margin: 10px; }
        .gallery-item img { max-width: 200px; max-height: 200px; border: 2px solid #fff; }
        a { color: #fff; text-decoration: none; }
    </style>
</head>
<body>
    <h1>Gallery</h1>
    <div>
        <a href="{{ url_for('index') }}">Back to Main</a>
    </div>
    <div>
        {% if mode == 'whitelist' %}
            <a href="{{ url_for('gallery', mode='blacklist') }}">Show All (Blacklist)</a>
            | Mode: Whitelist (Safe Only)
        {% else %}
            <a href="{{ url_for('gallery', mode='whitelist') }}">Show Only Safe (Whitelist)</a>
            | Mode: Blacklist (All Files)
        {% endif %}
    </div>
    <div class="gallery-container">
        {% for image in images %}
        <div class="gallery-item">
            <a href="{{ url_for('index', index=loop.index0, mode=mode) }}">
                <img src="{{ url_for('get_image', filename=image) }}" alt="{{ image }}">
            </a>
            <div>{{ image }}</div>
        </div>
        {% endfor %}
    </div>
</body>
</html>
'''

# ------------------ Login and Authentication ------------------ #
@app.route('/login', methods=['GET', 'POST'])
def login_view():
    error = None
    if request.method == 'POST':
        password = request.form.get('password', '')
        if password == PASSWORD:
            session['logged_in'] = True
            return redirect(url_for('index'))
        else:
            error = "Incorrect password"
    return render_template_string(LOGIN_TEMPLATE, error=error)

@app.route('/logout')
def logout():
    session.pop('logged_in', None)
    return redirect(url_for('login_view'))

LOGIN_TEMPLATE = '''
<!DOCTYPE html>
<html>
<head>
    <title>Login - Image Viewer</title>
    <style>
        body { background-color: #000; color: #fff; font-family: Arial, sans-serif; }
        .login-container { width: 300px; margin: 100px auto; text-align: center; }
        input { width: 90%; padding: 10px; margin: 10px 0; }
        button { width: 95%; padding: 10px; }
    </style>
</head>
<body>
    <div class="login-container">
        <h2>Login</h2>
        {% if error %}
            <p style="color: red;">{{ error }}</p>
        {% endif %}
        <form method="post">
            <input type="password" name="password" placeholder="Enter password" required>
            <button type="submit">Login</button>
        </form>
    </div>
</body>
</html>
'''

# ------------------ Main Image Viewer and Analysis ------------------ #
@app.route('/')
def index():
    if not session.get('logged_in'):
        return redirect(url_for('login_view'))
    mode = request.args.get('mode', 'blacklist')
    images = get_image_list()
    if mode == 'whitelist':
        images = [img for img in images if safe_flags.get(img, False)]
    if not images:
        return render_template_string(MAIN_TEMPLATE, image=None, prev_index=None, next_index=None, mode=mode)
    index_str = request.args.get('index', None)
    if index_str is not None and index_str.isdigit():
        current_index = int(index_str)
        if current_index < 0 or current_index >= len(images):
            current_index = len(images) - 1
    else:
        current_index = len(images) - 1
    current_image = images[current_index]
    prev_index = current_index - 1 if current_index > 0 else None
    next_index = current_index + 1 if current_index < len(images) - 1 else None
    return render_template_string(MAIN_TEMPLATE,
                                  image=current_image,
                                  prev_index=prev_index,
                                  next_index=next_index,
                                  mode=mode)

MAIN_TEMPLATE = '''
<!DOCTYPE html>
<html>
<head>
    <title>Image Viewer</title>
    <style>
        body { background-color: #000; color: #fff; font-family: Arial, sans-serif; text-align: center; }
        img { max-width: 90%%; max-height: 80vh; margin-top: 20px; border: 2px solid #fff; }
        .nav-buttons { margin-top: 20px; }
        .nav-buttons a { color: #fff; text-decoration: none; margin: 0 20px; font-size: 24px; }
        #mode-toggle { margin-top: 10px; }
        #toggle-container { margin-top: 20px; }
        #flag-container { margin-top: 10px; }
        .logout { margin-top: 20px; }
        #timer-container { margin-top: 30px; }
        #timer-progress { width: 90%%; height: 20px; }
        #analysis-info { margin-top: 20px; }
    </style>
</head>
<body>
    <h1>Image Viewer</h1>
    <div id="mode-toggle">
        {% if mode == 'whitelist' %}
            <a href="{{ url_for('index', mode='blacklist') }}">Switch to Blacklist (All Files)</a>
        {% else %}
            <a href="{{ url_for('index', mode='whitelist') }}">Switch to Whitelist (Safe Only)</a>
        {% endif %}
    </div>
    {% if image %}
        <img id="main-image" src="{{ url_for('get_image', filename=image) }}" alt="Image">
    {% else %}
        <p>No images found in the directory.</p>
    {% endif %}
    <div class="nav-buttons">
        {% if prev_index is not none %}
            <a href="{{ url_for('index', index=prev_index, mode=mode) }}">← Previous</a>
        {% endif %}
        {% if next_index is not none %}
            <a href="{{ url_for('index', index=next_index, mode=mode) }}">Next →</a>
        {% endif %}
        <a href="{{ url_for('gallery', mode=mode) }}">Gallery</a>
    </div>
    <div id="toggle-container">
        <button id="toggle-button" onclick="toggleCensor()">Toggle Censor (Admin Only)</button>
        <span id="toggle-status">Censor is currently: ON</span>
    </div>
    <div id="flag-container">
        <button id="flag-button" onclick="toggleSafeFlag()">Flag as Safe (Admin Only)</button>
        <span id="flag-status">Not Flagged as Safe</span>
    </div>
    <div id="analysis-info">
        <p>Top 3 NSFW Scores: <span id="top-scores">-</span></p>
        <p>Control: <span id="control-label">-</span> (<span id="control-score">-</span>)</p>
    </div>
    <div class="logout">
        <a href="{{ url_for('logout') }}">Logout</a>
    </div>
    <div id="timer-container">
        <p>Time until next image expected: <span id="timer-remaining">300</span> seconds</p>
        <progress id="timer-progress" value="0" max="300"></progress>
    </div>
    <script>
        function updateTimer() {
            fetch('/timer')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('timer-remaining').innerText = Math.ceil(data.remaining);
                    document.getElementById('timer-progress').value = 300 - data.remaining;
                })
                .catch(error => console.error('Error fetching timer:', error));
        }
        setInterval(updateTimer, 1000);

        function updateAnalysis() {
            var filename = "{{ image }}";
            fetch("/analyze/" + encodeURIComponent(filename))
                .then(response => response.json())
                .then(data => {
                    var topScores = data.top_three_scores;
                    document.getElementById("top-scores").innerText = topScores.map(score => score.toFixed(2)).join(", ");
                    document.getElementById("control-label").innerText = data.control_label;
                    document.getElementById("control-score").innerText = data.control_score.toFixed(2);
                    if(data.safe_flag) {
                        document.getElementById("flag-status").innerText = "Flagged as Safe";
                        document.getElementById("flag-button").innerText = "Unflag as Safe (Admin Only)";
                    } else {
                        document.getElementById("flag-status").innerText = "Not Flagged as Safe";
                        document.getElementById("flag-button").innerText = "Flag as Safe (Admin Only)";
                    }
                })
                .catch(error => console.error("Error fetching analysis:", error));
        }
        updateAnalysis();

        function toggleCensor() {
            var adminPass = prompt("Enter admin password to toggle censor:");
            if (adminPass != null) {
                var formData = new FormData();
                formData.append("password", adminPass);
                fetch("/toggle", { method: "POST", body: formData })
                    .then(response => response.json())
                    .then(data => {
                        if(data.success) {
                            document.getElementById("toggle-status").innerText = "Censor is currently: " + (data.censor_enabled ? "ON" : "OFF");
                            updateAnalysis();
                        } else {
                            alert("Invalid admin password.");
                        }
                    })
                    .catch(error => { alert("Error toggling censor."); console.error(error); });
            }
        }

        function toggleSafeFlag() {
            var adminPass = prompt("Enter admin password to toggle safe flag for this file:");
            if (adminPass != null) {
                var formData = new FormData();
                formData.append("password", adminPass);
                formData.append("filename", "{{ image }}");
                fetch("/toggle_flag", { method: "POST", body: formData })
                    .then(response => response.json())
                    .then(data => {
                        if(data.success) {
                            updateAnalysis();
                        } else {
                            alert("Error: " + data.error);
                        }
                    })
                    .catch(error => { alert("Error toggling safe flag."); console.error(error); });
            }
        }
    </script>
</body>
</html>
'''

# ------------------ Image and Analysis Endpoints ------------------ #
@app.route('/images/<path:filename>')
def get_image(filename):
    if not filename.lower().endswith(ALLOWED_EXTENSIONS):
        return "File type not allowed", 403
    image_path = os.path.join(OUTPUT_DIR, filename)
    try:
        image = Image.open(image_path).convert("RGB")
    except Exception as e:
        return f"Error loading image: {str(e)}", 500
    nsfw_confidences, nsfw_flag, control_label, control_score = analyze_image(image)
    if not safe_flags.get(filename, False) and censor_enabled and nsfw_flag:
        print(f"Image '{filename}' flagged as NSFW (confidences: {nsfw_confidences}). Applying strong censorship.")
        image = apply_strong_censorship(image)
    buf = BytesIO()
    image.save(buf, format="PNG")
    buf.seek(0)
    return send_file(buf, mimetype="image/png")

@app.route('/analyze/<path:filename>')
def analyze(filename):
    if not filename.lower().endswith(ALLOWED_EXTENSIONS):
        return jsonify({"error": "Invalid file"}), 400
    image_path = os.path.join(OUTPUT_DIR, filename)
    try:
        image = Image.open(image_path).convert("RGB")
    except Exception as e:
        return jsonify({"error": str(e)}), 500
    nsfw_confidences, nsfw_flag, control_label, control_score = analyze_image(image)
    top_three_scores = sorted(nsfw_confidences.values(), reverse=True)[:3]
    safe_flag = safe_flags.get(filename, False)
    return jsonify({
         "top_three_scores": top_three_scores,
         "control_label": control_label,
         "control_score": control_score,
         "censored": nsfw_flag,
         "safe_flag": safe_flag
    })

@app.route('/timer')
def timer():
    now = time.time()
    elapsed = now - last_new_image_time
    remaining = max(0, 300 - elapsed)
    return jsonify({
       'remaining': remaining,
       'elapsed': elapsed,
       'max_time': 300,
       'progress': min(100, (elapsed / 300) * 100)
    })

# ------------------ Main Application Startup ------------------ #
if __name__ == '__main__':
    def initialize_admin_folder():
        admin_base = os.path.join(OUTPUT_DIR, "Admin")
        current_folder = os.path.join(admin_base, "current")
        if os.path.exists(current_folder):
            new_name = "log-" + str(int(time.time()))
            new_path = os.path.join(admin_base, new_name)
            safe_rename(current_folder, new_path)
        os.makedirs(os.path.join(OUTPUT_DIR, "Admin", "current"), exist_ok=True)

    def clone_admin_folder():
        admin_base = os.path.join(OUTPUT_DIR, "Admin")
        current_folder = os.path.join(admin_base, "current")
        os.makedirs(current_folder, exist_ok=True)
        for f in os.listdir(current_folder):
            path = os.path.join(current_folder, f)
            if os.path.isfile(path):
                safe_remove(path)
            elif os.path.isdir(path):
                safe_rmtree(path)
        for f in os.listdir(OUTPUT_DIR):
            if f.lower().endswith(ALLOWED_EXTENSIONS) and f != "Admin":
                src_path = os.path.join(OUTPUT_DIR, f)
                try:
                    image = Image.open(src_path).convert("RGB")
                except Exception:
                    continue
                nsfw_confidences, nsfw_flag, control_label, control_score = analyze_image(image)
                if censor_enabled and nsfw_flag and not safe_flags.get(f, False):
                    image = apply_strong_censorship(image)
                if image is None:
                    continue
                dest_path = os.path.join(current_folder, f)
                try:
                    image.save(dest_path)
                except Exception as e:
                    print(f"Warning: could not save {dest_path}: {e}")

    def admin_folder_updater():
        while True:
            clone_admin_folder()
            time.sleep(5)

    initialize_admin_folder()
    admin_thread = threading.Thread(target=admin_folder_updater, daemon=True)
    admin_thread.start()
    monitor_thread = threading.Thread(target=monitor_images, daemon=True)
    monitor_thread.start()
    app.run(host="0.0.0.0", port=5000, debug=True)
