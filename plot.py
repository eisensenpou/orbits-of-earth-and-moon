"""
File: plot.py
Purpose: Animate the Sun–Earth–Moon system with a live zoomed inset around Earth.
"""
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from mpl_toolkits.axes_grid1.inset_locator import inset_axes
import shutil
from pathlib import Path
import numpy as np

# --- Load simulation data ---
df = pd.read_csv("build/orbit_three_body.csv")
steps = len(df)

# --- Create figure and axes ---
fig, ax = plt.subplots(figsize=(7, 7))
ax.set_aspect("equal")
ax.set_title("Earth & Moon Orbiting the Sun")
ax.set_xlabel("x (m)")
ax.set_ylabel("y (m)")
ax.grid(True, linestyle="--", alpha=0.4)

# Static orbital trails (faint background)
ax.plot(df["x_earth"], df["y_earth"], color="blue", alpha=0.3, lw=1)
ax.plot(df["x_moon"], df["y_moon"], color="gray", alpha=0.3, lw=0.8)
ax.scatter(df["x_sun"].iloc[0], df["y_sun"].iloc[0], color="gold", marker="*", s=120, label="Sun")
ax.legend()

# --- Inset axes (for zoomed view) ---
axins = inset_axes(ax, width="40%", height="40%", loc="lower left", borderpad=2)
axins.set_aspect("equal")
axins.set_title("Zoom on Moon orbit", fontsize=8)
zoom = 6e8

# --- Initialize moving points ---
(earth_dot,) = ax.plot([], [], "bo", markersize=6)
(moon_dot,) = ax.plot([], [], "o", color="gray", markersize=4)

(earth_dot_zoom,) = axins.plot([], [], "bo", markersize=5)
(moon_dot_zoom,) = axins.plot([], [], "o", color="gray", markersize=3)

# Preplot in inset (so it doesn't flicker)
axins.plot(df["x_earth"], df["y_earth"], color="blue", alpha=0.3)
axins.plot(df["x_moon"], df["y_moon"], color="orange", alpha=0.3)
axins.set_xlim(df["x_earth"].mean() - zoom, df["x_earth"].mean() + zoom)
axins.set_ylim(df["y_earth"].mean() - zoom, df["y_earth"].mean() + zoom)
axins.tick_params(labelsize=6)

# --- Update function ---
def update(frame):
    xe, ye = df.loc[frame, ["x_earth", "y_earth"]]
    xm, ym = df.loc[frame, ["x_moon", "y_moon"]]

    # Main view
    earth_dot.set_data([xe], [ye])
    moon_dot.set_data([xm], [ym])

    # Inset (smooth movement, no clearing)
    earth_dot_zoom.set_data([xe], [ye])
    moon_dot_zoom.set_data([xm], [ym])
    axins.set_xlim(xe - zoom, xe + zoom)
    axins.set_ylim(ye - zoom, ye + zoom)
    return earth_dot, moon_dot, earth_dot_zoom, moon_dot_zoom

# --- Run animation ---
ani = FuncAnimation(fig, update, frames=np.arange(0, steps, 20),
                    interval=20, blit=False, repeat=True)

# --- Save animation to results folder ---
results_dir = Path(__file__).resolve().parent / "results"
results_dir.mkdir(exist_ok=True)

mp4_path = results_dir / "earth_moon_zoom.mp4"
gif_path = results_dir / "earth_moon_zoom.gif"

try:
    if shutil.which("ffmpeg"):
        ani.save(mp4_path, writer="ffmpeg", fps=30)
        print(f"✅ Animation saved as {mp4_path}")
    elif shutil.which("magick") or shutil.which("convert"):
        ani.save(gif_path, writer="imagemagick", fps=30)
        print(f"✅ Animation saved as {gif_path}")
    else:
        print("⚠️ ffmpeg or ImageMagick not found — showing animation only.")
except Exception as e:
    print(f"⚠️ Could not save animation: {e}")

plt.show()
