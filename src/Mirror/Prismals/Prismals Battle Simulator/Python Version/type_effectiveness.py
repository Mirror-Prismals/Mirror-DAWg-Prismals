# type_effectiveness.py
import json

class TypeEffectiveness:
    def __init__(self, type_file, move_file):
        with open(type_file, 'r') as f:
            self.type_data = json.load(f)
        with open(move_file, 'r') as f:
            self.move_data = json.load(f)

    def get_effectiveness(self, move_type, defender_types):
        print(f"Move Type: {move_type}, Defender Types: {defender_types}")
        
        if isinstance(defender_types, str):
            defender_types = [defender_types]
        
        total_effectiveness = 1.0
        for defender_type in defender_types:
            if move_type == defender_type:
                continue  # Same type should be neutral, so we skip it

            weaknesses = self.type_data.get(defender_type, {}).get("weaknesses", [])
            resistances = self.type_data.get(defender_type, {}).get("resistances", [])
            
            if move_type in weaknesses:
                total_effectiveness *= 2.0  # Super effective
            elif move_type in resistances:
                total_effectiveness *= 0.5  # Not very effective
        
        return total_effectiveness

    def get_move_type(self, move_name):
        for move in self.move_data:
            if move['name'] == move_name:
                return move['type']
        return None

type_effectiveness = TypeEffectiveness('data/type_relationships.json', 'data/moves.json')

# For backwards compatibility
def get_effectiveness(move_type, defender_types):
    return type_effectiveness.get_effectiveness(move_type, defender_types)

def get_move_type(move_name):
    return type_effectiveness.get_move_type(move_name)
