#!/bin/bash

# small helper, created with Claude, to create an adaptive icon for Android
# edit the svg, then use this tool to create the artifacts needed to build the app

rsvg-convert -w 48 -h 48 icons/subsurface-mobile-icon-foreground-new.svg -o android-mobile/res/mipmap-mdpi/ic_launcher_foreground.png
rsvg-convert -w 72 -h 72 icons/subsurface-mobile-icon-foreground-new.svg -o android-mobile/res/mipmap-hdpi/ic_launcher_foreground.png
rsvg-convert -w 96 -h 96 icons/subsurface-mobile-icon-foreground-new.svg -o android-mobile/res/mipmap-xhdpi/ic_launcher_foreground.png
rsvg-convert -w 144 -h 144 icons/subsurface-mobile-icon-foreground-new.svg -o android-mobile/res/mipmap-xxhdpi/ic_launcher_foreground.png
rsvg-convert -w 192 -h 192 icons/subsurface-mobile-icon-foreground-new.svg -o android-mobile/res/mipmap-xxxhdpi/ic_launcher_foreground.png

