# Made during the Embedded and Real-Time Systems course, ECE AUTH, 2022.

A modified version of the prod-cons.c program, which calculates the average waiting time for each item, from the moment it is put in the queue, until the moment
it exits it, using P producer threads, and Q consumer threads.

Run using:

```
gcc -o out prod-cons.c -lm -lpthread
./out
```
