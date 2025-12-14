# 🎯 Missile Attack Simulation: DIU vs City University

An interactive OpenGL-based computer graphics project that simulates an animated missile attack scenario between **Daffodil International University (DIU)** and **City University**. This project demonstrates various computer graphics algorithms and animation techniques.

![Project Preview](Images/Screenshot%202025-12-15%20000554.png)

## 📋 Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Technologies Used](#technologies-used)
- [Algorithms Implemented](#algorithms-implemented)
- [Project Structure](#project-structure)
- [Installation & Setup](#installation--setup)
- [How to Run](#how-to-run)
- [Controls](#controls)
- [Screenshots](#screenshots)
- [Project Details](#project-details)

## 🎬 Overview

This project creates an immersive 3D-like 2D scene featuring:
- Two university buildings (DIU and City University) connected by a road
- Animated missile attacks with realistic physics
- Building destruction effects with explosions, fire, and debris
- Celebration animations with firecrackers and people
- Dynamic environment with moving clouds, vehicles, and flags
- Night sky with moon, stars, and atmospheric effects

## ✨ Features

### 🏢 Scene Elements
- **DIU Building**: Detailed architecture with windows, doors, and signage
- **City University Building**: Multi-block structure with windows and entrance
- **Road**: Connecting path with street lights and road markings
- **Environment**: Trees, bushes, mountains, and ground textures

### 🚀 Animations
- **Rocket/Missile Attack**: Parabolic trajectory from DIU to City University
- **Building Destruction**: Explosions, fire, smoke, and debris particles
- **Firecracker Celebration**: Colorful fireworks with multiple trajectories
- **People Animation**: 30 people walking between universities and celebrating
- **Drone**: Continuous back-and-forth movement between buildings
- **Clouds**: Looping cloud movement across the sky
- **Vehicles**: Cars moving on the road
- **Flags**: Waving flags on both buildings

### 🎨 Visual Effects
- Night sky with moon and stars
- Explosion effects with expanding circles and fade
- Fire effects with flickering animation
- Smoke particles rising from destruction
- Debris physics simulation
- Colorful firecracker explosions

### 🔊 Audio
- Rocket launch sound effect
- Explosion sound effect
- Firecracker celebration sound

## 🛠️ Technologies Used

- **OpenGL** (FreeGLUT) - Graphics rendering
- **C++** - Programming language
- **Windows Multimedia API** - Sound effects
- **Standard C++ Libraries** - Math, file I/O, time management

## 🧮 Algorithms Implemented

### 1. **Parabolic Trajectory Algorithm**
- Rocket follows a parabolic path with peak height
- Formula: `y = linearY + arcHeight` where `arcHeight = 4.0f * t * (1.0f - t)`
- Dynamic angle calculation using `atan2`

### 2. **Linear Interpolation (Lerp)**
- Smooth position interpolation for animations
- Used in rocket, drone, vehicle, and cloud movements

### 3. **Particle System**
- Debris particles with position, velocity, and life
- Firecracker particles with trajectory types
- Smoke effects with multiple puffs

### 4. **Physics Simulation**
- Gravity: `vy -= gravity * timeScale`
- Velocity-based movement: `x += vx * timeScale`
- Distance calculation: `sqrt(dx² + dy²)`

### 5. **Sine Wave Animation**
- Flag waving: `sin(flagWaveTime + x * 0.5f) * 0.3f`
- Walking animation: `sin(walkCycle * 2π) * 0.1f`
- Celebration: `sin(celebrationCycle * 2π) * 0.3f`
- Fire flicker: `sin(flicker * 10.0f) * 0.1f`

### 6. **Circle Drawing Algorithm**
- Triangle fan method for smooth circles
- Used for moon, stars, clouds, explosions, and particles

### 7. **Frame-Rate Independent Animation**
- Delta time calculation for consistent animation speed
- Normalized updates: `timeScale = deltaTime / 0.0167f`

### 8. **State Machine**
- People states: walking to city, walking to DIU, stopped, celebrating
- Rocket states: launching, impacting, destroyed
- Firecracker states: traveling, exploded, animating

### 9. **Explosion Effect Algorithm**
- Expanding circles with fade: `maxRadius = 3.0f * time`
- Alpha fade: `fade = 1.0f - (time / 2.0f)`
- Multi-layer rendering (outer, middle, inner, core)

### 10. **Collision Detection**
- Position-based collision checks
- Rocket impact detection: `rocketAnimTime >= 1.0f`
- People destination checks

## 📁 Project Structure

```
OpenGL Programming/
├── project.cpp          # Main source code
├── Music/
│   └── Rocket-launcher.wav  # Sound effects
├── Images/             # Screenshots
│   ├── Screenshot 2025-12-15 000554.png
│   ├── Screenshot 2025-12-15 000608.png
│   ├── Screenshot 2025-12-15 000616.png
│   ├── Screenshot 2025-12-15 000700.png
│   ├── Screenshot 2025-12-15 000722.png
│   └── Screenshot 2025-12-15 000738.png
└── README.md           # This file
```

## 💻 Installation & Setup

### Prerequisites
- Windows OS (for sound effects)
- C++ Compiler (MinGW/GCC recommended)
- FreeGLUT library
- OpenGL libraries

### Setup Steps

1. **Install FreeGLUT**
   - Download FreeGLUT for Windows
   - Extract and place in your project directory or system path

2. **Install OpenGL Libraries**
   - OpenGL is usually included with graphics drivers
   - Ensure your graphics drivers are up to date

3. **Compile the Project**
   ```bash
   g++ project.cpp -o project.exe -lfreeglut -lopengl32 -lglu32 -lwinmm
   ```

## 🚀 How to Run

1. **Compile the project** using the command above
2. **Ensure sound files** are in the `Music/` directory:
   - `Rocket-launcher.wav`
   - `explosion.wav`
   - `firecrackers.wav`
3. **Run the executable**:
   ```bash
   ./project.exe
   ```
4. **Watch the animation**:
   - Countdown timer (10 seconds)
   - First missile launch
   - Building impact
   - Second missile launch
   - Building destruction
   - Celebration with firecrackers

## 🎮 Controls

- **ESC** or **Q** - Exit the program
- **R** - Restart the animation (resets all states)

## 📸 Screenshots

### Initial Scene - Night Sky View
![Initial Scene](Images/Screenshot%202025-12-15%20000554.png)

### Countdown Timer
![Countdown](Images/Screenshot%202025-12-15%20000608.png)

### Rocket Launch
![Rocket Launch](Images/Screenshot%202025-12-15%20000616.png)

### Building Destruction
![Destruction](Images/Screenshot%202025-12-15%20000700.png)

### Celebration Scene
![Celebration](Images/Screenshot%202025-12-15%20000722.png)

### Victory Message
![Victory](Images/Screenshot%202025-12-15%20000738.png)

## 📊 Project Details

### Animation Timeline

1. **0-10 seconds**: Countdown timer displays
2. **10 seconds**: First missile launches from DIU
3. **~12 seconds**: First missile hits City University (no destruction)
4. **~16 seconds**: Second missile launches
5. **~18 seconds**: Second missile hits, building destroyed
6. **~22 seconds**: Firecracker celebration begins
7. **Ongoing**: People walk, celebrate, firecrackers continue

### Technical Specifications

- **Window Size**: 1000x700 pixels
- **World Coordinates**: X: [-40, 40], Y: [0, 20]
- **Frame Rate**: 60 FPS (frame-rate independent)
- **People Count**: 30 animated characters
- **Firecrackers**: 5-7 per batch (random)
- **Debris Particles**: 30 particles per explosion

### Color Scheme

- **Night Sky**: Dark blue (`0.02, 0.05, 0.15`)
- **Moon**: Off-white (`0.98, 0.98, 0.95`)
- **Stars**: White with varying brightness
- **Buildings**: Light colors (white, beige)
- **Explosions**: Yellow, orange, red
- **Firecrackers**: Red, yellow, blue, green

## 🎓 Educational Value

This project demonstrates:
- **Computer Graphics Fundamentals**: Rendering, transformations, color theory
- **Animation Techniques**: Keyframe animation, interpolation, physics simulation
- **Game Development Concepts**: State machines, particle systems, collision detection
- **Mathematical Applications**: Trigonometry, linear algebra, physics equations
- **Software Engineering**: Code organization, modular design, resource management

## 📝 Notes

- The project uses frame-rate independent animation for consistent performance
- Sound effects require Windows OS (uses Windows Multimedia API)
- All animations are synchronized and timed precisely
- The scene dynamically adjusts to window resizing

## 👨‍💻 Author

**Soad As Hamim Mahi**

## 📄 License

This project is created for educational purposes as part of a Computer Graphics course.

---

**Enjoy the animation! 🎉**

