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
use winit::{event::{Event, WindowEvent}, event_loop::{ControlFlow, EventLoop}, window::WindowBuilder};
use std::sync::Arc;

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

    event_loop.run(move |event, _, control_flow| {
        *control_flow = ControlFlow::Poll;
        match event {
            Event::WindowEvent { event: WindowEvent::CloseRequested, .. } => {
                *control_flow = ControlFlow::Exit;
            }
            Event::MainEventsCleared => {
                // Rendering code would go here. For now we just idle.
            }
            _ => ()
        }
    });
}
