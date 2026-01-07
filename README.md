# Compile
gcc -o stickynote stickynote.c `pkg-config --cflags --libs gtk+-3.0` -Wall -O2

# Run
./stickynote

# Run multiple notes with different IDs
./stickynote 1
./stickynote 2
