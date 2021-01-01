#!/bin/sh

/etc/init.d/framework stop
/usr/bin/lipc-set-prop -- com.lab126.powerd preventScreenSaver 1
./prboom -iwad /mnt/us/doom.wad -file /mnt/us/prboom.wad -nosound -nomusic -nosfx -warp 1 1
/usr/bin/lipc-set-prop -- com.lab126.powerd preventScreenSaver 0
/etc/init.d/framework start	