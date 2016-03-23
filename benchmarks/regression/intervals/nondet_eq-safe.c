
int main (void)
{
	int x = nondet (6, 10);
	if (x == 5) __poet_fail ();
}
