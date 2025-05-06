    # main.py
from titleCard import launch_title_card
from mainDashboard import launch_main_hub

def main():
    # Start with the title card, then transition to the main hub
    launch_title_card()  # This will block for its duration
    launch_main_hub()    # Then, start the main hub visualizer

if __name__ == "__main__":
    main()
