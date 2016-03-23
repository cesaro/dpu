int main (void)
{
	int x, y;

	x = 1;
	y = nondet (-200, 200);

	if (y <= 10)
	{
		y = 10;
	}
	else
	{
		while (x < y)
		{
			x = 2 * x;
			y = y - 1;
		}
	}

	x = y + 1;
	if (x <= 0) {
    __poet_fail ();
	}
//	assert (x > 0); // it holds
}
