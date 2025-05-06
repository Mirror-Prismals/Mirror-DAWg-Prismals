#Dev Note: this is OCPNORM67.py
import tkinter as tk
from tkinter import messagebox, scrolledtext

def hex_to_decimal(hex_value):
    if '-' in hex_value:
        parts = hex_value.split('-')
        if parts[0] == '':  # Case like "-80"
            return -int(parts[1], 16)
        elif parts[1] == '':  # Case like "80-"
            return int(parts[0], 16)
        else:  # Case like "F0-8"
            return int(parts[0], 16) - int(parts[1], 16)
    else:
        return int(hex_value, 16)

def expand_hyphenated(hex_value):
    if '-' in hex_value:
        parts = hex_value.split('-')
        if len(parts[0]) == 1:
            return f"{parts[0]}0-{parts[1]}"
        elif len(parts[1]) == 1:
            return f"{parts[0]}-{parts[1]}0"
    return hex_value

def simplify_value(value):
    if isinstance(value, str) and '-' in value:
        parts = value.split('-')
        return int(parts[0]) - int(parts[1])
    return value

def rationalize_values(decimal_channels):
    simplified_values = {color: simplify_value(value) for color, value in decimal_channels.items()}
    min_value = min(simplified_values.values())
    
    if min_value >= 0:
        return simplified_values
    
    return {color: value - min_value for color, value in simplified_values.items()}

def decimal_to_hex(value):
    return f"{value % 256:02X}"

def expand_hex(custom_hex):
    try:
        if not custom_hex.startswith("%~") or not all(c in "%~08Ff-" for c in custom_hex):
            raise ValueError("Invalid input format")

        hex_content = custom_hex[2:]
        if hex_content.count('-') != 1 or len(hex_content.replace('-', '')) != 6:
            raise ValueError("Input must contain exactly one negative sign and be six characters long")

        neg_pos = hex_content.find('-')
        hex_chars = hex_content.replace('-', '')

        def process_pair(first, second, neg_pos, pair_start):
            if neg_pos == pair_start:
                return f"-{first}{second}"
            elif neg_pos == pair_start + 1:
                return f"{first}-{second}"
            else:
                return f"{first}{second}"

        channels = {
            'Red': process_pair(hex_chars[0], hex_chars[1], neg_pos, 0),
            'Orange': process_pair(hex_chars[1], hex_chars[2], neg_pos, 1),
            'Green': process_pair(hex_chars[2], hex_chars[3], neg_pos, 2),
            'Cyan': process_pair(hex_chars[3], hex_chars[4], neg_pos, 3),
            'Blue': process_pair(hex_chars[4], hex_chars[5], neg_pos, 4),
            'Purple': (f"{hex_chars[5]}-{hex_chars[0]}" if neg_pos == 0 else
                       f"-{hex_chars[5]}{hex_chars[0]}" if neg_pos == 5 else
                       f"{hex_chars[5]}{hex_chars[0]}")
        }

        expanded_channels = {color: expand_hyphenated(value) for color, value in channels.items()}
        decimal_channels = {color: hex_to_decimal(expanded_channels[color]) for color in channels}
        rationalized_channels = rationalize_values(decimal_channels)
        hexed_channels = {color: decimal_to_hex(value) for color, value in rationalized_channels.items()}

        rgb_hex = hexed_channels['Red'] + hexed_channels['Green'] + hexed_channels['Blue']
        ocp_hex = hexed_channels['Orange'] + hexed_channels['Cyan'] + hexed_channels['Purple']

        return {
            'original': channels,
            'expanded': expanded_channels,
            'decimal': decimal_channels,
            'rationalized': rationalized_channels,
            'hexed': hexed_channels,
            'rgb_hex': rgb_hex,
            'ocp_hex': ocp_hex
        }
    except Exception as e:
        messagebox.showerror("Error", str(e))
        return None

def on_expand():
    custom_hex = entry.get()
    if not all(c in "%~08Ff-" for c in custom_hex):
        messagebox.showerror("Error", "Invalid characters in input. Only %~08Ff- are allowed.")
        return
    
    result = expand_hex(custom_hex)
    if result:
        original = result['original']
        expanded = result['expanded']
        decimal = result['decimal']
        rationalized = result['rationalized']
        hexed = result['hexed']
        rgb_hex = result['rgb_hex']
        ocp_hex = result['ocp_hex']

        result_text = f"Original:\n" + " ".join([f"{k}: {v}" for k, v in original.items()]) + "\n\n"
        result_text += f"Expanded:\n" + " ".join([f"{k}: {v}" for k, v in expanded.items()]) + "\n\n"
        result_text += f"Decimal:\n" + " ".join([f"{k}: {v}" for k, v in decimal.items()]) + "\n\n"
        result_text += f"Rationalized:\n" + " ".join([f"{k}: {v}" for k, v in rationalized.items()]) + "\n\n"
        result_text += f"Hexed:\n" + " ".join([f"{k}: {v}" for k, v in hexed.items()]) + "\n\n"
        result_text += f"RGBHEX: {rgb_hex}\n"
        result_text += f"OCPHEX: {ocp_hex}\n"
        
        result_text_widget.delete(1.0, tk.END)
        result_text_widget.insert(tk.END, result_text)

        rgb_label.config(text=f"RGBHEX: {rgb_hex}")
        ocp_label.config(text=f"OCPHEX: {ocp_hex}")
        rgb_color.config(bg=f"#{rgb_hex}")
        ocp_color.config(bg=f"#{ocp_hex}")

# Setup the main application window
root = tk.Tk()
root.title("Subhex! %~XXX-XXX")

# Create a frame for input and output
frame = tk.Frame(root)
frame.pack(pady=20, padx=20, fill=tk.BOTH, expand=True)

# Input label and entry
input_label = tk.Label(frame, text='''Enter custom hex code, 
        %~-XXXXXX
        %~X-XXXXX
        %~XX-XXXX
        %~XXX-XXX
        %~XXXX-XX
        %~XXXXX-X
        %~XXXXXX-''')
input_label.grid(row=0, column=0, padx=10, pady=10)
entry = tk.Entry(frame, width=20)
entry.grid(row=0, column=1, padx=10, pady=10)

# Expand button
expand_button = tk.Button(frame, text="Expand", command=on_expand)
expand_button.grid(row=0, column=2, padx=10, pady=10)

# Result text widget with scrollbar
result_text_widget = scrolledtext.ScrolledText(frame, wrap=tk.WORD, width=60, height=20)
result_text_widget.grid(row=1, column=0, columnspan=3, padx=10, pady=10, sticky="nsew")

# RGB and OCP labels and color displays
rgb_label = tk.Label(frame, text="RGBHEX: ")
rgb_label.grid(row=2, column=0, padx=10, pady=10)
rgb_color = tk.Label(frame, width=10, height=2)
rgb_color.grid(row=2, column=1, padx=10, pady=10)

ocp_label = tk.Label(frame, text="OCPHEX: ")
ocp_label.grid(row=3, column=0, padx=10, pady=10)
ocp_color = tk.Label(frame, width=10, height=2)
ocp_color.grid(row=3, column=1, padx=10, pady=10)

# Configure grid to allow expansion
frame.grid_rowconfigure(1, weight=1)
frame.grid_columnconfigure(0, weight=1)
frame.grid_columnconfigure(1, weight=1)
frame.grid_columnconfigure(2, weight=1)

# Exit button
exit_button = tk.Button(root, text="Exit", command=root.quit)
exit_button.pack(pady=20)

# Run the main loop
root.mainloop()
