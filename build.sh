g++ -Ofast -march=native -fPIC -shared -c tsc_clock.cc -o tsc_clock.o -Wattributes
g++ -fPIC -shared tsc_clock.o -o libtsc_clock.so