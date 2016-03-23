
int main (void)
{
	int x = nondet (1, 10);
	if (x < 1) __poet_fail ();
}
