[![Build Status](https://travis-ci.org/mbrossard/threadpool.svg?branch=master)](https://travis-ci.org/mbrossard/threadpool)

A simple C thread pool implementation
=====================================

Based on [mbrossard's](https://github.com/mbrossard/threadpool) and [chunleiguo's](https://github.com/chunleiguo/threadpool) work. 

Currently, the implementation:
 * Works with pthreads only. Can be compiled on Linux and Windows (pthread wrapper included).
 * Starts all threads on creation of the thread pool.
 * Stores the called function's arguments in a static array for performance.
 * Waits until all tasks are done if the queue is full, before queuing the new task.
 * Stops and joins all worker threads on destroy.
 * Could be used in a MATLAB MEX file.
 
General usage
=====================================

The threadpool is pretty simple and self explanatory to use.
Call threadpool_initialize to... initialize the threadpool.
Call threadpool_add to add a task to the threadpool. A task consists of a function and an argument. If you need more than one argument, use a structure.
Call threadpool_terminate to terminate the threadpool. It will wait for all the tasks to be done, shutdown the threads and free the allocated memory.
 
MATLAB MEX Usage
=====================================
 
Replace respectively printf, malloc, free with mexPrintf, mxMalloc, mxFree. If you need memory persistence from one run to the next, you have to run mexMakeMemoryPersistent on the allocated threadpool. That's it.
