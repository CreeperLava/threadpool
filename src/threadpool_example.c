#include "threadpool.h"
#include "threadpool.c"

double myFunction(double a, double b, double * res) {
	int tail;

  // it's a very inefficient example, it's there only to show how the threadpool is used
	for (int i = 0; i < 100; i++)
    tail = global_threadpool->tail; // store the index of the next available slot in the queue
    global_threadpool->args[tail].res = res;
    global_threadpool->args[tail].a = a;
    global_threadpool->args[tail].b = b; // fill the argument for this slot
  
    // add task to the threadpool by sending the function and the argument
		threadpool_add(&myFunctionWrapper, &global_threadpool->args[tail]);
}

void * myFunctionWrapper(void * mon_argument) {
	t_ma_structure * structure = mon_argument;
	*(structure->res) += structure->a + structure->b;
}

int main() {
	double res = 0;
	threadpool_initialize();
	myFunction(1, 2, &res);
  
  return 0;
}
