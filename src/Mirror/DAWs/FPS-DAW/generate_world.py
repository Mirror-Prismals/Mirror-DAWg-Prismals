import os
import numpy as np
from noise import pnoise2

def generate_terrain_mesh(width, depth, scale, octaves, persistence, lacunarity, height_multiplier):
    """
    Generates a grid-based terrain mesh using Perlin noise.

    Parameters:
    - width: Number of vertices along the X-axis.
    - depth: Number of vertices along the Z-axis.
    - scale: Scaling factor for noise.
    - octaves: Number of noise layers.
    - persistence: Amplitude of each octave.
    - lacunarity: Frequency of each octave.
    - height_multiplier: Multiplier to scale the height variations.

    Returns:
    - vertices: List of vertex positions.
    - faces: List of face indices.
    - uvs: List of UV coordinates.
    """
    vertices = []
    uvs = []
    faces = []

    # Generate heightmap using Perlin noise
    heightmap = np.zeros((width, depth))
    for x in range(width):
        for z in range(depth):
            height = pnoise2(
                x / scale,
                z / scale,
                octaves=octaves,
                persistence=persistence,
                lacunarity=lacunarity,
                repeatx=1024,
                repeaty=1024,
                base=42
            )
            heightmap[x][z] = height * height_multiplier

    # Create vertices and UVs
    for x in range(width):
        for z in range(depth):
            y = heightmap[x][z]
            vertices.append((x, y, z))
            u = x / (width - 1)
            v = z / (depth - 1)
            uvs.append((u, v))

    # Create faces (two triangles per quad)
    for x in range(width - 1):
        for z in range(depth - 1):
            top_left = x * depth + z
            top_right = (x + 1) * depth + z
            bottom_left = x * depth + (z + 1)
            bottom_right = (x + 1) * depth + (z + 1)

            # First triangle
            faces.append((top_left + 1, bottom_left + 1, top_right + 1))
            # Second triangle
            faces.append((top_right + 1, bottom_left + 1, bottom_right + 1))

    return vertices, faces, uvs

def write_obj(filename, vertices, faces, uvs):
    """
    Writes the mesh data to an OBJ file.

    Parameters:
    - filename: Output OBJ file path.
    - vertices: List of vertex positions.
    - faces: List of face indices.
    - uvs: List of UV coordinates.
    """
    with open(filename, 'w') as f:
        f.write("# Procedurally Generated Terrain\n")
        for v in vertices:
            f.write(f"v {v[0]} {v[1]} {v[2]}\n")
        for uv in uvs:
            f.write(f"vt {uv[0]} {uv[1]}\n")
        f.write("usemtl terrain_material\n")
        f.write("s off\n")
        for face in faces:
            # OBJ format indices start at 1
            # Assuming each vertex has a corresponding UV
            f.write(f"f {face[0]}/{face[0]} {face[1]}/{face[1]} {face[2]}/{face[2]}\n")
    
    print(f"OBJ file saved to {filename}")

def generate_world_obj(filename, width=100, depth=100, scale=50, octaves=6, persistence=0.5, lacunarity=2.0, height_multiplier=10):
    """
    Generates a procedural terrain and writes it to an OBJ file.

    Parameters:
    - filename: Output OBJ file path.
    - width: Number of vertices along the X-axis.
    - depth: Number of vertices along the Z-axis.
    - scale: Scaling factor for noise.
    - octaves: Number of noise layers.
    - persistence: Amplitude of each octave.
    - lacunarity: Frequency of each octave.
    - height_multiplier: Multiplier to scale the height variations.
    """
    vertices, faces, uvs = generate_terrain_mesh(width, depth, scale, octaves, persistence, lacunarity, height_multiplier)
    write_obj(filename, vertices, faces, uvs)

if __name__ == "__main__":
    # Define the output OBJ filename
    OUTPUT_FILENAME = "assets/terrain.obj"
    
    # Terrain generation parameters
    WIDTH = 100               # Number of vertices along X-axis
    DEPTH = 100               # Number of vertices along Z-axis
    SCALE = 50                # Noise scale
    OCTAVES = 6               # Number of noise layers
    PERSISTENCE = 0.5         # Amplitude of each octave
    LACUNARITY = 2.0          # Frequency of each octave
    HEIGHT_MULTIPLIER = 10    # Terrain height scaling
    
    # Ensure the assets directory exists
    os.makedirs(os.path.dirname(OUTPUT_FILENAME), exist_ok=True)
    
    # Generate the terrain OBJ
    generate_world_obj(
        filename=OUTPUT_FILENAME,
        width=WIDTH,
        depth=DEPTH,
        scale=SCALE,
        octaves=OCTAVES,
        persistence=PERSISTENCE,
        lacunarity=LACUNARITY,
        height_multiplier=HEIGHT_MULTIPLIER
    )
