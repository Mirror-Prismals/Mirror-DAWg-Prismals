# mirror.py
from ursina import Entity, color, time, Vec3
from ursina import camera  # if you need it
import math

class AIFPSController(Entity):
    """
    A minimal AI 'FPS bot' that follows a list of scripted actions.
    Each action has: 
       'type': move_forward, move_back, turn_left, turn_right, shoot, etc.
       'duration': how many seconds to keep doing this action
       'value': optional parameter (like speed or angle)
    """
    def __init__(self, position=(0,0,0), speed=5, rotation_speed=60):
        super().__init__(
            position=position,
            model='cube',
            color=color.azure,
            scale=(1,2,1),
            collider='box'
        )
        self.speed = speed
        self.rotation_speed = rotation_speed
        self.script = []
        self.current_action_index = 0
        self.action_time_left = 0

    def update(self):
        """Called every frame. We check the current action and perform it."""
        if self.current_action_index < len(self.script):
            action = self.script[self.current_action_index]

            # If starting a new action, reset the action_time_left
            if self.action_time_left <= 0:
                self.action_time_left = action.get('duration', 1)

            # Perform the action logic
            self.perform_action(action)

            # Decrement the timer
            self.action_time_left -= time.dt
            if self.action_time_left <= 0:
                self.current_action_index += 1

    def perform_action(self, action):
        """Interpret and execute the current action (move, turn, shoot, etc)."""
        act_type = action.get('type', '')
        value = action.get('value', 0)

        if act_type == 'move_forward':
            self.position += self.forward * self.speed * time.dt
        elif act_type == 'move_back':
            self.position -= self.forward * self.speed * time.dt
        elif act_type == 'strafe_left':
            self.position -= self.right * self.speed * time.dt
        elif act_type == 'strafe_right':
            self.position += self.right * self.speed * time.dt
        elif act_type == 'turn_left':
            self.rotation_y += self.rotation_speed * time.dt
        elif act_type == 'turn_right':
            self.rotation_y -= self.rotation_speed * time.dt
        elif act_type == 'shoot':
            # Placeholder for shooting logic (e.g., spawn a bullet or do a raycast)
            print("Bot is shooting!")
        # Add more action types as needed

    def load_script(self, script):
        """Assign a list of dict actions to the AI bot."""
        self.script = script
        self.current_action_index = 0
        self.action_time_left = 0

# A sample script to show how the bot might move.
sample_script = [
    {'type': 'move_forward', 'duration': 2},
    {'type': 'turn_left',    'duration': 1},
    {'type': 'move_forward', 'duration': 2},
    {'type': 'shoot',        'duration': 0.5},
    {'type': 'strafe_left',  'duration': 2},
    {'type': 'turn_right',   'duration': 1},
    {'type': 'move_forward', 'duration': 3},
]
