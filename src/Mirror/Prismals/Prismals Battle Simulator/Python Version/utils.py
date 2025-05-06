# utils.py
from move import Move
from prismal import Prismal

def show_stats(team1, team2, active1, active2):
    print("Team 1 Stats:")
    print(f"Active Prismal: {active1.name}")
    for prismal in team1:
        status = " (Active)" if prismal == active1 else ""
        types = ", ".join(prismal.types)
        print(f"{prismal.name}{status}: Types = {types}")
        print(f"HP = {prismal.hp}, Attack = {prismal.attack}, Defense = {prismal.defense}, Sp. Attack = {prismal.special_attack}, Sp. Defense = {prismal.special_defense}, Speed = {prismal.speed}")
        print("Moves:")
        for move in prismal.moves:
            move_type = "Special" if move.is_special() else "Physical"
            print(f"  {move.name}: Damage = {move.damage}, Accuracy = {move.accuracy}, Type = {move_type}")
    
    print("\nTeam 2 Stats:")
    print(f"Active Prismal: {active2.name}")
    for prismal in team2:
        status = " (Active)" if prismal == active2 else ""
        types = ", ".join(prismal.types)
        print(f"{prismal.name}{status}: Types = {types}")
        print(f"HP = {prismal.hp}, Attack = {prismal.attack}, Defense = {prismal.defense}, Sp. Attack = {prismal.special_attack}, Sp. Defense = {prismal.special_defense}, Speed = {prismal.speed}")
        print("Moves:")
        for move in prismal.moves:
            move_type = "Special" if move.is_special() else "Physical"
            print(f"  {move.name}: Damage = {move.damage}, Accuracy = {move.accuracy}, Type = {move_type}")

def select_move(prismal):
    if not prismal.moves:
        print("No moves available for this Prismal.")
        return None

    print("\nChoose a move:")
    for i, move in enumerate(prismal.moves):
        move_type = "Special" if move.is_special() else "Physical"
        print(f"{i + 1}. {move.name} - Damage: {move.damage}, Accuracy: {move.accuracy}, Type: {move_type}")

    try:
        move_index = int(input("Enter move number: ")) - 1

        if 0 <= move_index < len(prismal.moves):
            return prismal.moves[move_index]
        else:
            print("Invalid move number. Please try again.")
            return select_move(prismal)
    except ValueError:
        print("Invalid input. Please enter a number.")
        return select_move(prismal)

def switch_prismal(team):
    print("Choose a new Prismal for your team (or press 'b' to go back):")
    for i, prismal in enumerate(team):
        types = ", ".join(prismal.types)
        print(f"{i + 1}. {prismal.name} (Types: {types})")
    choice = input("Enter Prismal number: ")
    if choice == 'b':
        return team
    else:
        try:
            choice = int(choice) - 1
            if 0 <= choice < len(team):
                return team[choice:] + team[:choice]
            else:
                print("Invalid choice!")
                return team
        except ValueError:
            print("Invalid input!")
            return team
