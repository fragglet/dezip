tcc -B -y -mc -c -f- crc32.c
tcc -O -Z -G -M -y -mc -f- %1 unzip crc32.obj
csc unzip
