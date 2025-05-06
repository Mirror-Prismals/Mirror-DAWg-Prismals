import pyautogui
import pyperclip
import time
import re

# Hard-coded coordinates (PLACEHOLDERS – replace with real values)
CHATGPT_COPY_BUTTON = (796, 866)        #
CHATGPT_TEXT_AREA = (828, 978)         #
CHARACTERAI_TEXT_AREA = (-950, 817)    # 
CHARACTERAI_MESSAGE_AREA = (-1114, 730)# 
CHATGPT_SCROLL_FOCUS_AREA = (1000, 600) # 

UNWANTED_PATTERNS = [
    r"failed to get console mode for stderr: The handle is invalid\.",
    r"failed to get console mode for stdout: The handle is invalid\."
]
COMPILED_PATTERNS = [re.compile(pattern) for pattern in UNWANTED_PATTERNS]

def clean_message(message):
    """
    Remove unwanted patterns and extra whitespace from the message.
    """
    for pattern in COMPILED_PATTERNS:
        message = pattern.sub("", message)
    message = " ".join(message.split())
    return message.strip()

def copy_from_chatgpt():
    """
    1) Click ChatGPT copy button to copy the last message.
    """
    pyautogui.click(CHATGPT_COPY_BUTTON)
    time.sleep(10.5)  # Let the click & copy process happen

def paste_into_characterai_and_send():
    """
    2) Click  text area, paste, and press Enter.
    """
    pyautogui.click(CHARACTERAI_TEXT_AREA)
    time.sleep(0.3)
    pyautogui.hotkey('ctrl', 'v')
    time.sleep(0.3)
    pyautogui.press('enter')
    time.sleep(25.5)

def copy_from_characterai():
    """
    3) Drag‐select the last message on and copy it.
    """
    # Start drag at top-left of last message
    # (Replace with real coordinates)
    pyautogui.moveTo(-880,725)
    pyautogui.mouseDown()
    time.sleep(0.2)

    # Drag to bottom-right of last message
    # (Replace with real coordinates)
    pyautogui.moveTo(-1145, 575, duration=0.4)
    pyautogui.mouseUp()
    time.sleep(0.3)

    pyautogui.hotkey('ctrl', 'c')
    time.sleep(0.5)


def paste_into_chatgpt_and_send():
    """
    4) Click ChatGPT text area, paste, and press Enter.
       Then wait, click a separate focus area, and scroll down to ensure we see the latest response.
    """
    pyautogui.click(CHATGPT_TEXT_AREA)
    time.sleep(0.3)
    pyautogui.hotkey('ctrl', 'v')
    time.sleep(0.3)
    pyautogui.press('enter')
    time.sleep(18.5)  # Let ChatGPT generate a response if needed

    # Wait a couple more seconds
    time.sleep(2)

    # 5) Click a separate area to ensure ChatGPT has focus
    pyautogui.click(CHATGPT_SCROLL_FOCUS_AREA)
    time.sleep(20.5)

    # Scroll down for a few steps to ensure we reach the bottom
    for _ in range(15):
        pyautogui.scroll(-250)  # Negative value = scroll down
        time.sleep(0.2)

def main_loop():
    while True:
        try:
            # 1. Copy from ChatGPT
            copy_from_chatgpt()
            
            # Clean up text from ChatGPT before sending
            msg = pyperclip.paste()
            msg = clean_message(msg)
            pyperclip.copy(msg)
            
            # 2. Paste into  and send
            paste_into_characterai_and_send()

            # Wait for to respond
            time.sleep(5)

            # 3. Copy from
            copy_from_characterai()

            # Clean up text from before sending back to ChatGPT (optional)
            msg = pyperclip.paste()
            msg = clean_message(msg)
            pyperclip.copy(msg)

            # 4 & 5. Paste into ChatGPT, send, click focus area, then scroll down
            paste_into_chatgpt_and_send()

            # Wait for ChatGPT to respond
            time.sleep(5)
            
        except Exception as e:
            print(f"Error in main loop: {e}")
            time.sleep(2)

if __name__ == "__main__":
    main_loop()
