# animate_scaled_moon.py
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import numpy as np

df = pd.read_csv("build/orbit_three_body.csv")

# --- exaggerate Moon's distance from Earth for visualization ---
SCALE = 50  # exaggeration factor — try 30–100
x_rel = df["x_moon"] - df["x_earth"]
y_rel = df["y_moon"] - df["y_earth"]
df["x_moon_vis"] = df["x_earth"] + SCALE * x_rel
df["y_moon_vis"] = df["y_earth"] + SCALE * y_rel

# --- setup ---
fig, ax = plt.subplots(figsize=(7, 7))
ax.set_aspect("equal")
ax.set_title("Exaggerated Earth & Moon orbiting the Sun")
ax.set_xlabel("x (m)")
ax.set_ylabel("y (m)")
ax.grid(True, linestyle="--", alpha=0.3)

# Sun
ax.scatter(df["x_sun"].iloc[0], df["y_sun"].iloc[0], marker="*", color="gold", s=180, label="Sun")
ax.legend(loc="upper right")

# Axis limits with margin
x_all = pd.concat([df["x_earth"], df["x_moon_vis"]])
y_all = pd.concat([df["y_earth"], df["y_moon_vis"]])
pad_x = 0.05 * (x_all.max() - x_all.min())
pad_y = 0.05 * (y_all.max() - y_all.min())
ax.set_xlim(x_all.min() - pad_x, x_all.max() + pad_x)
ax.set_ylim(y_all.min() - pad_y, y_all.max() + pad_y)

# Moving dots
(earth_dot,) = ax.plot([], [], "o", color="tab:blue", markersize=6)
(moon_dot,) = ax.plot([], [], "o", color="tab:gray", markersize=4)

def init():
    earth_dot.set_data([], [])
    moon_dot.set_data([], [])
    return earth_dot, moon_dot

def update(i):
    xe, ye = df.loc[i, ["x_earth", "y_earth"]]
    xm, ym = df.loc[i, ["x_moon_vis", "y_moon_vis"]]
    earth_dot.set_data([xe], [ye])
    moon_dot.set_data([xm], [ym])
    return earth_dot, moon_dot

frames = np.arange(0, len(df), 10)
ani = FuncAnimation(fig, update, frames=frames, init_func=init, interval=20, blit=True, repeat=True)

plt.show()
