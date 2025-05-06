import pygame
import numpy as np
from math import sin, cos, radians

class Camera:
    def __init__(self, position=(0, 0, 0), yaw=0, pitch=0):
        self.position = np.array(position, dtype=float)
        self.yaw = yaw
        self.pitch = pitch
        self.speed = 0.1
        self.sensitivity = 0.1

    def get_view_matrix(self):
        yaw_rad = radians(self.yaw)
        pitch_rad = radians(self.pitch)
        
        front = np.array([
            cos(yaw_rad) * cos(pitch_rad),
            sin(pitch_rad),
            sin(yaw_rad) * cos(pitch_rad)
        ])
        front = front / np.linalg.norm(front)
        
        right = np.cross(front, [0, 1, 0])
        up = np.cross(right, front)
        
        return np.array([
            [right[0], up[0], -front[0], 0],
            [right[1], up[1], -front[1], 0],
            [right[2], up[2], -front[2], 0],
            [-np.dot(right, self.position), -np.dot(up, self.position), np.dot(front, self.position), 1]
        ], dtype=float)

    def handle_input(self, delta_time):
        keys = pygame.key.get_pressed()
        
        if keys[pygame.K_w]:
            self.position += self.get_front() * self.speed * delta_time
        if keys[pygame.K_s]:
            self.position -= self.get_front() * self.speed * delta_time
        if keys[pygame.K_a]:
            self.position -= self.get_right() * self.speed * delta_time
        if keys[pygame.K_d]:
            self.position += self.get_right() * self.speed * delta_time

    def handle_mouse(self, xrel, yrel):
        self.yaw += xrel * self.sensitivity
        self.pitch -= yrel * self.sensitivity

        if self.pitch > 89:
            self.pitch = 89
        if self.pitch < -89:
            self.pitch = -89

    def get_front(self):
        yaw_rad = radians(self.yaw)
        pitch_rad = radians(self.pitch)
        front = np.array([
            cos(yaw_rad) * cos(pitch_rad),
            sin(pitch_rad),
            sin(yaw_rad) * cos(pitch_rad)
        ])
        return front / np.linalg.norm(front)

    def get_right(self):
        front = self.get_front()
        right = np.cross(front, [0, 1, 0])
        return right / np.linalg.norm(right)
