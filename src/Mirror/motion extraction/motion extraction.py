import cv2
import numpy as np
import mss
import time
from collections import deque

# Parameters
DELAY_US = 5111  # 200,000 microseconds = 0.2 seconds
OVERLAY_OPACITY = 0.01

def invert_colors(frame):
    return 255 - frame

def overlay_images(base, overlay, alpha):
    return cv2.addWeighted(base, 1.0 - alpha, overlay, alpha, 0)

def main():
    with mss.mss() as sct:
        capture_mon = sct.monitors[1]
        display_mon = sct.monitors[2]
        cap_width, cap_height = capture_mon['width'], capture_mon['height']
        disp_width, disp_height = display_mon['width'], display_mon['height']
        disp_left, disp_top = display_mon['left'], display_mon['top']
        frame_buffer = deque()

        window_name = "Motion Extraction"
        cv2.namedWindow(window_name, cv2.WND_PROP_FULLSCREEN)
        cv2.moveWindow(window_name, disp_left, disp_top)
        cv2.setWindowProperty(window_name, cv2.WND_PROP_FULLSCREEN, cv2.WINDOW_FULLSCREEN)

        print("Press ESC to quit.")
        while True:
            now_us = int(time.time() * 1_000_000)
            sct_img = sct.grab(capture_mon)
            frame = np.array(sct_img)[:, :, :3]
            frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)

            # Add current frame and timestamp to buffer
            frame_buffer.append((now_us, frame.copy()))

            # Remove old frames from buffer
            cutoff_us = now_us - 2_000_000  # keep up to 2 seconds history
            while frame_buffer and frame_buffer[0][0] < cutoff_us:
                frame_buffer.popleft()

            # Find the frame closest to the desired delay
            target_time = now_us - DELAY_US
            delayed_frame = None
            for t, f in reversed(frame_buffer):
                if t <= target_time:
                    delayed_frame = f
                    break
            if delayed_frame is not None:
                inverted = invert_colors(delayed_frame)
                output = overlay_images(frame, inverted, OVERLAY_OPACITY)
            else:
                output = frame

            # Resize output to display monitor size, preserving aspect ratio and filling with black if needed
            out_aspect = disp_width / disp_height
            frame_aspect = cap_width / cap_height
            if out_aspect > frame_aspect:
                new_h = disp_height
                new_w = int(frame_aspect * disp_height)
            else:
                new_w = disp_width
                new_h = int(disp_width / frame_aspect)
            resized = cv2.resize(output, (new_w, new_h), interpolation=cv2.INTER_AREA)
            final = np.zeros((disp_height, disp_width, 3), dtype=np.uint8)
            y_off = (disp_height - new_h) // 2
            x_off = (disp_width - new_w) // 2
            final[y_off:y_off+new_h, x_off:x_off+new_w] = resized

            cv2.imshow(window_name, final)
            key = cv2.waitKey(1)
            if key == 27:  # ESC
                break

        cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
