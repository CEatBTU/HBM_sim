# HBM Accelerator
A SystemC simulation for HBM technology. Simulates a complete system (Interconnect + HBM memory end), and it is tunned to simulate the AMD/Xilinx interconnect. 



config/configuration.json has the basic hardware configuration for the HBM. This file should be edited to fit with a specific HBM architecture.

The default version is configured to have 8 switches, 2 stacks (each of 32Gb), for a total of 32 Pseudochannels.

## Configuration files paths.

There are a few lines of code from the sources that are pointing to the configuration of the HBM. These paths need to be checked before compiling. 

XilinxSwitch.cpp line 15. point to correct path 

if used: 
benchmark.cpp line 68 point to correct path config/configuration.json
line 215 pint to correct path config/DRAMSys
                

## Benchmarking
This repo includes some benchmarks to test the simulation. 
/benchmarks/benchmark.cpp can be used to test throughput with different memory access patterns: Single Channel Stride (SCS), Single Channel Random (SCRA), Cross Channel Stride (CCS), and Cross Channel Random (CCRA).


## Authors and acknowledgment
This work started as a Bachelor Thesis of Hiwa Khamo. 

