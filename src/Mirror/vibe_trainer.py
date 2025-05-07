import os
import sys
import math
import time
import threading
import queue
import random
import numpy as np
import torch
import torch.nn as nn
from torch.utils.data import Dataset, DataLoader
import pygame
from tkinter import Tk, filedialog
from collections import Counter

EVAL_EVERY_SECONDS = 600  # Now unused!
BOTTOM_ISLAND_SPLIT = 0.52
SEQ_LEN = CONTEXT_LENGTH = 64
DIFFUSION_STEPS = 8
BATCH_SIZE = 2
LR = 3e-4
MASK_TOKEN = "<mask>"
PAD_TOKEN = "<pad>"
VOCAB_SIZE = 50000
DEVICE = "cuda" if torch.cuda.is_available() else "cpu"

def read_config_file(config_path):
    out = {"corpus_path": None, "vocab_source": None, "checkpoint_path": None, "epochs_trained": 0, "prompts": []}
    with open(config_path, "r", encoding="utf8") as f:
        lines = f.readlines()
    for line in lines:
        if "=" in line:
            k, v = line.split("=", 1)
            k, v = k.strip(), v.strip()
            if k == "corpus_path":
                out["corpus_path"] = v
            elif k == "vocab_source":
                out["vocab_source"] = v
            elif k == "checkpoint_path":
                out["checkpoint_path"] = v
            elif k == "epochs_trained":
                out["epochs_trained"] = int(v)
            elif k == "prompts":
                prompts = v.split("---")
                out["prompts"] = [p.strip() for p in prompts if p.strip()]
    return out

def write_config_file(config_path, cfg):
    lines = []
    for k in ["corpus_path", "vocab_source", "checkpoint_path"]:
        lines.append(f"{k} = {cfg.get(k, '')}")
    lines.append(f"epochs_trained = {cfg.get('epochs_trained', 0)}")
    if cfg["prompts"]:
        lines.append("prompts = " + "---".join(cfg["prompts"]))
    else:
        lines.append("prompts = ")
    with open(config_path, "w", encoding="utf8") as f:
        f.write("\n".join(lines) + "\n")

class WordTokenizer:
    def __init__(self, vocablist, special_tokens=None):
        self.special_tokens = list(special_tokens) if special_tokens else []
        self.token2id = {tok: i for i, tok in enumerate(self.special_tokens)}
        offset = len(self.special_tokens)
        for i, w in enumerate(vocablist):
            if w not in self.token2id:
                self.token2id[w] = i + offset
        self.id2token = {i: t for t, i in self.token2id.items()}
        self.vocab_size = len(self.token2id)
        self.mask_token = MASK_TOKEN
        self.pad_token = PAD_TOKEN
        self.mask_token_id = self.token2id[MASK_TOKEN]
        self.pad_token_id = self.token2id[PAD_TOKEN]
    def encode(self, text):
        words = text.strip().split()
        return [self.token2id.get(w, self.pad_token_id) for w in words][:SEQ_LEN]
    def decode(self, ids):
        tokens = [self.id2token[i] for i in ids if i in self.id2token and i != self.pad_token_id]
        return " ".join(tokens)
    def __len__(self): return len(self.token2id)

def build_vocab_from_file(vocab_source, max_vocab=VOCAB_SIZE):
    counter = Counter()
    with open(vocab_source, "r", encoding="utf8") as f:
        for line in f:
            words = line.strip().split()
            counter.update(words)
    top = [w for w, _ in counter.most_common(max_vocab - 2) if w not in (MASK_TOKEN, PAD_TOKEN)]
    vocablist = [PAD_TOKEN, MASK_TOKEN] + top
    return vocablist

def get_tokenizer(cfg):
    vocabfile = (cfg["vocab_source"] or "") + ".vocabcache"
    if os.path.exists(vocabfile):
        with open(vocabfile, "r", encoding="utf8") as f:
            vocablist = [w.strip() for w in f if w.strip()]
    else:
        vocablist = build_vocab_from_file(cfg["vocab_source"], VOCAB_SIZE)
        with open(vocabfile, "w", encoding="utf8") as f:
            for w in vocablist:
                print(w, file=f)
    tokenizer = WordTokenizer(vocablist, special_tokens=[PAD_TOKEN, MASK_TOKEN])
    print("Vocab size:", len(tokenizer), "mask_token_id:", tokenizer.mask_token_id, "pad_token_id:", tokenizer.pad_token_id)
    return tokenizer

class TextDataset(Dataset):
    def __init__(self, folder, tokenizer, seqlen):
        self.chunks = []
        for fname in os.listdir(folder):
            if fname.endswith(".txt"):
                with open(os.path.join(folder, fname), encoding="utf8") as f:
                    text = f.read()
                    words = text.strip().split()
                    for i in range(0, len(words), seqlen):
                        chunk = words[i:i + seqlen]
                        if len(chunk) < seqlen:
                            chunk += [PAD_TOKEN] * (seqlen - len(chunk))
                        ids = [tokenizer.token2id.get(w, tokenizer.pad_token_id) for w in chunk]
                        self.chunks.append(torch.tensor(ids, dtype=torch.long))
        if not self.chunks:
            print("No training data found!")
    def __len__(self): return len(self.chunks)
    def __getitem__(self, idx): return self.chunks[idx]

class DiffusionLM(nn.Module):
    def __init__(self, vocab_size, num_timesteps):
        super().__init__()
        self.emb = nn.Embedding(vocab_size, 384)
        self.t_embed = nn.Embedding(num_timesteps + 1, 384)
        self.layers = nn.ModuleList([
            nn.TransformerEncoderLayer(d_model=384, nhead=6, batch_first=True) for _ in range(4)
        ])
        self.ln = nn.LayerNorm(384)
        self.out = nn.Linear(384, vocab_size)
    def forward(self, x, t_tokens):
        xe = self.emb(x)
        te = self.t_embed(t_tokens)
        h = xe + te
        for lyr in self.layers:
            h = lyr(h)
        h = self.ln(h)
        return nn.functional.log_softmax(self.out(h), dim=-1)

def save_model_with_vocab(model, tokenizer, path):
    out = {
        'state_dict': model.state_dict(),
        'vocab': list(tokenizer.token2id.keys())
    }
    torch.save(out, path)

def load_model_with_vocab(path, vocablist, num_timesteps=DIFFUSION_STEPS):
    checkpoint = torch.load(path, map_location=DEVICE)
    old_sd = checkpoint['state_dict']
    old_vocab = checkpoint['vocab']
    old_word2idx = {w:i for i,w in enumerate(old_vocab)}
    model = DiffusionLM(len(vocablist), num_timesteps)
    new_sd = model.state_dict()
    emb_dim = old_sd['emb.weight'].shape[1]

    for i, word in enumerate(vocablist):
        if word in old_word2idx:
            new_sd['emb.weight'][i] = old_sd['emb.weight'][old_word2idx[word]]
            new_sd['out.weight'][i] = old_sd['out.weight'][old_word2idx[word]]
            new_sd['out.bias'][i] = old_sd['out.bias'][old_word2idx[word]]
        else:
            nn.init.normal_(new_sd['emb.weight'][i:i+1], mean=0, std=0.02)
            nn.init.normal_(new_sd['out.weight'][i:i+1], mean=0, std=0.02)
            new_sd['out.bias'][i] = 0.0
    new_sd['t_embed.weight'][:old_sd['t_embed.weight'].shape[0]] = old_sd['t_embed.weight']
    for k in new_sd:
        if k.startswith('layers') or k == 'ln.weight' or k == 'ln.bias':
            new_sd[k] = old_sd[k]
    model.load_state_dict(new_sd)
    return model

def build_model(tokenizer):
    model = DiffusionLM(len(tokenizer), DIFFUSION_STEPS)
    return model

def per_token_q_sample(x_start, t_tokens, mask_token_id, steps):
    mask_rates = (t_tokens.float() + 1) / steps
    mask = torch.rand_like(mask_rates) < mask_rates
    x_t = x_start.clone()
    x_t[mask] = mask_token_id
    return x_t

def train_worker(
    config_path,
    msg_queue,
    eval_queue,
    stop_event,
    global_state,
    segment_queue
):
    torch.set_grad_enabled(True)
    random.seed(42)
    np.random.seed(42)
    torch.manual_seed(42)
    cfg = read_config_file(config_path)
    msg_queue.put(f"[INFO] Loading vocab/tokenizer/model...")
    tokenizer = get_tokenizer(cfg)

    model = None
    if os.path.exists(cfg["checkpoint_path"]):
        try:
            model = load_model_with_vocab(cfg["checkpoint_path"], list(tokenizer.token2id.keys()), DIFFUSION_STEPS).to(DEVICE)
            msg_queue.put(f"[INFO] Loaded checkpoint: {cfg['checkpoint_path']} with remapped vocab")
        except Exception as e:
            msg_queue.put(f"[WARN] Failed to load/match vocab: {e}, falling back to new model.")
            model = build_model(tokenizer).to(DEVICE)
        start_epoch = int(cfg.get("epochs_trained", 0))
    else:
        msg_queue.put("[INFO] No checkpoint found, training from scratch.")
        model = build_model(tokenizer).to(DEVICE)
        start_epoch = 0

    dataset = TextDataset(cfg["corpus_path"], tokenizer, SEQ_LEN)
    if len(dataset) == 0:
        msg_queue.put("[FATAL] No training data!")
        global_state["train_active"] = False
        return
    dataloader = DataLoader(dataset, batch_size=BATCH_SIZE, shuffle=True, drop_last=False)
    opt = torch.optim.AdamW(model.parameters(), lr=LR)
    loss_f = nn.NLLLoss(ignore_index=-100)
    EPOCHS = 99999
    SAVE_FREQ = 20
    last_eval_time = time.time()
    global_state["last_eval_time"] = last_eval_time

    global_state["total_sequences"] = len(dataset)
    global_state["current_sequence_indices"] = [None, None]
    global_state["train_active"] = True

    for epoch in range(start_epoch + 1, EPOCHS + 1):
        if stop_event.is_set(): break
        global_state["epoch"] = epoch
        cfg['epochs_trained'] = epoch
        write_config_file(config_path, cfg)
        msg_queue.put(f"=== EPOCH {epoch} ===")
        for i, (batch_idx, batch) in enumerate(zip(dataloader._index_sampler if hasattr(dataloader,'_index_sampler') else range(len(dataset)), dataloader)):
            batch = batch.to(DEVICE)
            t_tokens = torch.randint(0, DIFFUSION_STEPS, (batch.size(0), batch.size(1)), device=DEVICE)
            x_noised = per_token_q_sample(batch, t_tokens, tokenizer.mask_token_id, DIFFUSION_STEPS)
            logits = model(x_noised, t_tokens)
            labels = batch.clone()
            labels[x_noised != tokenizer.mask_token_id] = -100
            loss = loss_f(logits.view(-1, logits.size(-1)), labels.view(-1))
            opt.zero_grad()
            loss.backward()
            opt.step()
            if i % 2 == 0:
                msg_queue.put(f"Epoch {epoch} | Step {i} | Loss: {loss.item():.4f}")
            if i % SAVE_FREQ == 0:
                try:
                    save_model_with_vocab(model, tokenizer, cfg['checkpoint_path'])
                except Exception as e:
                    msg_queue.put("[Checkpoint save failed] " + str(e))
                msg_queue.put(f"[CHECKPOINT] Saved, step {i}")

            bs = batch.shape[0]
            if hasattr(dataloader, 'batch_sampler') and hasattr(dataloader.batch_sampler, 'sampler'):
                indices = []
                for idx in range(i*bs, min((i+1)*bs, len(dataset))):
                    indices.append(idx)
            else:
                indices = list(range(i*bs, min((i+1)*bs, len(dataset))))
            if segment_queue and batch.shape[0] > 0:
                seg_ids = batch[0]
                seg_text = tokenizer.decode([int(t) for t in seg_ids if t != tokenizer.pad_token_id])
                try:
                    segment_queue.get_nowait()
                except:
                    pass
                try:
                    segment_queue.put_nowait(seg_text)
                except:
                    pass
                first_seq_index = indices[0] if indices else None
                global_state["current_sequence_indices"] = [first_seq_index, len(dataset)]
    global_state["train_active"] = False

def encode_with_masks(prompt, tokenizer):
    ids = []
    split = prompt.strip().split()
    for w in split:
        if w == MASK_TOKEN:
            ids.append(tokenizer.mask_token_id)
        else:
            ids.append(tokenizer.token2id.get(w, tokenizer.pad_token_id))
    return ids[:SEQ_LEN]

def diffusion_sample(prompt, tokenizer, model, num_steps=DIFFUSION_STEPS):
    ids = encode_with_masks(prompt, tokenizer)
    mask_token_id = tokenizer.mask_token_id
    seq_in = torch.tensor(ids + [mask_token_id]*(SEQ_LEN-len(ids)), dtype=torch.long, device=DEVICE)
    seq = seq_in.clone()
    for t in reversed(range(num_steps)):
        still_masked = (seq==mask_token_id)
        if not still_masked.any(): break
        t_tokens = torch.full((SEQ_LEN,), t, dtype=torch.long, device=DEVICE)
        with torch.no_grad():
            logits = model(seq.unsqueeze(0), t_tokens.unsqueeze(0))[0]
        idx_to_fill = torch.where(still_masked)[0]
        fill_this_step = idx_to_fill[torch.randperm(len(idx_to_fill))[:max(1,len(idx_to_fill)//3)]]
        for idx in fill_this_step:
            token_probs = torch.exp(logits[idx])
            sampled = torch.multinomial(token_probs, 1).item()
            seq[idx] = sampled
    out_ids = [t for t in seq.tolist() if t != tokenizer.pad_token_id]
    out = tokenizer.decode(out_ids)
    return out

def eval_prompts(tokenizer, model, cfg):
    results = []
    prompts = cfg.get("prompts", [])
    for prompt in prompts:
        try:
            if MASK_TOKEN in prompt:
                infill_output = diffusion_sample(prompt, tokenizer, model)
                results.append((prompt, infill_output))
            else:
                # unconditional generation/continuation
                base = encode_with_masks(prompt, tokenizer)
                seq = torch.tensor(base, dtype=torch.long, device=DEVICE)
                idx = len(base)
                while idx < SEQ_LEN:
                    ids = seq.clone()
                    t_tokens = torch.zeros(SEQ_LEN, dtype=torch.long, device=DEVICE)
                    with torch.no_grad():
                        logits = model(ids.unsqueeze(0), t_tokens.unsqueeze(0))[0]
                        next_token = logits[idx].argmax().item()
                    seq = torch.cat([seq, torch.tensor([next_token], device=DEVICE)], dim=0)
                    idx += 1
                    if next_token == tokenizer.pad_token_id: break
                out = tokenizer.decode([int(x) for x in seq.cpu() if x != tokenizer.pad_token_id])
                results.append((prompt, out))
        except Exception as e:
            results.append((prompt, f"ERROR: {e}"))
    return results

segment_queue = queue.Queue(maxsize=2)

pygame.init()
info = pygame.display.Info()
WIDTH, HEIGHT = info.current_w, info.current_h
flags = pygame.NOFRAME | pygame.FULLSCREEN
screen = pygame.display.set_mode((WIDTH, HEIGHT), flags)
pygame.display.set_caption("Diffusion WordLM GUI")
BG_SURF = pygame.Surface((WIDTH, HEIGHT))

for fnt_name in ["segoeui", "arialblack", "arial", None]:
    try:
        if fnt_name:
            SMALLFONT = pygame.font.SysFont(fnt_name, 22)
            TERMINALFONT = pygame.font.SysFont(fnt_name, 20)
            LFNT = pygame.font.SysFont(fnt_name, 24)
            COUNTDOWN_FONT = pygame.font.SysFont(fnt_name, 46)
            EVALFONT = pygame.font.SysFont(fnt_name, 16)
        else:
            SMALLFONT = pygame.font.Font(None, 22)
            TERMINALFONT = pygame.font.Font(None, 20)
            LFNT = pygame.font.Font(None, 24)
            COUNTDOWN_FONT = pygame.font.Font(None, 46)
            EVALFONT = pygame.font.Font(None, 16)
        break
    except Exception:
        continue

def wrap_text(text, font, max_width):
    lines = []
    for paragraph in text.split('\n'):
        words = paragraph.split(' ')
        cur_line = ''
        for word in words:
            if cur_line:
                test_line = cur_line + ' ' + word
            else:
                test_line = word
            width, _ = font.size(test_line)
            if width > max_width and cur_line:
                lines.append(cur_line)
                cur_line = word
            else:
                cur_line = test_line
        if cur_line:
            lines.append(cur_line)
    return lines

def draw_aero_xp_bg(surf, animtime=0.0):
    sky_top, sky_bottom = (120, 190, 250), (10, 80, 200)
    for y in range(surf.get_height() // 2):
        ratio = y / (surf.get_height() // 2)
        c = [int(sky_top[i] * (1 - ratio) + sky_bottom[i] * ratio) for i in range(3)]
        pygame.draw.line(surf, (*c,), (0, y), (surf.get_width(), y))
    for layer, dy in enumerate([0, 30, 60]):
        pts = []
        amp = 90 + layer * 15
        freq = 0.0007 + layer * 0.00015
        ybase = int(surf.get_height() * 0.48 + dy)
        phase = animtime * (0.24 + 0.07 * layer)
        for x in range(0, surf.get_width() + 20, 10):
            wobble = 10 * np.sin((x + dy * 43) * 0.0024 + animtime * 0.6)
            y = ybase \
                + int(np.sin((x + dy * 40) * freq + phase) * amp) \
                + int(np.cos((x + dy * 13) * freq * 2.61 + 2 * phase) * amp * 0.2) \
                + wobble
            pts.append((x, y))
        color = (20 + dy * 2, 230 - dy * 3, 95 + dy * 1)
        pygame.draw.polygon(surf, color, [(0, surf.get_height())] + pts + [(surf.get_width(), surf.get_height())])

def draw_dynamic_island_top(surface, swirl_active, swirl_angle, current_index=None, total_sequences=None, show_eval_btn=False):
    base_w = WIDTH // 4
    base_h = 60
    island_w = base_w
    island_h = base_h
    island_x = WIDTH // 2 - island_w // 2
    island_y = 44
    border_radius = island_h // 2 + 8

    pygame.draw.rect(surface, (0, 0, 0), (island_x, island_y, island_w, island_h), border_radius=border_radius)
    x_btn_rad = 18
    x_btn_center = (island_x - 32, island_y + island_h // 2)
    pygame.draw.circle(surface, (240, 60, 90), x_btn_center, x_btn_rad)
    for d in [-1, 1]:
        pygame.draw.line(surface, (255, 255, 255),
                         (x_btn_center[0] - 8, x_btn_center[1] - d * 8),
                         (x_btn_center[0] + 8, x_btn_center[1] + d * 8), 4)
    margin = 16
    spacing = 16
    btn_rad = 18
    y_center = island_y + island_h // 2
    play_center = (island_x + margin + btn_rad, y_center)
    pygame.draw.circle(surface, (40, 230, 60), play_center, btn_rad)
    tri_size = btn_rad - 6
    tri_points = [
        (play_center[0] - tri_size // 2, play_center[1] - tri_size),
        (play_center[0] - tri_size // 2, play_center[1] + tri_size),
        (play_center[0] + tri_size, play_center[1]),
    ]
    pygame.draw.polygon(surface, (255, 255, 255), tri_points)
    stop_center = (play_center[0] + btn_rad * 2 + spacing, y_center)
    pygame.draw.circle(surface, (230, 40, 60), stop_center, btn_rad)
    square_size = btn_rad - 8
    pygame.draw.rect(surface, (255, 255, 255), (
        stop_center[0] - square_size // 2, stop_center[1] - square_size // 2,
        square_size, square_size), border_radius=5)
    
    # -- EVAL BUTTON or SWIRL loading indicator on right side --
    right_btn_rad = 18
    right_btn_center = (island_x + island_w + 32, y_center)
    eval_rect = pygame.Rect(right_btn_center[0] - right_btn_rad, right_btn_center[1] - right_btn_rad, right_btn_rad*2, right_btn_rad*2)
    if show_eval_btn:
        pygame.draw.circle(surface, (255, 220, 70), right_btn_center, right_btn_rad)
        # Draw lightning bolt (simple poly path)
        lx, ly = right_btn_center
        bolt = [
            (lx - 5, ly - 6), (lx + 1, ly - 6),
            (lx - 2, ly + 1), (lx + 5, ly + 1),
            (lx - 7, ly + 10), (lx - 3, ly + 1)
        ]
        pygame.draw.polygon(surface, (255,255,255), bolt)
    elif swirl_active:
        swirl_center = right_btn_center
        swirl_rad_outer = right_btn_rad
        swirl_thick = 5
        num_segments = 14
        swirl_angle = swirl_angle % (2 * math.pi)
        arc_len = math.pi * 1.1
        start_ang = swirl_angle
        end_ang = swirl_angle + arc_len
        for i in range(num_segments):
            frac = i / num_segments
            ang1 = start_ang + frac * (end_ang - start_ang)
            ang2 = start_ang + (i + 1) / num_segments * (end_ang - start_ang)
            color = (40, 150 + 80 * frac, 255)
            pygame.draw.arc(surface, color,
                            (swirl_center[0] - swirl_rad_outer, swirl_center[1] - swirl_rad_outer,
                             2 * swirl_rad_outer, 2 * swirl_rad_outer),
                            ang1, ang2, swirl_thick)
    play_rect = pygame.Rect(play_center[0] - btn_rad, play_center[1] - btn_rad, btn_rad * 2, btn_rad * 2)
    stop_rect = pygame.Rect(stop_center[0] - btn_rad, stop_center[1] - btn_rad, btn_rad * 2, btn_rad * 2)
    x_rect = pygame.Rect(x_btn_center[0] - x_btn_rad, x_btn_center[1] - x_btn_rad, x_btn_rad * 2, x_btn_rad * 2)
    # Training sequence status
    if current_index is not None and total_sequences is not None:
        idx_str = f"Training on sequence {current_index + 1} / {total_sequences}"
        infont = pygame.font.SysFont(None, 28)
        surf = infont.render(idx_str, True, (255,220,40))
        tx = island_x + island_w//2 - surf.get_width()//2
        ty = island_y + island_h - surf.get_height() - 4
        # Optional: text "glow"
        surface.blit(pygame.font.SysFont(None, 28).render(idx_str, True, (0,0,0)), (tx+1, ty+1))
        surface.blit(surf, (tx, ty))
    return play_rect, stop_rect, x_rect, eval_rect

def draw_recycling_symbol(surface, rot_angle, center, size):
    cx, cy = center
    arm_len = size * 0.52
    arrow_w = size * 0.11
    arrow_l = size * 0.23
    arrow_col = (60, 110, 255)
    arrow_outline = (240, 240, 255)
    for i in range(3):
        theta0 = rot_angle + i * 2 * math.pi / 3
        theta1 = theta0 + 2.6
        x0 = cx + math.cos(theta0) * arm_len
        y0 = cy + math.sin(theta0) * arm_len
        x1 = cx + math.cos(theta1) * arm_len
        y1 = cy + math.sin(theta1) * arm_len
        v = np.array([x1 - x0, y1 - y0])
        v = v / np.linalg.norm(v)
        n = np.array([-v[1], v[0]])
        shaft0 = np.array([x0, y0]) + n * arrow_w / 2
        shaft1 = np.array([x0, y0]) - n * arrow_w / 2
        shaft2 = np.array([x1, y1]) - n * arrow_w / 2
        shaft3 = np.array([x1, y1]) + n * arrow_w / 2
        arrow_tip = np.array([x1, y1]) + v * arrow_l
        arrow_left = np.array([x1, y1]) + n * arrow_w
        arrow_right = np.array([x1, y1]) - n * arrow_w
        pygame.draw.polygon(surface, arrow_col, [shaft0, shaft1, shaft2, shaft3])
        pygame.draw.polygon(surface, arrow_outline, [shaft0, shaft1, shaft2, shaft3], 2)
        pygame.draw.polygon(surface, arrow_col, [arrow_tip, arrow_left, arrow_right])
        pygame.draw.polygon(surface, arrow_outline, [arrow_tip, arrow_left, arrow_right], 2)
    pygame.draw.circle(surface, (230, 245, 255, 120), (int(cx), int(cy)), int(size * 0.08))

def eval_area_max_width():
    base_w = WIDTH // 4
    base_h = 60
    island_w = int(base_w * 1.5)
    island_x = WIDTH // 2 - island_w // 2
    margin_rx = 18
    split_x = int(island_x + island_w * BOTTOM_ISLAND_SPLIT)
    rx = split_x + margin_rx
    return (island_x + island_w) - rx - 20

def max_lines_on_terminal():
    base_h = 60
    margin_y = 18
    island_h = base_h * 3
    term_h = island_h - 2 * margin_y
    max_lines = int((term_h) // 24)
    return max_lines

def draw_dynamic_island_bottom_split(surface, terminal_lines, eval_pairs, term_scroll, auto_term_follow, eval_scroll, current_seg_text=None):
    base_w = WIDTH // 4
    base_h = 60
    island_w = int(base_w * 1.5)
    island_h = base_h * 3
    island_x = WIDTH // 2 - island_w // 2
    margin_bottom = 44
    island_y = HEIGHT - island_h - margin_bottom
    border_radius = 36
    pygame.draw.rect(surface, (0, 0, 0), (island_x, island_y, island_w, island_h), border_radius=border_radius)
    split_x = int(island_x + island_w * BOTTOM_ISLAND_SPLIT)
    pygame.draw.line(surface, (40, 100, 60), (split_x, island_y + 18), (split_x, island_y + island_h - 18), 2)
    margin_x = 18
    margin_y = 18
    left_lines = terminal_lines
    term_h = island_h - 2 * margin_y
    max_lines = int((term_h) // 24)
    if len(left_lines) == 0:
        left_lines = [" "]
    scroll_off = term_scroll
    vis_start = max(0, len(left_lines) - max_lines - scroll_off)
    vis_end = vis_start + max_lines
    for idx, line in enumerate(left_lines[vis_start:vis_end]):
        textsurf = TERMINALFONT.render(line, True, (0, 255, 0))
        surface.blit(textsurf, (island_x + margin_x, island_y + margin_y + idx * 24))
    if len(left_lines) > max_lines:
        sb_w = 7
        sb_x = island_x + 5
        sb_y = island_y + margin_y
        sb_h = int(term_h * max_lines / max(1, len(left_lines)))
        sb_top = int(sb_y + (term_h - sb_h) * (vis_start / max(1, len(left_lines) - max_lines)))
        pygame.draw.rect(surface, (0, 110, 0), (sb_x, sb_top, sb_w, sb_h), border_radius=3)
    margin_rx = 18
    margin_ry = 22
    rx = split_x + margin_rx
    ry = island_y + margin_ry
    y_cursor = ry
    prompt_color = (0, 170, 255)
    model_color = (255, 255, 255)
    sep_col = (60, 150, 200)
    eval_max_width = eval_area_max_width()
    eval_max_height = (island_y + island_h) - ry - 22
    line_height = EVALFONT.get_linesize()
    lines = []
    for (prompt, result) in eval_pairs:
        lines.append((LFNT, "Prompt:", prompt_color))
        wrapped_prompt = wrap_text(prompt, EVALFONT, eval_max_width)
        for wline in wrapped_prompt:
            lines.append((EVALFONT, wline, prompt_color))
        lines.append((LFNT, "Model:", model_color))
        for line in result.strip().split("\n"):
            for splitline in wrap_text(line, EVALFONT, eval_max_width):
                lines.append((EVALFONT, splitline, model_color))
        lines.append(("SEP", None, sep_col))
    extra_lines = []
    if current_seg_text:
        extra_lines.append((EVALFONT, "Currently training segment:", (255,255,80)))
        for l in wrap_text(current_seg_text, EVALFONT, eval_max_width):
            extra_lines.append((EVALFONT, l, (200,200,120)))
    lines.extend(extra_lines)
    visible_lines = eval_max_height // line_height
    vis_start = max(0, len(lines) - visible_lines - eval_scroll)
    vis_end = vis_start + visible_lines
    y_draw = y_cursor
    for font, line, color in lines[vis_start:vis_end]:
        if font == "SEP":
            if y_draw + 5 < island_y + island_h - 24:
                pygame.draw.line(surface, color, (rx, y_draw), (split_x + (island_w-split_x)*0.94, y_draw), 1)
                y_draw += 5
            continue
        tsurf = font.render(line, True, color)
        surface.blit(tsurf, (rx, y_draw))
        y_draw += line_height

def eval_max_scroll_offset(eval_pairs, extra_bottom_lines):
    base_w = WIDTH // 4
    base_h = 60
    island_w = int(base_w * 1.5)
    island_h = base_h * 3
    margin_bottom = 44
    island_x = WIDTH // 2 - island_w // 2
    margin_ry = 22
    margin_rx = 18
    split_x = int(island_x + island_w * BOTTOM_ISLAND_SPLIT)
    rx = split_x + margin_rx
    island_y = HEIGHT - island_h - margin_bottom
    ry = island_y + margin_ry
    eval_max_height = (island_y + island_h) - ry - 22
    line_height = EVALFONT.get_linesize()
    lines = []
    for (prompt, result) in eval_pairs:
        lines.append("[Prompt:]")
        lines += wrap_text(prompt, EVALFONT, eval_area_max_width())
        lines.append("[Model:]")
        for out_line in result.strip().split("\n"):
            lines += wrap_text(out_line, EVALFONT, eval_area_max_width())
        lines.append("")
    lines += [""] * extra_bottom_lines
    visible_lines = eval_max_height // line_height
    n = max(0, len(lines) - visible_lines)
    return n

terminal_scroll = 0
eval_scroll = 0
auto_follow_terminal = True
eval_auto_follow = True

def terminal_scroll_up():
    global terminal_scroll, auto_follow_terminal
    terminal_scroll += 1
    max_off = max(0, len(terminal_lines) - max_lines_on_terminal())
    if terminal_scroll > max_off: terminal_scroll = max_off
    if terminal_scroll != 0: auto_follow_terminal = False

def terminal_scroll_down():
    global terminal_scroll, auto_follow_terminal
    terminal_scroll -= 1
    if terminal_scroll < 0: terminal_scroll = 0
    if terminal_scroll == 0: auto_follow_terminal = True

def reset_scroll_if_needed():
    global terminal_scroll, auto_follow_terminal
    max_off = max(0, len(terminal_lines) - max_lines_on_terminal())
    if auto_follow_terminal or terminal_scroll > max_off:
        terminal_scroll = 0
        auto_follow_terminal = True

def pick_config_file():
    Tk().withdraw()
    filename = filedialog.askopenfilename(
        title="Select your DiffusionLM config file...",
        filetypes=[("Config Files", "*.txt *.conf *.cfg *.ini"), ("All Files", "*.*")]
    )
    return filename

running = True
t0 = time.time()
swirl_active = False
swirl_angle = 0
terminal_lines = []
eval_output_pairs = []
current_seg_text = ""
train_thread = None
train_state = {"running": False}
stop_event = threading.Event()
msg_queue = queue.Queue()
eval_queue = queue.Queue()
global_state = {"epoch": 0, "step": 0, "last_eval_time": time.time(), "total_sequences": 0, "current_sequence_indices": [None, None], "train_active": False}

config_path = None
tokenizer = None
model = None
cfg = None

while running:
    try:
        current_seg_text = segment_queue.get_nowait()
    except queue.Empty:
        pass
    for event in pygame.event.get():
        if event.type == pygame.QUIT or (event.type == pygame.KEYDOWN and event.key == pygame.K_ESCAPE):
            running = False
        elif event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
            mouse_pos = pygame.mouse.get_pos()
            index, total = global_state.get("current_sequence_indices", [None, None])
            train_active = swirl_active or global_state.get("train_active", False)
            play_rect, stop_rect, x_rect, eval_rect = draw_dynamic_island_top(BG_SURF, swirl_active, swirl_angle, index, total, show_eval_btn=(not train_active and config_path))
            if x_rect.collidepoint(mouse_pos):
                running = False
            elif play_rect.collidepoint(mouse_pos) and not swirl_active:
                config_path = pick_config_file()
                if not config_path or not os.path.isfile(config_path):
                    continue
                stop_event.clear()
                swirl_active = True
                train_state["running"] = True
                terminal_lines = ["[INFO] Training started."]
                terminal_scroll = 0
                auto_follow_terminal = True
                global_state["last_eval_time"] = time.time()
                global_state["train_active"] = True
                train_thread = threading.Thread(target=train_worker, args=(
                    config_path, msg_queue, eval_queue, stop_event, global_state, segment_queue), daemon=True)
                train_thread.start()
            elif stop_rect.collidepoint(mouse_pos) and swirl_active:
                terminal_lines += ["[INFO] Stop requested, will halt at next save-point/batch."]
                stop_event.set()
                swirl_active = False
            elif eval_rect.collidepoint(mouse_pos) and (not swirl_active and not global_state.get("train_active", False)) and config_path:
                # Manual eval!
                try:
                    cfg = read_config_file(config_path)
                    # load tokenizer/model
                    tokenizer = get_tokenizer(cfg)
                    if os.path.exists(cfg["checkpoint_path"]):
                        model = load_model_with_vocab(cfg["checkpoint_path"], list(tokenizer.token2id.keys()), DIFFUSION_STEPS).to(DEVICE)
                    else:
                        model = build_model(tokenizer).to(DEVICE)
                    model.eval()
                    terminal_lines += ["[EVAL] Manual evaluation running..."]
                    results = eval_prompts(tokenizer, model, cfg)
                    eval_output_pairs = results
                    eval_scroll = 0
                    eval_auto_follow = True
                    terminal_lines += ["[EVAL] Complete."]
                except Exception as e:
                    terminal_lines += [f"[EVAL ERROR] {e}"]
        elif event.type == pygame.MOUSEWHEEL:
            mx, my = pygame.mouse.get_pos()
            base_w = WIDTH // 4
            base_h = 60
            island_w = int(base_w * 1.5)
            island_h = base_h * 3
            island_x = WIDTH // 2 - island_w // 2
            margin_bottom = 44
            island_y = HEIGHT - island_h - margin_bottom
            split_x = int(island_x + island_w * BOTTOM_ISLAND_SPLIT)
            left_area = pygame.Rect(island_x, island_y, split_x - island_x, island_h)
            right_area = pygame.Rect(split_x, island_y, island_w - (split_x - island_x), island_h)
            if left_area.collidepoint(mx, my):
                if event.y > 0:
                    for _ in range(event.y):
                        terminal_scroll_up()
                elif event.y < 0:
                    for _ in range(-event.y):
                        terminal_scroll_down()
            elif right_area.collidepoint(mx, my):
                if event.y > 0:
                    for _ in range(event.y):
                        eval_scroll += 1
                        max_off = max(0, eval_max_scroll_offset(eval_output_pairs, 2 if current_seg_text else 0))
                        if eval_scroll > max_off: eval_scroll = max_off
                        if eval_scroll != 0: eval_auto_follow = False
                elif event.y < 0:
                    for _ in range(-event.y):
                        eval_scroll -= 1
                        if eval_scroll < 0: eval_scroll = 0
                        if eval_scroll == 0: eval_auto_follow = True
        elif event.type == pygame.KEYDOWN:
            mx, my = pygame.mouse.get_pos()
            base_w = WIDTH // 4
            base_h = 60
            island_w = int(base_w * 1.5)
            island_x = WIDTH // 2 - island_w // 2
            margin_bottom = 44
            island_h = base_h * 3
            island_y = HEIGHT - island_h - margin_bottom
            split_x = int(island_x + island_w * BOTTOM_ISLAND_SPLIT)
            right_area = pygame.Rect(split_x, island_y, island_w - (split_x - island_x), island_h)
            if right_area.collidepoint(mx, my):
                if event.key in [pygame.K_UP, pygame.K_PAGEUP]:
                    eval_scroll += 1
                    max_off = max(0, eval_max_scroll_offset(eval_output_pairs, 2 if current_seg_text else 0))
                    if eval_scroll > max_off: eval_scroll = max_off
                    if eval_scroll != 0: eval_auto_follow = False
                elif event.key in [pygame.K_DOWN, pygame.K_PAGEDOWN]:
                    eval_scroll -= 1
                    if eval_scroll < 0: eval_scroll = 0
                    if eval_scroll == 0: eval_auto_follow = True
                elif event.key == pygame.K_END:
                    eval_scroll = 0
                    eval_auto_follow = True
            else:
                if event.key in [pygame.K_UP, pygame.K_PAGEUP]:
                    terminal_scroll_up()
                elif event.key in [pygame.K_DOWN, pygame.K_PAGEDOWN]:
                    terminal_scroll_down()
                elif event.key == pygame.K_END:
                    terminal_scroll = 0
                    auto_follow_terminal = True

    newlines = 0
    while not msg_queue.empty():
        line = msg_queue.get()
        for lsub in line.split("\n"):
            lsub = lsub.strip()
            if lsub:
                terminal_lines.append(lsub)
                newlines += 1
    last_pairs = None
    while not eval_queue.empty():
        last_pairs = eval_queue.get()
    if last_pairs is not None:
        eval_output_pairs = last_pairs
        eval_scroll = 0
        eval_auto_follow = True
    if newlines > 0:
        reset_scroll_if_needed()
    if len(terminal_lines) == 0:
        terminal_lines = [" "]
    animtime = time.time() - t0
    BG_SURF.fill((0, 0, 0))
    draw_aero_xp_bg(BG_SURF, animtime)

    index, total = global_state.get("current_sequence_indices", [None, None])
    train_active = swirl_active or global_state.get("train_active", False)
    play_rect, stop_rect, x_rect, eval_rect = draw_dynamic_island_top(
        BG_SURF, swirl_active, swirl_angle, index, total, show_eval_btn=(not train_active and config_path)
    )
    draw_dynamic_island_bottom_split(
        BG_SURF,
        terminal_lines,
        eval_output_pairs,
        terminal_scroll,
        auto_follow_terminal,
        eval_scroll,
        current_seg_text=current_seg_text
    )
    if swirl_active:
        swirl_angle += 0.12
    symb_size = int(min(WIDTH, HEIGHT) * 0.28)
    center = (WIDTH // 2, HEIGHT // 2)
    recycle_rot = (animtime * 0.7) % (2 * math.pi)
    draw_recycling_symbol(BG_SURF, recycle_rot, center, symb_size)
    screen.blit(BG_SURF, (0, 0))
    pygame.display.flip()
    pygame.time.Clock().tick(60)

pygame.quit()
