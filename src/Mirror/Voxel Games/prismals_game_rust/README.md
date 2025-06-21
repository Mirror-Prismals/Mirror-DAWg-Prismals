# Prismals Game (Rust + Vulkan)

This is an experimental rewrite of `prismals_game.cpp` in Rust using the
[`vulkano`](https://github.com/vulkano-rs/vulkano) and [`winit`](https://github.com/rust-windowing/winit) libraries.
It currently only opens a window and sets up a Vulkan context. Rendering,
chunk generation and all gameplay logic still need to be ported from the
original C++ version.

To build, make sure you have Rust installed and Vulkan drivers available.
Then run:

```bash
cargo run
```

from this directory. Expect missing features and TODO comments; this is a
minimal starting point.
