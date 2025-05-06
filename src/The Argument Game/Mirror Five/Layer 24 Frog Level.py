import sys
import os

# Add the taming-transformers directory to the sys.path
taming_transformers_path = os.path.abspath(os.path.dirname(__file__))
sys.path.append(taming_transformers_path)

import torch
from omegaconf import OmegaConf
from taming.models.vqgan import VQModel

def load_vqgan_model(config_path, checkpoint_path):
    config = OmegaConf.load(config_path)
    model = VQModel(**config.model.params)
    model.eval().requires_grad_(False)
    model.load_state_dict(torch.load(checkpoint_path, map_location="cpu")["state_dict"])
    return model

# Paths to the configuration file and checkpoint
config_path = os.path.join(taming_transformers_path, 'configs', 'model.yaml')
checkpoint_path = os.path.join(taming_transformers_path, 'vqgan_imagenet_f16_16384.ckpt')

# Load the model
model = load_vqgan_model(config_path, checkpoint_path)

print("Model loaded successfully")
