# tsc_clock
Low latency high precision clock. 
## how to compile
```bash
mkdir build && cd build
cmake .. && make
```
Provide single-threaded and multi-process versions of TSC clock, default compilation uses single-threaded version, if you need to use multi-process version, you need to add the compilation parameter -DTSC_GLOBAL=true when compiling
TSC clock needs to be calibrated with an external clock source, the default compilation will be calibrated automatically, if manual calibration is required, increase the compilation parameter -DSYNC_CALIBRATE=true
Compile option LINK_INIT Select whether to initialize during linking, defaults to true, and is valid only for single-threaded versions
## app
app/tsc_tool tsc_tool is used to manage the multi-process version TSC clock, and provides initialization, destruction, and calibration clock functions
```bash
Usage:
tsc options:
   -i|--init       Init tsc environment
   -c|--clean      Clean tsc environment
   -d|--daemon     Calibrate tsc clock periodcally
```
## benmark
rdsys_latency: 26.3636, rdtsc_latency: 11.3336, rdns_latency: 11.4636