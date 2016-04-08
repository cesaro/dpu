#include <stdio.h>

int mult (int a, int b)
{
	int i, c;
	c = 0;
	for (i = 0; i < b; ++i) c += a;
	return c;
}

int main (int argc, char ** argv)
{
	printf ("result %d\n", mult (argc, 5));
	return 0;
}
