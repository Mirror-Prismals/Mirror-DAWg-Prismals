# simulator.py
import json
import random
from prismal import Prismal
from move import Move
from battle import Battle
from utils import show_stats, select_move, switch_prismal
import team  # Import team module for team management

def load_data():
    with open('data/prismals.json') as f:
        prismals_data = json.load(f)
    
    with open('data/moves.json') as f:
        moves_data = json.load(f)
    
    with open('data/teams.json') as f:
        teams_data = json.load(f)
    
    prismals = {}
    for p in prismals_data:
        prismals[p['name']] = Prismal(
            p['name'], p['hp'], p['attack'], p['defense'], p['special_attack'], p['special_defense'], p['speed'], p['types']
        )
    
    move_dict = {m['name']: Move(m['name'], m['damage'], m['accuracy'], m['is_special']) for m in moves_data}
    
    # Create teams using the team module
    team1 = team.create_team()
    team2 = team.create_team()
    
    for name, moves in teams_data['team1'].items():
        prismal = prismals[name]
        prismal.moves = [move_dict[m] for m in moves]
        team.add_prismal(team1, prismal)
    
    for name, moves in teams_data['team2'].items():
        prismal = prismals[name]
        prismal.moves = [move_dict[m] for m in moves]
        team.add_prismal(team2, prismal)
    
    return team1, team2

def execute_turn(battle, move1, move2):
    if battle.active1.speed > battle.active2.speed:
        first, second = battle.active1, battle.active2
        first_move, second_move = move1, move2
    elif battle.active1.speed < battle.active2.speed:
        first, second = battle.active2, battle.active1
        first_move, second_move = move2, move1
    else:
        # If speeds are equal, randomly choose who goes first
        if random.choice([True, False]):
            first, second = battle.active1, battle.active2
            first_move, second_move = move1, move2
        else:
            first, second = battle.active2, battle.active1
            first_move, second_move = move2, move1

    print(f"{first.name} attacks first!")
    second.take_damage(first_move, first)
    if second.is_alive():
        print(f"{second.name} attacks!")
        first.take_damage(second_move, second)
    else:
        print(f"{second.name} has been defeated!")

    if not battle.active1.is_alive():
        battle.handle_defeat(battle.active1)
    if not battle.active2.is_alive():
        battle.handle_defeat(battle.active2)

def main():
    team1, team2 = load_data()
    
    battle = Battle(team1, team2)
    
    while True:
        print(battle)
        
        if battle.is_battle_over():
            winner = 'Team 1' if not team1 else 'Team 2'
            print(f"{winner} wins!")
            break

        print("Team 1's turn")
        print(f"Active Prismal: {battle.active1.name}")
        print("Available moves:")
        for move in battle.active1.moves:
            move_type = "Special" if move.is_special() else "Physical"
            print(f" - {move.name} (Damage: {move.damage}, Accuracy: {move.accuracy}, Type: {move_type})")
        
        action1 = input(f"Team 1: Do you want to (a)ttack, (s)witch Prismal, or (st)ats? ")
        if action1 == 's':
            new_prismal = switch_prismal(team1)
            if new_prismal:
                battle.switch_prismal(1, new_prismal)
            move1 = None
        elif action1 == 'st':
            show_stats(team1, team2, battle.active1, battle.active2)
            continue
        elif action1 == 'a':
            move1 = select_move(battle.active1)
        else:
            print("Invalid action. Skipping turn.")
            move1 = None

        print("\nTeam 2's turn")
        print(f"Active Prismal: {battle.active2.name}")
        print("Available moves:")
        for move in battle.active2.moves:
            move_type = "Special" if move.is_special() else "Physical"
            print(f" - {move.name} (Damage: {move.damage}, Accuracy: {move.accuracy}, Type: {move_type})")
        
        action2 = input(f"Team 2: Do you want to (a)ttack, (s)witch Prismal, or (st)ats? ")
        if action2 == 's':
            new_prismal = switch_prismal(team2)
            if new_prismal:
                battle.switch_prismal(2, new_prismal)
            move2 = None
        elif action2 == 'st':
            show_stats(team1, team2, battle.active1, battle.active2)
            continue
        elif action2 == 'a':
            move2 = select_move(battle.active2)
        else:
            print("Invalid action. Skipping turn.")
            move2 = None

        if move1 and move2:
            execute_turn(battle, move1, move2)
        elif move1:
            battle.active2.take_damage(move1, battle.active1)
            if not battle.active2.is_alive():
                battle.handle_defeat(battle.active2)
        elif move2:
            battle.active1.take_damage(move2, battle.active2)
            if not battle.active1.is_alive():
                battle.handle_defeat(battle.active1)

if __name__ == "__main__":
    main()
