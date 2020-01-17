#!/bin/sh
#timeout 30 asciiquarium
ls --color
fortune | cowsay 
echo $TERM @$(hostname)
sleep 5
