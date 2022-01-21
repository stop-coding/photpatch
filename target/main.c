#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define lengthof(x) (sizeof(x) / sizeof(x[0]))


void helloworld(void)
{
    printf(" i will running always....\n");
}

int cmp(const void *a, const void *b)
{
    helloworld();
	sleep(1);
}

int main(int argc, char **argv)
{
	int data[] = {1, 2, 3, 4};
	int needle = 4;

	// ensure we backtrace through dynamically linked symbols too
    while(1)
	    bsearch(&needle, data, lengthof(data), sizeof(data[0]), cmp);

	// not reached
	return 1;
}