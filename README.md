# What

This is a version of pcsx4all with MIPS-to-ARM64 dynamic JIT (lightrec), originally forked from pcsx4all by pcercuei. I then patched it for MNT Reform to use SDL2. This allows for smooth GL scaling of the screen and operation on wayland.

# How to build

```
git clone git://git.savannah.gnu.org/lightning.git
cd lightning
autoreconf --install
./configure
make -j $(nproc)
sudo make install
cd ..

git clone https://github.com/pcercuei/lightrec.git
cd lightrec
mkdir build
cd build
cmake ..
make -j $(nproc)
sudo make install
cd ../..

sudo ldconfig /usr/local/lib

make -j $(nproc)
```

# Hints

- Put a BIOS file (scph1001.bin, lowercase!) in ~/.pcsx4all/bios
- Select the BIOS file under Core Settings -> Set BIOS file
- Switch off HLE emulated BIOS
- Navigate menu with cursor keys and CTRL to select
- Keyboard mapping (you can customize these in src/port/sdl/port.cpp):
  - Cursors: D-Pad
  - Enter: Start
  - Backspace: Select
  - x: Circle
  - a: Square
  - s: Triangle
  - z: Cross
  - q: L1
  - w: R1
  - e: L2
  - r: R2

