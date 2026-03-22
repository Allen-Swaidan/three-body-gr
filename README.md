# Three-Body Problem — General Relativity Simulation

A real-time 3D simulation of the gravitational three-body problem using **general relativity** instead of Newtonian mechanics. Built with modern **OpenGL 4.5** and **C++17** assisted with Claude AI.

Rather than using Newton's law of gravitation, this simulation models gravity through the **Einstein-Infeld-Hoffmann (EIH) equations of motion** — the first post-Newtonian (1PN) approximation derived directly from the Einstein field equations. A deformable spacetime fabric grid warps in real-time to visualize the curvature of spacetime around each massive body.

<p align="center">
  <img src="assets/demo.gif" alt="Three-body GR simulation demo" width="800">
</p>

---

## Features

- **General relativistic gravity** — 1PN post-Newtonian approximation (Einstein-Infeld-Hoffmann equations) capturing:
  - Velocity-dependent gravitational interactions
  - Perihelion precession
  - Gravitomagnetic / frame-dragging corrections
  - Non-linear gravitational self-interaction
- **Spacetime fabric grid** — a deformable wireframe mesh that warps into gravitational wells following the Schwarzschild metric deviation, updated every frame
- **Trace paths** — colored orbital trails for each body showing the chaotic evolution of the system
- **Configurable initial conditions** — set position, velocity, and mass for each of the 3 bodies via the GUI before launching the simulation
- **Free-roaming camera** — fly around the scene with WASD + mouse look to observe the dynamics from any angle
- **Time scale control** — speed up or slow down the simulation in real-time
- **RK4 integration** — 4th-order Runge-Kutta integrator with sub-stepping for numerical stability
- **Dear ImGui interface** — clean control panel for setup, playback, and display options

---

## Physics

### Einstein-Infeld-Hoffmann Equations

The simulation implements the **1PN (first post-Newtonian) EIH equations of motion**, which are derived from the Einstein field equations in the weak-field, slow-motion limit. For body *i*, the acceleration is:

```
a_i = a_i(Newtonian) + a_i(1PN correction)
```

The Newtonian term is the familiar:

```
a_i(Newton) = sum_j  G * m_j * (r_j - r_i) / |r_j - r_i|^3
```

The 1PN correction adds terms proportional to **v²/c²** and **GM/(rc²)** that encode:

- **Velocity-dependent forces** — the gravitational interaction depends on how fast the bodies are moving
- **Gravitational potential coupling** — the potential energy of nearby bodies modifies the force
- **Gravitomagnetic effects** — a frame-dragging term analogous to magnetic forces in electromagnetism

These corrections produce measurable effects like Mercury's perihelion precession, which Newtonian gravity cannot explain.

### Spacetime Curvature Visualization

The grid mesh computes the **Schwarzschild metric deviation** at each vertex by superposing the gravitational fields of all three bodies:

```
g_tt ≈ -(1 - 2GM/(rc²))
```

The vertical displacement of each grid point is proportional to the gravitational potential, creating the classic "rubber sheet" visualization of curved spacetime — but computed from the actual metric tensor rather than an arbitrary function.

---

## Building

### Prerequisites

- **CMake** 3.20+
- **C++17** compatible compiler (GCC 9+, Clang 10+, MSVC 2019+)
- **OpenGL 4.5** capable GPU and drivers
- **X11 development libraries** (Linux only):

```bash
sudo apt-get install libxinerama-dev libxcursor-dev libxi-dev libxrandr-dev
```

All other dependencies (**GLFW**, **GLM**, **GLAD**, **Dear ImGui**) are automatically downloaded via CMake FetchContent — no manual installation needed.

### Build Steps

```bash
git clone https://github.com/Allen-Swaidan/three-body-gr.git
cd three-body-gr
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Run

```bash
./three_body_gr
```

The executable must be run from the `build/` directory (or wherever the `shaders/` folder was copied to by CMake).

---

## Controls

| Input | Action |
|---|---|
| **WASD** | Move camera horizontally |
| **Space** | Move camera up |
| **Left Shift** | Move camera down |
| **ESC** | Toggle mouse look (capture/release cursor) |
| **Scroll Wheel** | Zoom in/out (adjust FOV) |
| **ImGui Panel** | Configure bodies, start/pause/reset simulation |

### Workflow

1. **Setup mode** — adjust position, velocity, and mass for each body using the drag sliders in the control panel
2. Click **Start Simulation** to begin
3. Press **ESC** to capture the mouse and fly around the scene
4. Press **ESC** again to release the mouse and interact with the UI
5. Use the **Time Scale** slider to speed up or slow down
6. Click **Reset** to return to setup mode

---

## Recording a Demo GIF

The application includes a built-in recording mode with an autopilot camera that orbits the scene:

```bash
# Record 15 seconds of demo footage (saves PPM frames to frames/)
./three_body_gr --record

# Optionally set duration
./three_body_gr --record --duration 20

# Convert frames to GIF with ffmpeg
ffmpeg -framerate 30 -i frames/frame_%05d.ppm -vf "fps=15,scale=800:-1:flags=lanczos,split[s0][s1];[s0]palettegen=max_colors=128[p];[s1][p]paletteuse=dither=bayer" assets/demo.gif
```

The autopilot camera orbits, rises, and dips around the simulation to showcase the bodies, traces, spacetime grid warping, and smooth camera movement.

---

## Project Structure

```
three-body-gr/
├── CMakeLists.txt              # Build system with FetchContent dependencies
├── include/
│   ├── body.h                  # Body struct, sphere mesh, trace renderer
│   ├── camera.h                # Free-roaming FPS camera
│   ├── shader.h                # GLSL shader loader/compiler
│   ├── simulation.h            # GR simulation (EIH equations + RK4)
│   └── spacetime_grid.h        # Deformable spacetime fabric mesh
├── shaders/
│   ├── body.vert / body.frag   # Lit sphere rendering with rim glow
│   ├── grid.vert / grid.frag   # Curvature-colored wireframe grid
│   └── trace.vert / trace.frag # Colored orbital trace lines
└── src/
    ├── main.cpp                # Application entry, OpenGL setup, ImGui UI
    ├── body.cpp                # Sphere mesh generation, trace buffer management
    ├── camera.cpp              # Camera movement and mouse look
    ├── shader.cpp              # Shader compilation and uniform setters
    ├── simulation.cpp          # EIH equations, RK4 integrator, curvature calc
    └── spacetime_grid.cpp      # Grid vertex warping from metric deviation
```

---

## Dependencies

| Library | Version | Purpose |
|---|---|---|
| [GLFW](https://github.com/glfw/glfw) | 3.3.8 | Windowing and input |
| [GLM](https://github.com/g-truc/glm) | 0.9.9.8 | Mathematics (vectors, matrices) |
| [GLAD](https://github.com/Dav1dde/glad) | 0.1.36 | OpenGL 4.5 function loading |
| [Dear ImGui](https://github.com/ocornut/imgui) | 1.89.9 | Immediate-mode GUI |

---

## License

This project is released under the [MIT License](LICENSE).
