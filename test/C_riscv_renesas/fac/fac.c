/* MDH WCET BENCHMARK SUITE */
/*
 * Changes: CS 2006/05/19: Changed loop bound from constant to variable.
 */

int fac_n;

int fac_f (int n)
{
  if (n == 0)
    return 1;
  else
    return (n * fac_f (n-1));
}

void main() {
  int s = 0;
  // loop bound = 5
  for (int i = 0;  i <= fac_n; i++)
      s += fac_f (i);
}