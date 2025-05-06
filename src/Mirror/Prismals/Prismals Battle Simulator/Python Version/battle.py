# battle.py
from team import remove_prismal
from type_effectiveness import type_effectiveness
from move import Move

class Battle:
    def __init__(self, team1, team2):
        self.team1 = team1
        self.team2 = team2
        self.active1 = team1[0] if team1 else None
        self.active2 = team2[0] if team2 else None
        self.turn = 1

    def perform_turn(self, move1=None, move2=None):
        if self.turn % 2 == 1:
            attacker, defender = self.active1, self.active2
            move = move1 if move1 else self.ai_choose_move(attacker)
        else:
            attacker, defender = self.active2, self.active1
            move = move2 if move2 else self.ai_choose_move(attacker)

        if not isinstance(move, Move):
            print(f"Error: Expected a Move object but got {type(move)}")
            return

        defender.take_damage(move, attacker)
        
        if not defender.is_alive():
            self.handle_defeat(defender)

        self.turn += 1

    def handle_defeat(self, defeated_prismal):
        team = self.team1 if defeated_prismal in self.team1 else self.team2
        remove_prismal(team, defeated_prismal)
        
        if not team:
            print(f"Team {'1' if team == self.team1 else '2'} has been defeated!")
            return
        
        print(f"Choose a new Prismal for Team {'1' if team == self.team1 else '2'}:")
        for i, prismal in enumerate(team):
            print(f"{i + 1}. {prismal.name}")
        
        choice = int(input("Enter the number of the Prismal: ")) - 1
        if 0 <= choice < len(team):
            new_active = team[choice]
            if team == self.team1:
                self.active1 = new_active
            else:
                self.active2 = new_active
            print(f"{new_active.name} has been sent out!")
        else:
            print("Invalid choice. Please try again.")
            self.handle_defeat(defeated_prismal)

    def is_battle_over(self):
        return not self.team1 or not self.team2

    def __str__(self):
        team1_names = ", ".join(p.name for p in self.team1 if p.is_alive())
        team2_names = ", ".join(p.name for p in self.team2 if p.is_alive())
        return f"Battle between Team 1 ({team1_names}) and Team 2 ({team2_names})"

    def ai_choose_move(self, prismal):
        # Simple AI: choose the move with the highest damage
        return max(prismal.moves, key=lambda move: move.damage)
