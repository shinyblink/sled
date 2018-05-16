#!/bin/sh
/opt/devkitpro/tools/bin/3dsxtool sled sled.3dsx

lftp <<CMD
open ftp://192.168.1.209:5000
put sled.3dsx -o /3ds/sled.3dsx
CMD
