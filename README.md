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


