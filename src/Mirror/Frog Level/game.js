import * as THREE from 'three';
import { PointerLockControls } from 'three/addons/controls/PointerLockControls.js';
import { Water } from 'three/addons/objects/Water.js';
import { Sky } from 'three/addons/objects/Sky.js';

class IslandFPS {
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
    this.water = null;
    this.sun = new THREE.Vector3();
    this.clock = new THREE.Clock();

    this.init();
  }

  init() {
    this.renderer.setSize(window.innerWidth, window.innerHeight);
    this.renderer.toneMapping = THREE.ACESFilmicToneMapping;
    this.renderer.toneMappingExposure = 0.5; 
    this.renderer.setClearColor(0x000022); 
    document.getElementById('game-container').appendChild(this.renderer.domElement);

    this.controls = new PointerLockControls(this.camera, document.body);
    
    document.addEventListener('click', () => {
      this.controls.lock();
    });

    document.addEventListener('keydown', (event) => this.onKeyDown(event));
    document.addEventListener('keyup', (event) => this.onKeyUp(event));

    this.camera.position.set(0, 2, 5);

    // Create island
    this.createIsland();
    
    // Create water
    this.createWater();
    
    // Create sky
    this.createSky();
    
    // Create palm tree
    this.createPalmTree();

    this.createOceanParticles();

    // Add moonlight (dim bluish light)
    const moonLight = new THREE.DirectionalLight(0x557799, 0.3);
    moonLight.position.set(2, 10, 2);
    this.scene.add(moonLight);

    // Very dim ambient light for night
    const ambientLight = new THREE.AmbientLight(0x222244, 0.2);
    this.scene.add(ambientLight);

    this.animate();
    window.addEventListener('resize', () => this.onWindowResize(), false);
  }

  createIsland() {
    // Create noise texture for sand
    const noiseTexture = new THREE.TextureLoader().load('https://raw.githubusercontent.com/mrdoob/three.js/master/examples/textures/terrain/grasslight-big.jpg'); // idk who this is < blame claude
    noiseTexture.wrapS = noiseTexture.wrapT = THREE.RepeatWrapping;
    noiseTexture.repeat.set(5, 5);

    // Create island geometry with more detail
    const geometry = new THREE.CircleGeometry(10, 64);
    const material = new THREE.MeshStandardMaterial({ 
      color: 0x403015, 
      roughness: 0.8,
      metalness: 0.1,
      map: noiseTexture,
      bumpMap: noiseTexture,
      bumpScale: 0.2
    });
    const island = new THREE.Mesh(geometry, material);
    island.rotation.x = -Math.PI / 2;
    this.scene.add(island);

    // Add grass patch with noise
    const grassTexture = new THREE.TextureLoader().load('https://raw.githubusercontent.com/mrdoob/three.js/master/examples/textures/terrain/grasslight-big.jpg');
    grassTexture.wrapS = grassTexture.wrapT = THREE.RepeatWrapping;
    grassTexture.repeat.set(2, 2);

    const grassGeometry = new THREE.CircleGeometry(5, 64);
    const grassMaterial = new THREE.MeshStandardMaterial({ 
      color: 0x1a3311, 
      roughness: 0.8,
      metalness: 0.1,
      map: grassTexture,
      bumpMap: grassTexture,
      bumpScale: 0.3
    });
    const grass = new THREE.Mesh(grassGeometry, grassMaterial);
    grass.rotation.x = -Math.PI / 2;
    grass.position.y = 0.01;
    this.scene.add(grass);
  }

  createWater() {
    const waterGeometry = new THREE.PlaneGeometry(10000, 10000, 100, 100);
    this.water = new Water(waterGeometry, {
      textureWidth: 1024,
      textureHeight: 1024,
      waterNormals: new THREE.TextureLoader().load('https://raw.githubusercontent.com/mrdoob/three.js/master/examples/textures/waternormals.jpg', function(texture) {
        texture.wrapS = texture.wrapT = THREE.RepeatWrapping;
        texture.repeat.set(4, 4);
      }),
      sunDirection: new THREE.Vector3(),
      sunColor: 0x555555, 
      waterColor: 0x001133, 
      distortionScale: 3.5,
      fog: false,
      alpha: 0.8
    });
    this.water.rotation.x = -Math.PI / 2;
    this.water.position.y = -0.5;
    this.scene.add(this.water);
  }

  createOceanParticles() {
    const particleCount = 1000;
    const particles = new THREE.BufferGeometry();
    const positions = new Float32Array(particleCount * 3);
    const velocities = new Float32Array(particleCount);

    for (let i = 0; i < particleCount; i++) {
      positions[i * 3] = Math.random() * 100 - 50;
      positions[i * 3 + 1] = -0.5 + Math.random() * 0.5;
      positions[i * 3 + 2] = Math.random() * 100 - 50;
      velocities[i] = Math.random() * 0.02;
    }

    particles.setAttribute('position', new THREE.BufferAttribute(positions, 3));
    particles.setAttribute('velocity', new THREE.BufferAttribute(velocities, 1));

    const particleMaterial = new THREE.PointsMaterial({
      color: 0x113366, 
      size: 0.1,
      transparent: true,
      opacity: 0.4
    });

    this.oceanParticles = new THREE.Points(particles, particleMaterial);
    this.scene.add(this.oceanParticles);
  }

  createSky() {
    const sky = new Sky();
    sky.scale.setScalar(10000);
    this.scene.add(sky);

    const skyUniforms = sky.material.uniforms;
    skyUniforms['turbidity'].value = 1;  
    skyUniforms['rayleigh'].value = 0.5;   
    skyUniforms['mieCoefficient'].value = 0.005;
    skyUniforms['mieDirectionalG'].value = 0.7;

    const parameters = {
      elevation: 180, 
      azimuth: 180
    };

    const phi = THREE.MathUtils.degToRad(90 - parameters.elevation);
    const theta = THREE.MathUtils.degToRad(parameters.azimuth);
    this.sun.setFromSphericalCoords(1, phi, theta);
    skyUniforms['sunPosition'].value.copy(this.sun);
    this.water.material.uniforms['sunDirection'].value.copy(this.sun).normalize();
  }

  createPalmTree() {
    const trunkGeometry = new THREE.CylinderGeometry(0.15, 0.2, 4, 8);
    const trunkTexture = new THREE.TextureLoader().load('https://raw.githubusercontent.com/mrdoob/three.js/master/examples/textures/wood/hardwood2_diffuse.jpg');
    trunkTexture.wrapS = trunkTexture.wrapT = THREE.RepeatWrapping;
    trunkTexture.repeat.set(2, 4);
    
    const trunkMaterial = new THREE.MeshStandardMaterial({ 
      color: 0x8B4513,
      map: trunkTexture,
      roughness: 0.9,
      metalness: 0.1
    });
    
    const trunk = new THREE.Mesh(trunkGeometry, trunkMaterial);
    trunk.position.set(0, 2, 0);
    trunk.rotation.z = Math.random() * 0.2 - 0.1;
    this.scene.add(trunk);

    // Create more realistic leaves
    const leavesGroup = new THREE.Group();
    const leafShape = new THREE.Shape();
    leafShape.moveTo(0, 0);
    leafShape.quadraticCurveTo(0.5, 1, 0, 2);
    leafShape.quadraticCurveTo(-0.5, 1, 0, 0);

    const leafGeometry = new THREE.ShapeGeometry(leafShape);
    const leafMaterial = new THREE.MeshStandardMaterial({ 
      color: 0x2d5a27,
      roughness: 0.8,
      metalness: 0.1,
      side: THREE.DoubleSide
    });

    for (let i = 0; i < 12; i++) {
      const leaf = new THREE.Mesh(leafGeometry, leafMaterial);
      leaf.scale.set(1 + Math.random() * 0.5, 1.5 + Math.random() * 0.5, 1);
      leaf.position.y = 4;
      leaf.rotation.z = (i * Math.PI / 6) + (Math.random() * 0.2 - 0.1);
      leaf.rotation.x = Math.PI / 4 + (Math.random() * 0.2 - 0.1);
      leaf.rotation.y = Math.random() * 0.2 - 0.1;
      leavesGroup.add(leaf);
    }
    
    leavesGroup.position.y = -0.5;
    this.scene.add(leavesGroup);
    this.palmLeaves = leavesGroup;
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
    
    const time = this.clock.getElapsedTime();
    
    // Animate water
    this.water.material.uniforms['time'].value += 1.0 / 60.0;
    
    // Animate ocean particles
    if (this.oceanParticles) {
      const positions = this.oceanParticles.geometry.attributes.position.array;
      const velocities = this.oceanParticles.geometry.attributes.velocity.array;
      
      for (let i = 0; i < positions.length; i += 3) {
        positions[i + 1] = -0.5 + Math.sin(time * velocities[i/3] + positions[i]) * 0.3;
      }
      this.oceanParticles.geometry.attributes.position.needsUpdate = true;
    }
    
    // Animate palm leaves with more natural movement
    if (this.palmLeaves) {
      this.palmLeaves.rotation.y = Math.sin(time * 0.5) * 0.1;
      this.palmLeaves.children.forEach((leaf, i) => {
        leaf.rotation.z += Math.sin(time + i) * 0.001;
        leaf.rotation.x += Math.cos(time + i) * 0.0005;
      });
    }

    this.updateMovement();
    this.renderer.render(this.scene, this.camera);
  }
}

new IslandFPS();
