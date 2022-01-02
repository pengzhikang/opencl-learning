if [ ! -d build ]; then
    mkdir build
fi
if [ -e test]; then
    rm test
fi
cd build
rm -rf ./*
cmake ..
make
cd ..
./test