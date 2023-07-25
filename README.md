# usage

## tracing
start the program with LD_PRELOAD=libmallocCatcher.so <yourExecutable > output.txt 2>&1

start tracing 

gdb -p <pidof yourExecutable>
set MyHeap.enabled=1

# offline analysis with leakfinder

provide the output.txt to leakfinder

leakfinder output.txt

# open issues

calloc is not overriden, because it segfaults on startup
[Title](https://bugs.chromium.org/p/chromium/issues/detail?id=28244)

## performance

would be interesting if backtrace_symbols_fd(buffer, cnt, logfd) is not needed and the runtime addresses are used.
But how to get symbols/line number then?

with /proc/<pidof yourExecutable>/maps ?