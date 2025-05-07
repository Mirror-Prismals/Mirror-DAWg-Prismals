import torch
from transformers import GPT2LMHeadModel, GPT2Tokenizer
import time
import os

# --- Configuration ---
model_name = "gpt2-small"
output_dir = "gpt2-small-washed-auto-eval"
sequence_length = 1024
batch_size = 32
learning_rate = 5e-5
num_epochs = 10  # Increased for more detailed tracking
save_every_steps = 5000
device = "cuda" if torch.cuda.is_available() else "cpu"

print(f"Using device: {device}")

# --- 1. Load Model and Tokenizer ---
model = GPT2LMHeadModel.from_pretrained(model_name).to(device)
tokenizer = GPT2Tokenizer.from_pretrained(model_name)
tokenizer.pad_token = tokenizer.eos_token

# --- 2. Function to Generate Random Data ---
def generate_random_sequence(seq_length):
    random_bytes = os.urandom(seq_length // 2)
    hex_string = random_bytes.hex()[:seq_length]
    return hex_string

# --- 3. Training Loop (Washing Process with Dynamic Data Generation and Auto-Evaluation) ---
optimizer = torch.optim.AdamW(model.parameters(), lr=learning_rate)
total_steps = (len(torch.utils.data.DataLoader([None] * (100000 // batch_size), batch_size=batch_size)) * num_epochs) # Estimate
step = 0
start_time = time.time()

print("Starting the washing process with dynamic data generation and auto-evaluation...")

evaluation_prompts = ["The quick brown fox", "Hello world", "Once upon a time", "0x", "A random sequence of"]

for epoch in range(num_epochs):
    print(f"\n--- Starting Epoch {epoch+1}/{num_epochs} ---")
    dummy_dataset = [None] * (100000 // batch_size)
    dataloader = torch.utils.data.DataLoader(dummy_dataset, batch_size=batch_size, shuffle=True)

    for batch_idx, _ in enumerate(dataloader):
        model.train()
        optimizer.zero_grad()

        batch = [generate_random_sequence(sequence_length) for _ in range(batch_size)]
        tokens = tokenizer(batch, truncation=True, padding='max_length', max_length=sequence_length, return_tensors="pt").to(device)
        input_ids, attention_mask = tokens["input_ids"], tokens["attention_mask"]

        outputs = model(input_ids, attention_mask=attention_mask, labels=input_ids)
        loss = outputs.loss
        loss.backward()
        optimizer.step()

        step += 1
        if step % 1000 == 0: # Reduced frequency for step-wise logging
            elapsed_time = time.time() - start_time
            sequences_processed = step * batch_size
            print(f"Epoch {epoch+1}/{num_epochs}, Step {step}/{total_steps:.0f}, Loss: {loss.item():.4f}, "
                  f"Sequences Processed: {sequences_processed}, Time: {elapsed_time:.2f}s")

        if step % save_every_steps == 0:
            save_path = os.path.join(output_dir, f"checkpoint_step_{step}")
            model.save_pretrained(save_path)
            tokenizer.save_pretrained(save_path)
            print(f"Checkpoint saved at {save_path}")

    # --- Auto-Evaluation at the End of Each Epoch ---
    print(f"\n--- Evaluation at the End of Epoch {epoch+1} ---")
    model.eval()
    with torch.no_grad():
        for prompt in evaluation_prompts:
            input_ids = tokenizer.encode(prompt, return_tensors="pt").to(device)
            output = model.generate(input_ids, max_length=50, num_return_sequences=1, do_sample=True, temperature=1.0)
            generated_text = tokenizer.decode(output[0], skip_special_tokens=True)
            print(f"Prompt: '{prompt}'\nGenerated: '{generated_text}'\n---")
    model.train() # Set back to train mode

end_time = time.time()
total_wash_time = end_time - start_time
print(f"\nTotal washing time: {total_wash_time:.2f} seconds")

# --- 4. Save the Final Washed Model ---
model.save_pretrained(output_dir)
tokenizer.save_pretrained(output_dir)
print(f"Final washed model saved to {output_dir}")
