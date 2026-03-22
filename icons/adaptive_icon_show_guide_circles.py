"""create a visual overlay of the adaptive icon with the guide circles"""

# truly a tool only useful if either (a) we ever change the Subsurface-mobile icon
# or (more likely) (b) the next time Google makes random changes to how icons are shown
# on the launcher screen. It basically helps you make choices how to mangle our icon so
# that it doesn't look ridiculous on the Android launcher screen

import re
import sys
import subprocess

with open("icons/subsurface-mobile-icon-foreground-new.svg", "r", encoding="utf-8") as f:
    fg_content = f.read()

match = re.search(r"(<!-- Foreground.*?)(</svg>)", fg_content, re.DOTALL)
defs_match = re.search(r"(<defs.*?</defs>)", fg_content, re.DOTALL)
if match is None or defs_match is None:
    print("Can't find the Foreground or defs element - bailing")
    sys.exit(1)
g_content = match.group(1)
defs = defs_match.group(1)

composite = f"""<?xml version="1.0" encoding="UTF-8"?>
<svg width="540" height="540" viewBox="0 0 540 540"
     xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink">
  {defs}
  <rect width="540" height="540" fill="#2d5b9a"/>
  {g_content}
  <circle cx="270" cy="270" r="178" fill="none" stroke="red" stroke-width="2" stroke-dasharray="8,4"/>
  <circle cx="270" cy="270" r="194" fill="none" stroke="yellow" stroke-width="2" stroke-dasharray="8,4"/>
</svg>"""

with open("/tmp/mobile-icon-with-guide-circles.svg", "w", encoding="utf-8") as f:
    f.write(composite)
subprocess.run(
    [
        "rsvg-convert",
        "-w",
        "540",
        "-h",
        "540",
        "/tmp/mobile-icon-with-guide-circles.svg",
        "-o",
        "/tmp/mobile-icon-with-guide-circles.png",
    ],
    check=True,
)
print("created /tmp/mobile-icon-with-guide-circles.png")
