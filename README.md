# memcopytest
Examples of my attempts at copying memory quickly.

## Building
build using

```
make all RELEASE=y
```

## Running
So far the best results I've seen have been when running using the following command:

```
numactl -m 0 -N 0 bin/release/x86-64-GCC-Linux/memcpytest -b 8 -i 10 -t boring -w 8
```

This does the following (in left to right order):
- use numactl to lock the threads and memory allocation to the first CPU in the system
- operate with source and destination buffers of 8GB
- run 10 iterations of the test
- use the 'boring' implementation (this is marginally faster than my hugepages implementation)
- run with 8 worker threads

Use the following command to run with 16GB buffers (remove the numactl part if not applicable):

```
numactl -m 0 -N 0 bin/release/x86-64-GCC-Linux/memcpytest -b 16 -i 10 -t boring -w 8
```

The test system used had the following specifications:
- 2x Intel Xeon E5-2695 v2 CPUs
- Supermicro something something C602 dual socket motherboard with a weird E-ATX mounting hole layout. Good thing I had a drill to hand.
- 4x 8GB DDR3 REG ECC 1066MHz (Samsung?) memory attached to CPU 0 in quad-channel mode
- GTX 970 GPU
- Intel 900p 280GB SSD
