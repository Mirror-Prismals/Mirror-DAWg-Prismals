def convert_to_three_pair_hex_palette(values, format):
    if len(values) != len(format):
        raise ValueError("Number of values must match the number of channels in the format")
    
    hex_strings = []
    
    for value, channel in zip(values, format):
        # Check if the channel is within the expected range 'a' to 'j'
        if 'a' <= channel <= 'j':
            # Normalize the value by multiplying by 1000 and converting to an integer
            int_value = int(round(value * 1000))  # Round to handle floating-point imprecision
            
            # Convert the integer value to a hexadecimal string, ensuring it's at least three digits long
            hex_string = f"{int_value:X}".zfill(3)
            
            # Append the hex string to the list
            hex_strings.append(hex_string)
    
    # Join all hex strings together with a '#' prefix
    return "#" + "".join(hex_strings)

def test_convert_to_three_pair_hex_palette():
    # Test cases with expected outputs
    test_cases = [
        
        ([1, 2, 3, 4, 5, 6, 7, 8, 8, 10], "pbgrycogle", "")
        
    ]
    
    for input_values, format, expected in test_cases:
        actual = convert_to_three_pair_hex_palette(input_values, format)
        print(f"Format: {format}, {', '.join(map(str, input_values))}, = {actual}")
    
    print("All test cases pass")

# Run the test function
test_convert_to_three_pair_hex_palette()
