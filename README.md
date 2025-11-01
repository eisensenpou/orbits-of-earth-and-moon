# 🌍 Earth and Moon Orbits Simulation

This project simulates the **three-body system** of the Sun, Earth, and Moon — computing their orbits using Newtonian gravity and visualizing the results in both static and animated plots.

---

## 🚀 Overview

- Written in **C++** for physics simulation.
- Exports results as `orbit_three_body.csv`.
- Includes **Python visualizations** for:
  - Static orbits with zoomed inset.
  - Animated Sun–Earth–Moon motion.
  - Scaled Moon orbits for clearer visualization.

---

## 🧩 Project Structure

```
earth-and-moon-orbits/
├── include/
│   ├── body.h
│   ├── simulation.h
│   ├── utils.h
├── src/
│   ├── main.cpp
│   ├── simulation.cpp
│   └── utils.cpp
├── polt.py
├── animate_two_dots.py
├── animate_scaled_moon.py
├── CMakeLists.txt
└── orbit_three_body.csv
```

---

## ⚙️ Building the Simulation

### Requirements
- **CMake ≥ 3.14**
- **C++17-compatible compiler** (GCC, Clang, or MSVC)

### Build and Run
```bash
mkdir build && cd build
cmake ..
make
./bin/earth_and_moon_orbits
```

This generates `orbit_three_body.csv` — the orbital data used by the Python visualizations.

---

## 📊 Visualization (Python)

### Requirements
```bash
pip install matplotlib pandas
```

### Static Plot
```bash
python polt.py
```

### Two-dot Animation (real scale)
```bash
python animate_two_dots.py
```

### Scaled Animation (visible Moon orbit)
```bash
python animate_scaled_moon.py
```

---

## 🎥 Saving Animations Automatically

At the end of any animation script, add:

```python
import shutil

# --- Save animation automatically ---
if shutil.which("ffmpeg"):
    ani.save("earth_moon_orbits.mp4", writer="ffmpeg", fps=30)
    print("✅ Saved animation as earth_moon_orbits.mp4")
elif shutil.which("magick"):
    ani.save("earth_moon_orbits.gif", writer="imagemagick", fps=30)
    print("✅ Saved animation as earth_moon_orbits.gif")
else:
    print("⚠️ ffmpeg or ImageMagick not found — showing only in window.")

plt.show()
```

> 🧰 Install dependencies:
> - **Ubuntu / Fedora:** `sudo apt install ffmpeg` or `sudo dnf install ffmpeg`
> - **macOS:** `brew install ffmpeg`
> - **Windows:** download FFmpeg from [ffmpeg.org](https://ffmpeg.org)

---

## 🌌 Example Results

| View | Description |
|------|--------------|
| 🌞 **Sun-centered view** | Earth and Moon orbiting the Sun |
| 🌙 **Zoomed Earth-centered view** | Moon’s orbit around Earth |
| 🎬 **Animated** | Smooth motion of Earth–Moon system around the Sun |

---

### 🪐 Animated Earth–Moon System
![Earth and Moon orbiting the Sun](results/earth_moon_orbits.gif)

The animation shows the Earth and Moon orbiting the Sun.  
The Moon’s orbit is visually scaled for clarity, revealing the “wobble” as both bodies travel through space.

---

## 🧠 Physics Model

- Newtonian gravitational interactions (no relativity)
- Time step: **1 hour (Δt = 3600 s)**
- Method: **Euler integration**
- Bodies:
  | Body | Mass (kg) | Notes |
  |------|------------|-------|
  | Sun | 1.9891×10³⁰ | fixed at origin |
  | Earth | 5.972×10²⁴ | initial velocity ~30 km/s |
  | Moon | 7.3477×10²² | offset 384,400 km; velocity 1 km/s relative to Earth |

Simulation duration: **1 year (8766 steps)**.

---

## 🧑‍💻 Author

**Sinan Demir**  
A hobbyist exploring orbital mechanics, physics simulations, and visualization.

> “Somewhere, something incredible is waiting to be known.” — *Carl Sagan*

---

## 🛰️ License

**MIT License** — free to use, modify, and share.
