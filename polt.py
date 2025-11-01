import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from mpl_toolkits.axes_grid1.inset_locator import inset_axes

# --- Load data ---
df = pd.read_csv("build/orbit_three_body.csv")
steps = len(df)

fig, ax = plt.subplots(figsize=(7,7))
ax.set_aspect("equal")
ax.set_title("Earth & Moon Orbiting the Sun")
ax.set_xlabel("x (m)")
ax.set_ylabel("y (m)")
ax.grid(True, linestyle="--", alpha=0.4)

# --- Static Sun marker ---
ax.scatter(df["x_sun"].iloc[0], df["y_sun"].iloc[0], color="gold", marker="*", s=120, label="Sun")
ax.legend()

# --- Inset setup ---
axins = inset_axes(ax, width="40%", height="40%", loc="lower left", borderpad=2)
axins.set_aspect("equal")
axins.set_title("Zoom on Moon orbit", fontsize=8)
zoom = 6e8

# --- Dynamic artists ---
earth_line, = ax.plot([], [], color="blue", lw=1.5, label="Earth orbit")
moon_line, = ax.plot([], [], color="orange", lw=0.8, label="Moon orbit")
earth_dot, = ax.plot([], [], 'bo', markersize=6)
moon_dot, = ax.plot([], [], 'o', color="gray", markersize=4)

# In inset
earth_line_z, = axins.plot([], [], color="blue", lw=1)
moon_line_z, = axins.plot([], [], color="orange", lw=1)
earth_dot_z, = axins.plot([], [], 'bo', markersize=4)
moon_dot_z, = axins.plot([], [], 'o', color="gray", markersize=3)

# --- Update function ---
def update(frame):
    xe, ye = df["x_earth"][:frame], df["y_earth"][:frame]
    xm, ym = df["x_moon"][:frame], df["y_moon"][:frame]

    # Main plot trails
    earth_line.set_data(xe, ye)
    moon_line.set_data(xm, ym)
    earth_dot.set_data([xe.iloc[-1]], [ye.iloc[-1]])
    moon_dot.set_data([xm.iloc[-1]], [ym.iloc[-1]])

    # Inset trails
    axins.set_xlim(xe.iloc[-1] - zoom, xe.iloc[-1] + zoom)
    axins.set_ylim(ye.iloc[-1] - zoom, ye.iloc[-1] + zoom)
    earth_line_z.set_data(xe, ye)
    moon_line_z.set_data(xm, ym)
    earth_dot_z.set_data([xe.iloc[-1]], [ye.iloc[-1]])
    moon_dot_z.set_data([xm.iloc[-1]], [ym.iloc[-1]])

    axins.set_title("Zoom on Moon orbit", fontsize=8)
    axins.tick_params(labelsize=6)
    return (earth_line, moon_line, earth_dot, moon_dot,
            earth_line_z, moon_line_z, earth_dot_z, moon_dot_z)

ani = FuncAnimation(fig, update, frames=range(1, steps, 20), interval=20, blit=False, repeat=True)

plt.show()
