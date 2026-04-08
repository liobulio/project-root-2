# Assignment #3 : Memory Management


## Authors
- Hengkun Zhang [261238813]
- Zili Zhuang [261238809]

## Startercode Declaration 
- This implementation uses our own code form assignment#2.

## Split of Work
* Split Shell Memory (1.2.1)
  * into frame store and variable store 
* Paging Infrastructure
* Allow Duplicate Script Names in `exec`
* Demand Paging (1.2.2)
* LRU Eviction (1.2.3)

## The Big Picture 
### Before
When we run `exec`, the entire content of every program got loaed into a flat array in memeory before execution started.
### After 
Memory is splitted into fixed size frames(each hold 3 lines of code). The program is loaded lazily, more pages are loaded on-demand when we encounter a page fault(when the process tries to execute a line that is not in physical memory yet)  


## How to Compile and Run
Clean before using 
```
make clean
```
Compile the shell
```
make mysh framesize=X varsize=Y
```
Interactive mode
```
./mysh
```
Batch mode
```
./mysh < <path>/testfile.txt
```
run before testing tc3.txt
```bash
make clean && make mysh framesize=21 varmemsize=10
```
run before testing tc5.txt
```bash
make clean && make mysh framesize=6 varmemsize=10
```
