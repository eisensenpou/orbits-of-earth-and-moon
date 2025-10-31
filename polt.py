import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv("build/orbit_three_body.csv")

# Compute Moon position relative to Earth
df["x_rel"] = df["x_moon"] - df["x_earth"]
df["y_rel"] = df["y_moon"] - df["y_earth"]

plt.plot(df["x_rel"], df["y_rel"], label="Moon orbit around Earth")
plt.gca().set_aspect('equal')
plt.xlabel("x (m)")
plt.ylabel("y (m)")
plt.title("Moon orbit around Earth (zoomed view)")
plt.legend()
plt.show()
