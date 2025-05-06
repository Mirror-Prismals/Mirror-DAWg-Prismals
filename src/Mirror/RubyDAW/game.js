import * as THREE from 'three';
import { PointerLockControls } from 'three/addons/controls/PointerLockControls.js';

class RubyCaveFPS {
  constructor() {
    this.scene = new THREE.Scene();
    this.camera = new THREE.PerspectiveCamera(75, window.innerWidth / window.innerHeight, 0.1, 1000);
    this.renderer = new THREE.WebGLRenderer({ antialias: true });
    this.controls = null;
    this.moveSpeed = 0.15;
    this.velocity = new THREE.Vector3();
    this.direction = new THREE.Vector3();
    this.moveForward = false;
    this.moveBackward = false;
    this.moveLeft = false;
    this.moveRight = false;

    this.init();
  }

  init() {
    // Setup renderer
    this.renderer.setSize(window.innerWidth, window.innerHeight);
    this.renderer.setClearColor(0x110000);
    document.getElementById('game-container').appendChild(this.renderer.domElement);

    // Setup controls
    this.controls = new PointerLockControls(this.camera, document.body);
    
    document.addEventListener('click', () => {
      this.controls.lock();
    });

    // Movement controls
    document.addEventListener('keydown', (event) => this.onKeyDown(event));
    document.addEventListener('keyup', (event) => this.onKeyUp(event));

    // Create cave environment
    this.createCaveEnvironment();

    // Add lighting
    this.addLighting();

    // Position camera
    this.camera.position.y = 2;

    // Start animation loop
    this.animate();

    // Handle window resize
    window.addEventListener('resize', () => this.onWindowResize(), false);
  }

  createCaveEnvironment() {
    // Create crystalline walls
    const geometry = new THREE.IcosahedronGeometry(50, 1);
    const material = new THREE.MeshPhongMaterial({
      color: 0xff0000,
      specular: 0xffffff,
      shininess: 100,
      transparent: true,
      opacity: 0.9,
      side: THREE.BackSide
    });
    const cave = new THREE.Mesh(geometry, material);
    this.scene.add(cave);

    // Add crystal formations
    for (let i = 0; i < 50; i++) {
      const crystal = new THREE.Mesh(
        new THREE.ConeGeometry(Math.random() * 2 + 0.5, Math.random() * 4 + 2, 5),
        new THREE.MeshPhongMaterial({
          color: new THREE.Color(0.8 + Math.random() * 0.2, 0, 0),
          specular: 0xffffff,
          shininess: 100
        })
      );
      
      crystal.position.set(
        Math.random() * 80 - 40,
        Math.random() * 30 - 15,
        Math.random() * 80 - 40
      );
      
      crystal.rotation.set(
        Math.random() * Math.PI,
        Math.random() * Math.PI,
        Math.random() * Math.PI
      );
      
      this.scene.add(crystal);
    }
  }

  addLighting() {
    const ambientLight = new THREE.AmbientLight(0x330000);
    this.scene.add(ambientLight);

    const pointLight = new THREE.PointLight(0xff0000, 1, 100);
    pointLight.position.set(0, 10, 0);
    this.scene.add(pointLight);

    // Add multiple crystal lights
    for (let i = 0; i < 10; i++) {
      const light = new THREE.PointLight(0xff2200, 0.5, 20);
      light.position.set(
        Math.random() * 40 - 20,
        Math.random() * 20 - 10,
        Math.random() * 40 - 20
      );
      this.scene.add(light);
    }
  }

  onKeyDown(event) {
    switch (event.code) {
      case 'ArrowUp':
      case 'KeyW':
        this.moveForward = true;
        break;
      case 'ArrowDown':
      case 'KeyS':
        this.moveBackward = true;
        break;
      case 'ArrowLeft':
      case 'KeyA':
        this.moveLeft = true;
        break;
      case 'ArrowRight':
      case 'KeyD':
        this.moveRight = true;
        break;
    }
  }

  onKeyUp(event) {
    switch (event.code) {
      case 'ArrowUp':
      case 'KeyW':
        this.moveForward = false;
        this.velocity.z = 0; 
        break;
      case 'ArrowDown':
      case 'KeyS':
        this.moveBackward = false;
        this.velocity.z = 0; 
        break;
      case 'ArrowLeft':
      case 'KeyA':
        this.moveLeft = false;
        this.velocity.x = 0; 
        break;
      case 'ArrowRight':
      case 'KeyD':
        this.moveRight = false;
        this.velocity.x = 0; 
        break;
    }
  }

  onWindowResize() {
    this.camera.aspect = window.innerWidth / window.innerHeight;
    this.camera.updateProjectionMatrix();
    this.renderer.setSize(window.innerWidth, window.innerHeight);
  }

  updateMovement() {
    if (this.controls.isLocked) {
      this.direction.z = Number(this.moveForward) - Number(this.moveBackward);
      this.direction.x = Number(this.moveRight) - Number(this.moveLeft);
      this.direction.normalize();

      this.velocity.x = 0;
      this.velocity.z = 0;
      
      if (this.moveForward || this.moveBackward) {
        this.velocity.z = -this.direction.z * this.moveSpeed;
      }
      if (this.moveLeft || this.moveRight) {
        this.velocity.x = -this.direction.x * this.moveSpeed;
      }

      this.controls.moveRight(-this.velocity.x);
      this.controls.moveForward(-this.velocity.z);
    }
  }

  animate() {
    requestAnimationFrame(() => this.animate());
    this.updateMovement();
    
    this.camera.position.y += Math.sin(Date.now() * 0.002) * 0.001;
    
    this.renderer.render(this.scene, this.camera);
  }
}

new RubyCaveFPS();
