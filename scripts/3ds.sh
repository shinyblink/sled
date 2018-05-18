#!/bin/sh
# Script to load sled via 3dslink.
/opt/devkitpro/tools/bin/3dsxtool sled sled.3dsx
/opt/devkitpro/tools/bin/3dslink sled.3dsx
