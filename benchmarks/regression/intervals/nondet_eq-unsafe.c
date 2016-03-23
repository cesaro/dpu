
int main (void)
{
	int x = nondet (1, 5);
	if (x == 3) __poet_fail ();
}

