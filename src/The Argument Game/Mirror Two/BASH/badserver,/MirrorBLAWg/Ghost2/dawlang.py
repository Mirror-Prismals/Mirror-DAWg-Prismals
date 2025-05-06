from pynput import mouse

# Function to handle mouse movements
def on_move(x, y):
    print(f'Mouse moved to ({x}, {y})')

# Function to handle mouse clicks
def on_click(x, y, button, pressed):
    if pressed:
        print(f'Mouse clicked at ({x}, {y}) with {button}')
    else:
        print(f'Mouse released at ({x}, {y}) with {button}')

# Function to handle mouse scrolls
def on_scroll(x, y, dx, dy):
    print(f'Scroll at ({x}, {y}) with delta ({dx}, {dy})')

# Collect mouse events
with mouse.Listener(on_move=on_move, on_click=on_click, on_scroll=on_scroll) as listener:
    listener.join()
