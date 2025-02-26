# A parallel prefix-sum implementation

This program uses the [Hillis and Steele](https://en.wikipedia.org/wiki/Prefix_sum#Algorithm_1:_Shorter_span,_more_parallel) parallel prefix-sum algorithm, where ideally there are as many processors as elements.
In this ideal case, the algorithm runs in O(log2(n)) time

## Building

You can build this program with the following instructions
```
git clone https://github.com/siddharth-narayan/CS4348-HW1.git
cd CS4348-HW1
make
```


## Usage instructions
```
$ parallel_prefix_sum  --help
Usage:
  By default uses 16 numbers and  2 processes
  parallel_prefix_sum  [OPTIONS]

Options:
  -f <path>            Sets filepath for the array input. If not specified, an array will be generated automatically.
  -s <num>             Sets the number of integers in the generated array that will be used for the prefix sum algorithm.
  -j <num>             Sets the number of processes that will be created to run the prefix sum algorithm in parallel.
  -d <level>           Sets the debug level. This flag can be used standalone without a level, which will be basic debugging.
  -h or --help         Print this help dialogue.
```
