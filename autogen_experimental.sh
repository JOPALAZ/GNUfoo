aclocal
autoconf
automake --add-missing
./configure CFLAGS="-O3 -finline-functions -funroll-loops -fprefetch-loop-arrays -fomit-frame-pointer -march=native"
make
