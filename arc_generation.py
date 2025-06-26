import numpy as np
from PIL import Image
import math

# Display dimensions and ring configuration (matching your ESP32 code)
WIDTH, HEIGHT = 240, 240
CENTER = (120, 120)
R_INNER = 89
R_OUTER = 101
START_ANGLE = -135
SWEEP_ANGLE = 270  # full arc

def generate_full_arc_image():
    img = Image.new("RGB", (WIDTH, HEIGHT), (0, 0, 0))

    # Draw the arc from -135 to +135 degrees (270° total) in 0.1° steps
    for angle in np.arange(START_ANGLE, START_ANGLE + SWEEP_ANGLE, 0.1):
        rad = math.radians(angle)
        for r in range(R_INNER, R_OUTER + 1):
            x = int(round(CENTER[0] + r * math.cos(rad)))
            y = int(round(CENTER[1] + r * math.sin(rad)))
            if 0 <= x < WIDTH and 0 <= y < HEIGHT:
                img.putpixel((x, y), (255, 255, 255))  # white arc

    return img

# Generate and save the full arc image
arc_img = generate_full_arc_image()
arc_img.save("full_arc_ring.png")
print("Saved: full_arc_ring.png")
