# team.py
from prismal import Prismal

def create_team():
    return []

def add_prismal(team, prismal):
    team.append(prismal)

def remove_prismal(team, prismal):
    if prismal in team:
        team.remove(prismal)

def list_prismals(team):
    return [str(prismal) for prismal in team]

def get_prismal_by_name(team, name):
    for prismal in team:
        if prismal.name == name:
            return prismal
    return None
