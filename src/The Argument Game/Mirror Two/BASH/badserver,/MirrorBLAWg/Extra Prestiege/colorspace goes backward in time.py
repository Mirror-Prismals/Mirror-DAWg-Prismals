def map_characters_to_colors(input_string):
    # Define a dictionary to map characters to their corresponding color hex codes
    color_codes = {
        'p': '#680164',  # Purple
        'b': '#0000FF',  # Blue
        'g': '#216216',  # Green
        'r': '#FF0000',  # Red
        'y': '#FFFF00',  # Yellow
        'c': '#00FFFF',  # Cyan
        'o': '#F5A000',  # Orange
        'l': '#8BB58F',  # Lime
        'e': '#808080',   # Extra (Grey)
        'd': '#808080',   # Grey
        'f': '#808080',   # Grey
        'h': '#808080',   # Grey
        'i': '#808080',   # Grey
        'j': '#808080',   # Grey
        'k': '#808080',   # Grey
        'm': '#808080',   # Grey
        'n': '#808080',   # Grey
        'q': '#808080',   # Grey
        's': '#808080',   # Grey
        't': '#808080',   # Grey
        'u': '#808080',   # Grey
        'v': '#808080',   # Grey
        'w': '#808080',   # Grey
        'x': '#808080',   # Grey
        'z': '#808080',   # Grey
        'a': '#808080',   # Grey
        'P': '#680164',  # Purple
        'B': '#0000FF',  # Blue
        'G': '#216216',  # Green
        'R': '#FF0000',  # Red
        'Y': '#FFFF00',  # Yellow
        'C': '#00FFFF',  # Cyan
        'O': '#F5A000',  # Orange
        'L': '#8BB58F',  # Lime
        'E': '#808080',  # Extra (Grey)
        'D': '#808080',  # Grey
        'F': '#808080',  # Grey
        'H': '#808080',  # Grey
        'I': '#808080',  # Grey
        'J': '#808080',  # Grey
        'K': '#808080',  # Grey
        'M': '#808080',  # Grey
        'N': '#808080',  # Grey
        'Q': '#808080',  # Grey
        'S': '#808080',  # Grey
        'T': '#808080',  # Grey
        'U': '#808080',  # Grey
        'V': '#808080',  # Grey
        'W': '#808080',  # Grey
        'X': '#808080',  # Grey
        'Z': '#808080',  # Grey
        'A': '#808080',  # Grey

    }
    
    # Initialize a list to store the hex codes
    hex_codes = []
    
    # Iterate over the characters in the input string
    for char in input_string:
        # If the character is found in the dictionary, append its hex code to the list
        if char in color_codes:
            hex_codes.append(color_codes[char])
    
    # Return the list of hex codes
    return hex_codes

# Test the function
input_string = "pbgryUWgle"
result = map_characters_to_colors(input_string)
print(result)
