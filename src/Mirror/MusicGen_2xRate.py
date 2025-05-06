# inference.py
import torch
import torchaudio
import soundfile as sf
from model import MiniMusicGen
from audio_utils import reverse_pitch_and_time_shift

def load_model(ckpt_path, device="cpu"):
    model = MiniMusicGen()
    model.load_state_dict(torch.load(ckpt_path, map_location=device))
    model.to(device)
    model.eval()
    return model

def generate_audio(model, prompt, device="cpu", tgt_len=16000):
    with torch.no_grad():
        text = [prompt]
        pred_wave = model(text, tgt_len=tgt_len)
    # pred_wave shape: [batch=1, time]
    pred_wave = pred_wave.squeeze(0).cpu().numpy()
    return pred_wave

def main():
    device = "cuda" if torch.cuda.is_available() else "cpu"
    model = load_model("mini_musicgen_2x.pth", device=device)

    prompt = "A cool jazzy tune with saxophone"
    print("Generating 2x pitched/time audio from prompt:", prompt)
    wave_2x = generate_audio(model, prompt, device=device, tgt_len=32000)  
    # Suppose we do 32000 samples = 2 seconds at 16k (but remember it's 2x domain)

    # Now reverse shift it (pitch/time down)
    wave_final, final_sr = reverse_pitch_and_time_shift(wave_2x, 16000, semitones=-12, speed_factor=0.5)

    # Save to disk
    sf.write("generated_final.wav", wave_final, final_sr)
    print("Saved generated_final.wav at normal pitch/time")

if __name__ == "__main__":
    main()
