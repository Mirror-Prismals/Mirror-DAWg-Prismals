import torch
from transformers import GPT2LMHeadModel, GPT2Tokenizer

# Load GPT-2 model and tokenizer
model_name = 'gpt2'
tokenizer = GPT2Tokenizer.from_pretrained(model_name)
model = GPT2LMHeadModel.from_pretrained(model_name)

# Function to generate words using GPT-2
def generate_words_gpt2(prompt, num_words=5):
    input_ids = tokenizer.encode(prompt, return_tensors='pt')
    generated_words = set()
    
    for _ in range(num_words * 3):  # Generate more sequences to ensure uniqueness
        output = model.generate(input_ids, max_length=len(input_ids[0]) + 1, num_return_sequences=1, pad_token_id=tokenizer.eos_token_id)
        generated_text = tokenizer.decode(output[0], skip_special_tokens=True)
        word = generated_text.split()[-1]
        if word not in prompt.split():  # Ensure the word is new
            generated_words.add(word)
        if len(generated_words) >= num_words:
            break
    
    return list(generated_words)[:num_words]

# Function to generate a fake crafting tree using GPT-2
def generate_fake_crafting_tree_gpt2(target_word, max_depth=3):
    # Initialize the tree
    tree = {target_word: []}
    
    def add_to_tree(current_word, depth, visited):
        if depth == 0 or current_word in visited:
            return
        visited.add(current_word)
        components = generate_words_gpt2(current_word, num_words=2)
        if not components:  # If no meaningful components, return
            return
        tree[current_word] = components
        for component in components:
            add_to_tree(component, depth - 1, visited)
    
    add_to_tree(target_word, max_depth, set())
    return tree

# Function to display the crafting tree
def display_crafting_tree(tree, word, depth=0):
    if word not in tree:
        return f"{' ' * depth * 2}{word}\n"
    result = f"{' ' * depth * 2}{word}\n"
    for component in tree[word]:
        result += display_crafting_tree(tree, component, depth + 1)
    return result

# Main function
def main():
    print("Welcome to the GPT-2 Enhanced Fake Crafting Tree Generator!")
    while True:
        word = input("Enter a word to see its crafting tree (or type 'exit' to quit): ").strip().lower()
        if word == 'exit':
            break
        try:
            max_depth = int(input("Enter the maximum depth for the crafting tree (default is 3): ") or 3)
        except ValueError:
            max_depth = 3
        tree = generate_fake_crafting_tree_gpt2(word, max_depth)
        print("\nCrafting Tree for '{}':".format(word))
        print(display_crafting_tree(tree, word))

if __name__ == "__main__":
    main()
