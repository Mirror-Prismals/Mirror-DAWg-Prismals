//! Minimal Rust/Vulkan skeleton for Prismals.
//!
//! This is a starting point for porting `prismals_game.cpp` into Rust. It
//! creates a window and initializes Vulkan using the `vulkano` and `winit`
//! crates. Full rendering code is left as a TODO so that the original C++ file
//! remains intact.

use vulkano::instance::{Instance, InstanceCreateInfo};
use vulkano::device::{Device, DeviceCreateInfo, QueueCreateInfo};
use vulkano::swapchain::{Surface, Swapchain, SwapchainCreateInfo};
use vulkano::image::ImageUsage;
use vulkano::sync::GpuFuture;
use winit::{event::{Event, WindowEvent, KeyboardInput, VirtualKeyCode, ElementState},
    event_loop::{ControlFlow, EventLoop}, window::WindowBuilder};
use nalgebra::Vector3;
use std::time::Instant;
use std::sync::Arc;

const GRAVITY: f32 = 9.8;
const SLOPE_ANGLE: f32 = 30.0_f32.to_radians();

struct InputState {
    forward: bool,
}

impl Default for InputState {
    fn default() -> Self {
        Self { forward: false }
    }
}

struct Player {
    position: Vector3<f32>,
    velocity: Vector3<f32>,
}

impl Player {
    fn new() -> Self {
        Self {
            position: Vector3::new(0.0, 10.0, 0.0),
            velocity: Vector3::zeros(),
        }
    }

    fn update(&mut self, dt: f32, input: &InputState) {
        let slope_height = 10.0 - self.position.x * SLOPE_ANGLE.tan();
        if self.position.y > slope_height {
            self.velocity.y -= GRAVITY * dt;
        } else if self.velocity.y < 0.0 {
            self.position.y = slope_height;
            self.velocity.y = 0.0;
        }

        if self.position.y <= slope_height {
            // Sliding acceleration along the slope
            self.velocity.x += GRAVITY * SLOPE_ANGLE.sin() * dt;
            if input.forward {
                self.velocity.x += 5.0 * dt;
            }
        }

        self.position += self.velocity * dt;
    }
}

fn main() {
    // Create winit event loop and window
    let event_loop = EventLoop::new();
    let window = WindowBuilder::new()
        .with_title("Prismals Vulkan")
        .build(&event_loop)
        .expect("failed to create window");

    // Create Vulkan instance
    let instance = Instance::new(InstanceCreateInfo::default())
        .expect("failed to create Vulkan instance");

    // Create surface from winit window
    let surface = unsafe { Surface::from_window(Arc::clone(&instance), window) }
        .expect("failed to create surface");

    // Pick first physical device with graphics queue
    let physical = vulkano::instance::physical::PhysicalDevice::enumerate(&instance)
        .next().expect("no device available");
    let queue_family = physical.queue_families()
        .find(|q| q.supports_graphics())
        .expect("no graphics queue found");

    // Create logical device and graphics queue
    let (device, mut queues) = Device::new(
        physical,
        DeviceCreateInfo {
            queue_create_infos: vec![QueueCreateInfo::family(queue_family)],
            ..Default::default()
        }
    ).expect("failed to create device");
    let queue = queues.next().unwrap();

    // Create swapchain
    let (_swapchain, _images) = Swapchain::new(
        device.clone(), surface,
        SwapchainCreateInfo {
            image_usage: ImageUsage::color_attachment(),
            ..Default::default()
        }
    ).expect("failed to create swapchain");

    let mut player = Player::new();
    let mut input = InputState::default();
    let mut last_frame = Instant::now();

    event_loop.run(move |event, _, control_flow| {
        *control_flow = ControlFlow::Poll;
        match event {
            Event::WindowEvent { event, .. } => match event {
                WindowEvent::CloseRequested => *control_flow = ControlFlow::Exit,
                WindowEvent::KeyboardInput { input: KeyboardInput { state, virtual_keycode: Some(key), .. }, .. } => {
                    match (key, state) {
                        (VirtualKeyCode::W, ElementState::Pressed) => input.forward = true,
                        (VirtualKeyCode::W, ElementState::Released) => input.forward = false,
                        _ => {}
                    }
                }
                _ => {}
            },
            Event::MainEventsCleared => {
                let now = Instant::now();
                let dt = now.duration_since(last_frame).as_secs_f32();
                last_frame = now;

                player.update(dt, &input);

                // Rendering code would go here. For now we just idle.
            }
            _ => {}
        }
    });
}
