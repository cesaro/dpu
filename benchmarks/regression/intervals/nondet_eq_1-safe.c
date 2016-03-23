
int main (void)
{
	int x = nondet (1, 5);
	if (x == 7) __poet_fail ();
}
