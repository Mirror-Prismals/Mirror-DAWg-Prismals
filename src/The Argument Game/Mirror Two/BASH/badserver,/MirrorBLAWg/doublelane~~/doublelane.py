import os

class DoubleLane:
    def __init__(self):
        self.scripts = [f for f in os.listdir() if f.endswith('.py') and f not in ['double_main.py', 'DoubleLane.py']]

    def main_menu(self):
        print("Welcome to DoubleLane!")
        print("Choose options to run multiple scripts:")
        for i, script in enumerate(self.scripts):
            print(f"{i + 1}. {script}")
        
        choices = input(f"Enter your choices (e.g., 1,2,3 for first three scripts): ")
        selected_scripts = self.parse_choices(choices)
        
        if selected_scripts:
            self.run_selected_scripts(selected_scripts)
        else:
            print("Invalid choices. Please enter valid numbers separated by commas.")

    def parse_choices(self, choices):
        try:
            selected_indices = [int(choice.strip()) - 1 for choice in choices.split(',')]
            selected_scripts = [self.scripts[i] for i in selected_indices if 0 <= i < len(self.scripts)]
            return selected_scripts
        except ValueError:
            return []

    def run_selected_scripts(self, selected_scripts):
        for script in selected_scripts:
            print(f"Running {script}...")
            os.system(f"python {script}")

if __name__ == "__main__":
    double_lane = DoubleLane()
    double_lane.main_menu()
