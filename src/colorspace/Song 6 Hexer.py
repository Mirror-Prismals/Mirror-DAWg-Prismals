import numpy as np
import librosa
from sklearn.cluster import KMeans
import hashlib

def generate_colors(audio_data):
    # Compute the hash of the audio file's contents
    audio_hash = hashlib.md5(audio_data).hexdigest()

    # Use the hash value as the seed for random number generation
    np.random.seed(int(audio_hash[:8], 16))

    # Generate random colors
    num_colors = 6
    random_colors = [np.random.randint(0, 256, 3) for _ in range(num_colors)]

    # Convert RGB values to hexadecimal format
    hex_colors = ['#' + ''.join(f'{c:02x}' for c in color) for color in random_colors]

    return hex_colors

# Define the location of your .wav file
wav_file_location = ''

# Load the audio file
audio, sample_rate = librosa.load(wav_file_location, sr=16000)

# Generate the mel-spectrogram
mel_spectrogram = librosa.feature.melspectrogram(y=audio, sr=sample_rate, n_mels=3, fmax=8000)
mel_spectrogram_reshaped = mel_spectrogram.T

# Perform K-means clustering
num_clusters = 6
kmeans = KMeans(n_clusters=num_clusters)
kmeans.fit(mel_spectrogram_reshaped)
clustered_features = kmeans.labels_
cluster_centers = kmeans.cluster_centers_

# Get the audio file contents as bytes
with open(wav_file_location, "rb") as f:
    audio_data = f.read()

# Generate colors based on the audio file contents
colors = generate_colors(audio_data)

print(colors)
