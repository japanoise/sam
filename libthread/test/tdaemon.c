#include <u.h>
#include <fmt.h>
#include <thread.h>

void proc(void *v) {
	sleep(5 * 1000);
	print("still running\n");
}

void threadmain(int argc, char **argv) { proccreate(proc, NULL, 32768); }
