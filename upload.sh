#write to flash AND set fuse
sudo avrdude -c jtag2updi -p t1616 -P /dev/ttyUSB0 -e -U flash:w:a.out -Ufuse2:w:0x02:m -B5
