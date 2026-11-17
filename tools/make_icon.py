"""生成 AccelBall 程序图标（深色圆底 + 绿色内存环）。"""

import math
import os

from PIL import Image, ImageDraw

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SIZE = 1024
CENTER = SIZE / 2

image = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
draw = ImageDraw.Draw(image)

margin = 60
draw.ellipse([margin, margin, SIZE - margin, SIZE - margin], fill=(26, 32, 44, 255))

# 顶部高光（Pillow 的 ImageDraw 不做 alpha 混合，这里用不透明的略亮色）
draw.ellipse(
    [SIZE * 0.24, SIZE * 0.12, SIZE * 0.70, SIZE * 0.40],
    fill=(40, 46, 58, 255),
)

ring_pad = 140
ring_width = 76
ring_box = [ring_pad, ring_pad, SIZE - ring_pad, SIZE - ring_pad]
draw.arc(ring_box, 0, 360, fill=(58, 68, 84, 255), width=ring_width)

start_angle = -90
sweep = 270
draw.arc(ring_box, start_angle, start_angle + sweep, fill=(34, 197, 94, 255), width=ring_width)

radius = (SIZE - 2 * ring_pad) / 2
for angle in (start_angle, start_angle + sweep):
    radians = math.radians(angle)
    x = CENTER + radius * math.cos(radians)
    y = CENTER + radius * math.sin(radians)
    draw.ellipse(
        [x - ring_width / 2, y - ring_width / 2, x + ring_width / 2, y + ring_width / 2],
        fill=(34, 197, 94, 255),
    )

icon = image.resize((256, 256), Image.LANCZOS)
output = os.path.join(ROOT, "src", "app.ico")
icon.save(output, format="ICO", sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])
print(f"wrote {output}")
