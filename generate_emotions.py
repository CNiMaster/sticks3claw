#!/usr/bin/env python3
"""Generate simple RGB565 emotion frames for Sticks3 135x240 TFT display."""

import struct
import os
import math

W, H = 135, 240

def rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

def save_rgb565(pixels, path):
    with open(path, 'wb') as f:
        for r, g, b in pixels:
            f.write(struct.pack('<H', rgb565(r, g, b)))

def bg_color():
    return (20, 20, 30)

def draw_circle(pixels, cx, cy, radius, color, fill=True):
    for y in range(H):
        for x in range(W):
            dx, dy = x - cx, y - cy
            if fill:
                if dx*dx + dy*dy <= radius*radius:
                    pixels[y * W + x] = color
            else:
                dist = math.sqrt(dx*dx + dy*dy)
                if abs(dist - radius) < 2:
                    pixels[y * W + x] = color

def draw_rect(pixels, x1, y1, x2, y2, color):
    for y in range(max(0, y1), min(H, y2)):
        for x in range(max(0, x1), min(W, x2)):
            pixels[y * W + x] = color

def draw_ellipse(pixels, cx, cy, rx, ry, color):
    for y in range(H):
        for x in range(W):
            dx, dy = x - cx, y - cy
            if rx > 0 and ry > 0:
                if (dx*dx)/(rx*rx) + (dy*dy)/(ry*ry) <= 1:
                    pixels[y * W + x] = color

def new_frame():
    bg = bg_color()
    return [bg] * (W * H)

# Face center position
FACE_CX, FACE_CY = 67, 100
FACE_R = 50
SKIN = (255, 220, 180)
EYE_WHITE = (255, 255, 255)
PUPIL = (30, 30, 30)
MOUTH_COLOR = (200, 80, 80)

def make_idle(frame_num):
    pixels = new_frame()
    # Face
    draw_circle(pixels, FACE_CX, FACE_CY, FACE_R, SKIN)
    # Eyes - two circles
    draw_circle(pixels, FACE_CX - 18, FACE_CY - 10, 8, EYE_WHITE)
    draw_circle(pixels, FACE_CX + 18, FACE_CY - 10, 8, EYE_WHITE)
    draw_circle(pixels, FACE_CX - 16, FACE_CY - 10, 4, PUPIL)
    draw_circle(pixels, FACE_CX + 20, FACE_CY - 10, 4, PUPIL)
    # Mouth - small smile
    for i in range(-12, 13):
        x = FACE_CX + i
        y = FACE_CY + 18 + int(math.sqrt(max(0, 144 - i*i)) * 0.3)
        if 0 <= x < W and 0 <= y < H:
            for dy in range(-1, 2):
                if 0 <= y+dy < H:
                    pixels[(y+dy) * W + x] = MOUTH_COLOR
    # Blink on frame 1
    if frame_num == 1:
        draw_rect(pixels, FACE_CX - 26, FACE_CY - 14, FACE_CX - 8, FACE_CY - 6, SKIN)
        draw_rect(pixels, FACE_CX + 10, FACE_CY - 14, FACE_CX + 28, FACE_CY - 6, SKIN)
        draw_rect(pixels, FACE_CX - 25, FACE_CY - 10, FACE_CX - 9, FACE_CY - 8, PUPIL)
        draw_rect(pixels, FACE_CX + 11, FACE_CY - 10, FACE_CX + 27, FACE_CY - 8, PUPIL)
    return pixels

def make_listening(frame_num):
    pixels = new_frame()
    draw_circle(pixels, FACE_CX, FACE_CY, FACE_R, SKIN)
    # Wide open eyes
    draw_circle(pixels, FACE_CX - 18, FACE_CY - 10, 10, EYE_WHITE)
    draw_circle(pixels, FACE_CX + 18, FACE_CY - 10, 10, EYE_WHITE)
    draw_circle(pixels, FACE_CX - 18 + frame_num * 2, FACE_CY - 10, 5, PUPIL)
    draw_circle(pixels, FACE_CX + 18 + frame_num * 2, FACE_CY - 10, 5, PUPIL)
    # O mouth
    draw_ellipse(pixels, FACE_CX, FACE_CY + 20, 8, 12, MOUTH_COLOR)
    draw_ellipse(pixels, FACE_CX, FACE_CY + 20, 5, 9, (60, 30, 30))
    # Ear wave
    if frame_num > 0:
        for i in range(3):
            angle = frame_num * 0.5 + i * 0.8
            ex = FACE_CX + 55 + int(math.sin(angle) * 5)
            ey = FACE_CY - 10 + i * 12
            draw_circle(pixels, ex, ey, 3, (100, 200, 255), fill=False)
    return pixels

def make_thinking(frame_num):
    pixels = new_frame()
    draw_circle(pixels, FACE_CX, FACE_CY, FACE_R, SKIN)
    # Eyes looking up-right
    draw_circle(pixels, FACE_CX - 18, FACE_CY - 12, 8, EYE_WHITE)
    draw_circle(pixels, FACE_CX + 18, FACE_CY - 12, 8, EYE_WHITE)
    draw_circle(pixels, FACE_CX - 14, FACE_CY - 16, 4, PUPIL)
    draw_circle(pixels, FACE_CX + 22, FACE_CY - 16, 4, PUPIL)
    # Wavy mouth
    for i in range(-10, 11):
        x = FACE_CX + i
        y = FACE_CY + 18 + int(math.sin(i * 0.5 + frame_num) * 3)
        if 0 <= x < W and 0 <= y < H:
            pixels[y * W + x] = MOUTH_COLOR
    return pixels

def make_speaking(frame_num):
    pixels = new_frame()
    draw_circle(pixels, FACE_CX, FACE_CY, FACE_R, SKIN)
    # Happy eyes (arcs)
    draw_circle(pixels, FACE_CX - 18, FACE_CY - 8, 8, EYE_WHITE)
    draw_circle(pixels, FACE_CX + 18, FACE_CY - 8, 8, EYE_WHITE)
    draw_circle(pixels, FACE_CX - 18, FACE_CY - 8, 4, PUPIL)
    draw_circle(pixels, FACE_CX + 18, FACE_CY - 8, 4, PUPIL)
    # Open mouth varying size
    mouth_h = 6 + abs(frame_num - 1) * 4
    draw_ellipse(pixels, FACE_CX, FACE_CY + 20, 12, mouth_h, MOUTH_COLOR)
    if mouth_h > 8:
        draw_ellipse(pixels, FACE_CX, FACE_CY + 20, 8, mouth_h - 4, (60, 30, 30))
    return pixels

def make_happy(frame_num):
    pixels = new_frame()
    draw_circle(pixels, FACE_CX, FACE_CY, FACE_R, SKIN)
    # Happy squinted eyes
    for sign in [-1, 1]:
        ex = FACE_CX + sign * 18
        for i in range(-8, 9):
            x = ex + i
            y = FACE_CY - 10 + int(math.sqrt(max(0, 64 - i*i)) * 0.4)
            if 0 <= x < W and 0 <= y < H:
                pixels[y * W + x] = PUPIL
    # Big smile
    for i in range(-18, 19):
        x = FACE_CX + i
        y = FACE_CY + 16 + int(math.sqrt(max(0, 324 - i*i)) * 0.5)
        if 0 <= x < W and 0 <= y < H:
            for dy in range(-1, 3):
                if 0 <= y+dy < H:
                    pixels[(y+dy) * W + x] = MOUTH_COLOR
    return pixels

def make_sad(frame_num):
    pixels = new_frame()
    draw_circle(pixels, FACE_CX, FACE_CY, FACE_R, SKIN)
    # Sad eyes (droopy)
    draw_circle(pixels, FACE_CX - 18, FACE_CY - 8, 8, EYE_WHITE)
    draw_circle(pixels, FACE_CX + 18, FACE_CY - 8, 8, EYE_WHITE)
    draw_circle(pixels, FACE_CX - 20, FACE_CY - 6, 4, PUPIL)
    draw_circle(pixels, FACE_CX + 16, FACE_CY - 6, 4, PUPIL)
    # Sad mouth (inverted smile)
    for i in range(-12, 13):
        x = FACE_CX + i
        y = FACE_CY + 22 - int(math.sqrt(max(0, 144 - i*i)) * 0.3)
        if 0 <= x < W and 0 <= y < H:
            pixels[y * W + x] = MOUTH_COLOR
    # Tear
    if frame_num > 0:
        draw_circle(pixels, FACE_CX - 26, FACE_CY + 2 + frame_num * 3, 2, (100, 180, 255))
    return pixels

def make_surprised(frame_num):
    pixels = new_frame()
    draw_circle(pixels, FACE_CX, FACE_CY, FACE_R, SKIN)
    # Very wide eyes
    r = 12 + frame_num
    draw_circle(pixels, FACE_CX - 18, FACE_CY - 10, min(r, 14), EYE_WHITE)
    draw_circle(pixels, FACE_CX + 18, FACE_CY - 10, min(r, 14), EYE_WHITE)
    draw_circle(pixels, FACE_CX - 18, FACE_CY - 10, 5, PUPIL)
    draw_circle(pixels, FACE_CX + 18, FACE_CY - 10, 5, PUPIL)
    # Big O mouth
    draw_ellipse(pixels, FACE_CX, FACE_CY + 20, 10, 14, MOUTH_COLOR)
    draw_ellipse(pixels, FACE_CX, FACE_CY + 20, 6, 10, (60, 30, 30))
    # Shock lines
    for angle_deg in [30, 60, 120, 150]:
        angle = math.radians(angle_deg)
        x1 = FACE_CX + int(math.cos(angle) * 55)
        y1 = FACE_CY - 20 + int(math.sin(angle) * 55)
        x2 = FACE_CX + int(math.cos(angle) * 65)
        y2 = FACE_CY - 20 + int(math.sin(angle) * 65)
        for t in range(10):
            x = int(x1 + (x2 - x1) * t / 10)
            y = int(y1 + (y2 - y1) * t / 10)
            if 0 <= x < W and 0 <= y < H:
                pixels[y * W + x] = (255, 255, 100)
    return pixels

EMOTIONS = {
    'idle': {'frames': 2, 'loop': True, 'duration': [3000, 200], 'gen': make_idle},
    'listening': {'frames': 3, 'loop': True, 'duration': [300, 300, 300], 'gen': make_listening},
    'thinking': {'frames': 3, 'loop': True, 'duration': [300, 300, 300], 'gen': make_thinking},
    'speaking': {'frames': 3, 'loop': True, 'duration': [150, 150, 150], 'gen': make_speaking},
    'happy': {'frames': 2, 'loop': True, 'duration': [500, 500], 'gen': make_happy},
    'sad': {'frames': 3, 'loop': True, 'duration': [500, 500, 500], 'gen': make_sad},
    'surprised': {'frames': 2, 'loop': False, 'duration': [300, 500], 'gen': make_surprised},
}

def main():
    base_dir = os.path.dirname(os.path.abspath(__file__))
    emotions_dir = os.path.join(base_dir, 'data', 'emotions')

    for name, config in EMOTIONS.items():
        emotion_dir = os.path.join(emotions_dir, name)
        os.makedirs(emotion_dir, exist_ok=True)

        gen = config['gen']
        frame_files = []
        for i in range(config['frames']):
            pixels = gen(i)
            filename = f"frame_{i:02d}.rgb565"
            filepath = os.path.join(emotion_dir, filename)
            save_rgb565(pixels, filepath)
            frame_files.append(filename)
            print(f"  Generated {filepath}")

        # Write config.json
        config_json = '{\n  "loop": ' + ('true' if config['loop'] else 'false') + ',\n  "frames": [\n'
        for idx, (fname, dur) in enumerate(zip(frame_files, config['duration'])):
            config_json += f'    {{"file": "{fname}", "duration": {dur}}}'
            if idx < len(frame_files) - 1:
                config_json += ','
            config_json += '\n'
        config_json += '  ]\n}\n'

        config_path = os.path.join(emotion_dir, 'config.json')
        with open(config_path, 'w') as f:
            f.write(config_json)
        print(f"  Generated {config_path}")

    print(f"\nAll emotions generated in {emotions_dir}")

if __name__ == '__main__':
    main()
