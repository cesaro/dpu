
int main (void)
{
	int x = nondet (1, 10);
	if (x > 15) __poet_fail ();
}
