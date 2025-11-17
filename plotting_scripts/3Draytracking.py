"""
3Draytracing (Earth–Moon Zoom)
Author: Sinan Demir
Date: 11/16/2025
"""

import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from mpl_toolkits.mplot3d import Axes3D
import numpy as np
from pathlib import Path
import shutil

df = pd.read_csv("build/orbit_three_body.csv")
steps = len(df)

R_EARTH = 6.371e6

# ---- Earth-Moon zoom scale ----
XR = 8e8  # x/y half-range
ZR = 5e8  # z half-range


def circle_on_plane(center, axis, radius, n=120):
    axis = np.asarray(axis, float)
    norm = np.linalg.norm(axis)
    if norm == 0 or radius <= 0:
        return np.array([]), np.array([]), np.array([])

    axis /= norm

    # pick arbitrary nonparallel vector
    if abs(axis[0]) < 0.9:
        ref = np.array([1, 0, 0])
    else:
        ref = np.array([0, 1, 0])

    v1 = np.cross(axis, ref)
    v1 /= np.linalg.norm(v1)
    v2 = np.cross(axis, v1)

    theta = np.linspace(0, 2*np.pi, n)
    pts = (
        center.reshape(3, 1)
        + radius * (v1.reshape(3, 1) * np.cos(theta)
                    + v2.reshape(3, 1) * np.sin(theta))
    )
    return pts[0], pts[1], pts[2]


# ---- Figure ----
fig = plt.figure(figsize=(8, 8))
ax = fig.add_subplot(111, projection="3d")

ax.set_title("Earth–Moon System (Zoomed) with Solar Ray Tracing")
ax.set_xlabel("x (m)")
ax.set_ylabel("y (m)")
ax.set_zlabel("z (m)")

# Earth marker stays at origin in VIEW coordinates
earth_dot, = ax.plot([], [], [], "bo", markersize=7, label="Earth")
moon_dot,  = ax.plot([], [], [], "ko", markersize=4, label="Moon")
shadow_dot, = ax.plot([], [], [], "ro", markersize=5, label="Shadow center")

ray_sm_line, = ax.plot([], [], [], color="orange", lw=1, alpha=0.7)
shadow_axis_line, = ax.plot([], [], [], color="black", lw=1, alpha=0.7)
umbra_line, = ax.plot([], [], [], color="black", lw=1.0)
penumbra_line, = ax.plot([], [], [], color="red", lw=0.9)

eclipse_text = ax.text2D(0.05, 0.95, "", transform=ax.transAxes)


def eclipse_type_to_str(et):
    return {
        1: "Total eclipse (umbra)",
        2: "Annular eclipse (antumbra)",
        3: "Partial eclipse (penumbra)",
    }.get(et, "No eclipse")


# ----------------- UPDATE -----------------
def update(i):
    # absolute positions
    xe, ye, ze = df.at[i, "x_earth"], df.at[i, "y_earth"], df.at[i, "z_earth"]
    xm, ym, zm = df.at[i, "x_moon"], df.at[i, "y_moon"], df.at[i, "z_moon"]
    xs, ys, zs = df.at[i, "x_sun"], df.at[i, "y_sun"], df.at[i, "z_sun"]

    sx, sy, sz = df.at[i, "shadow_x"], df.at[i, "shadow_y"], df.at[i, "shadow_z"]
    umbra_r = df.at[i, "umbra_r"]
    penumbra_r = df.at[i, "penumbra_r"]
    eclipse_type = int(df.at[i, "eclipse_type"])

    # transform to Earth-centered coordinates
    E = np.array([xe, ye, ze])
    M = np.array([xm, ym, zm]) - E
    S = np.array([xs, ys, zs]) - E
    shadow_center = np.array([sx, sy, sz]) - E

    # Earth is origin
    earth_dot.set_data([0], [0])
    earth_dot.set_3d_properties([0])

    # Moon position
    moon_dot.set_data([M[0]], [M[1]])
    moon_dot.set_3d_properties([M[2]])

    # Shadow center
    shadow_dot.set_data([shadow_center[0]], [shadow_center[1]])
    shadow_dot.set_3d_properties([shadow_center[2]])

    # Sun→Moon line
    sm = np.vstack([S, M])
    ray_sm_line.set_data(sm[:, 0], sm[:, 1])
    ray_sm_line.set_3d_properties(sm[:, 2])

    # Shadow axis (Moon → Earth)
    me_vec = -M
    L = np.linalg.norm(me_vec)

    if L > 0:
        u = me_vec / L
        ts = np.linspace(0, L + 3 * R_EARTH, 60)
        axis = M.reshape(3, 1) + u.reshape(3, 1) * ts
        shadow_axis_line.set_data(axis[0], axis[1])
        shadow_axis_line.set_3d_properties(axis[2])
    else:
        shadow_axis_line.set_data([], [])
        shadow_axis_line.set_3d_properties([])

    # Umbra / Penumbra cross-sections
    if L > 0:
        if umbra_r > 0:
            ux, uy, uz = circle_on_plane(shadow_center, me_vec, umbra_r)
            umbra_line.set_data(ux, uy)
            umbra_line.set_3d_properties(uz)
        else:
            umbra_line.set_data([], [])
            umbra_line.set_3d_properties([])

        if penumbra_r > 0:
            px, py, pz = circle_on_plane(shadow_center, me_vec, penumbra_r)
            penumbra_line.set_data(px, py)
            penumbra_line.set_3d_properties(pz)
        else:
            penumbra_line.set_data([], [])
            penumbra_line.set_3d_properties([])

    # Set dynamic Earth-centered box
    ax.set_xlim(-XR, XR)
    ax.set_ylim(-XR, XR)
    ax.set_zlim(-ZR, ZR)

    eclipse_text.set_text(f"{eclipse_type_to_str(eclipse_type)}")

    return []


ani = FuncAnimation(fig, update, frames=range(0, steps, 20),
                    interval=20, blit=False, repeat=True)


plt.show()
