# prismal.py
from type_effectiveness import type_effectiveness
from move import Move

class Prismal:
    def __init__(self, name, hp, attack, defense, special_attack, special_defense, speed, types):
        self.name = name
        self.hp = hp
        self.max_hp = hp
        self.attack = attack
        self.defense = defense
        self.special_attack = special_attack
        self.special_defense = special_defense
        self.speed = speed
        self.types = types if isinstance(types, list) else [types]
        self.moves = []
        self.fainted = False

    def add_move(self, move):
        self.moves.append(move)

    def take_damage(self, move, attacker):
        if not isinstance(move, Move):
            print(f"Error: Expected a Move object but got {type(move)}")
            return

        move_type = type_effectiveness.get_move_type(move.name)
        
        # Calculate effectiveness for multiple types
        effectiveness_multiplier = 1
        for defender_type in self.types:
            effectiveness_multiplier *= type_effectiveness.get_effectiveness(move_type, defender_type)
        
        # Determine if the move is physical or special
        is_special = move.is_special()
        
        # Calculate stat multiplier based on move type
        if is_special:
            stat_multiplier = attacker.special_attack / self.special_defense
        else:
            stat_multiplier = attacker.attack / self.defense
        
        # Calculate damage
        # New formula: Power * STAB * Effectiveness * Stat Multiplier
        damage = move.damage
        
        # Apply STAB (Same Type Attack Bonus)
        stab_multiplier = 1.15 if move_type in attacker.types else 1.0
        
        # Apply all multipliers
        damage *= stab_multiplier * effectiveness_multiplier * stat_multiplier
        
        damage = max(1, int(damage))  # Ensure at least 1 damage is dealt
        
        self.hp -= damage
        self.hp = max(0, self.hp)  # Ensure HP doesn't go below 0

        print(f"{self.name} took {damage:.2f} damage from {move.name}")
        print(f"- Base Power: {move.damage}")
        print(f"- STAB: {stab_multiplier:.2f}x")
        print(f"- Type Effectiveness: {effectiveness_multiplier:.2f}x")
        print(f"- Stat Multiplier: {stat_multiplier:.2f}x")
        print(f"- Move Type: {'Special' if is_special else 'Physical'}")
        
        if self.hp == 0:
            self.fainted = True
            print(f"{self.name} has fainted!")

    def is_alive(self):
        return self.hp > 0

    def __str__(self):
        return f"{self.name} (HP: {self.hp}/{self.max_hp}, Attack: {self.attack}, Defense: {self.defense}, Sp. Attack: {self.special_attack}, Sp. Defense: {self.special_defense}, Speed: {self.speed}, Types: {', '.join(self.types)})"
