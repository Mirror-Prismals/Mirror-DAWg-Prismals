# move.py
from type_effectiveness import get_effectiveness, get_move_type

class Move:
    def __init__(self, name, damage, accuracy, is_special):
        self.name = name
        self.damage = damage
        self.accuracy = accuracy
        self._is_special = is_special

    def calculate_damage(self, attacker_type, defender_type):
        move_type = get_move_type(self.name)
        effectiveness = get_effectiveness(move_type, defender_type)
        return self.damage * effectiveness

    def is_special(self):
        return self._is_special
