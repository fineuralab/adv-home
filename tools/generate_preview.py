#!/usr/bin/env python3
"""Arrange framebuffer test captures without substituting a UI mockup."""
import argparse
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

parser = argparse.ArgumentParser()
parser.add_argument("directory", type=Path)
args = parser.parse_args()
canvas = Image.new("RGB", (1016, 652), "#0b1017")
draw = ImageDraw.Draw(canvas)
font = ImageFont.truetype(str(Path(__file__).resolve().parent.parent / "assets/fonts/Roboto[wdth,wght].ttf"), 20)
for index, (name, title) in enumerate([("home", "Home"), ("files", "SD card"),
                                        ("confirm", "Confirm install"), ("settings", "Settings")]):
    x, y = 18 + (index % 2) * 500, 10 + (index // 2) * 322
    draw.text((x, y), title, font=font, fill="#b8c9db")
    screen = Image.open(args.directory / "screens" / (name + ".ppm"))
    canvas.paste(screen.resize((480, 270), Image.Resampling.NEAREST), (x, y + 32))
version = (Path(__file__).resolve().parent.parent / "core/version.h").read_text().split('"')[1]
canvas.save(args.directory / ("ADV-Home-" + version + "-preview.png"))
