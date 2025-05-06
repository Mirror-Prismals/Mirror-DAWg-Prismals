from transformers import GPT2LMHeadModel, GPT2Tokenizer, Trainer, TrainingArguments
import torch

# Step 1: Load and preprocess the dataset
# Specify the path to your text file
input_file = "StillThere.txt"

# Load the GPT-2 tokenizer
tokenizer = GPT2Tokenizer.from_pretrained(r"M:\fine_tuned_gpt2_cont_2\checkpoint-1500")

# Assign a padding token (fix for the missing pad_token)
tokenizer.pad_token = tokenizer.eos_token

# Read the text file
print("Reading and tokenizing the dataset...")
with open(input_file, "r", encoding="utf-8") as f:
    text_data = f.read()

# Tokenize the text data
tokenized_data = tokenizer(
    text_data.split("\n"),  # Split into lines or paragraphs
    truncation=True,
    padding=True,  # Pad sequences to the same length
    max_length=1024,  # Truncate long sequences
    return_tensors="pt",  # Return PyTorch tensors
)

# Create a PyTorch dataset
class TextDataset(torch.utils.data.Dataset):
    def __init__(self, encodings):
        self.input_ids = encodings["input_ids"]
        self.attention_mask = encodings["attention_mask"]

    def __len__(self):
        return len(self.input_ids)  # Match input_ids length

    def __getitem__(self, idx):
        # Return input_ids, attention_mask, and labels (labels = input_ids for causal language modeling)
        return {
            "input_ids": self.input_ids[idx],
            "attention_mask": self.attention_mask[idx],
            "labels": self.input_ids[idx],  # Labels are the same as input_ids for GPT-2
        }

dataset = TextDataset(tokenized_data)

# Step 2: Load the GPT-2 model
print("Loading the GPT-2 model...")
model = GPT2LMHeadModel.from_pretrained(r"M:\fine_tuned_gpt2_cont_2\checkpoint-1500")

tokenizer.pad_token = tokenizer.eos_token

# Step 3: Set up the training arguments
print("Setting up training arguments...")
training_args = TrainingArguments(
    output_dir="./fine_tuned_gpt2_cont_3",  # Directory to save the model
    overwrite_output_dir=True,
    num_train_epochs=1,  # Adjust as needed
    per_device_train_batch_size=1,  # Small batch size for low VRAM
    gradient_accumulation_steps=8,  # Simulates larger batch sizes
    save_steps=500,
    save_total_limit=2,
    logging_dir="./logs",
    logging_steps=100,
    learning_rate=5e-5,
    warmup_steps=500,
    weight_decay=0.01,
    fp16=True,  # Mixed precision for faster training
    optim="adamw_torch",  # Memory-efficient optimizer
    dataloader_pin_memory=False,  # Avoids memory overhead
    report_to="none",  # No external reporting
)

# Step 4: Create the Trainer
print("Initializing Trainer...")
trainer = Trainer(
    model=model,
    args=training_args,
    train_dataset=dataset,
    tokenizer=tokenizer,
)

# Step 5: Train the model
print("Starting fine-tuning...")
trainer.train()

# Save the fine-tuned model
print("Saving the fine-tuned model...")
model.save_pretrained("./fine_tuned_gpt2")
tokenizer.save_pretrained("./fine_tuned_gpt2")

print("Fine-tuning complete!")
