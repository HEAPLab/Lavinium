/*************************************************************************/
/*                                                                       */
/*   SNU-RT Benchmark Suite for Worst Case Timing Analysis               */
/*   =====================================================               */
/*                              Collected and Modified by S.-S. Lim      */
/*                                           sslim@archi.snu.ac.kr       */
/*                                         Real-Time Research Group      */
/*                                        Seoul National University      */
/*                                                                       */
/*                                                                       */
/*        < Features > - restrictions for our experimental environment   */
/*                                                                       */
/*          1. Completely structured.                                    */
/*               - There are no unconditional jumps.                     */
/*               - There are no exit from loop bodies.                   */
/*                 (There are no 'break' or 'return' in loop bodies)     */
/*          2. No 'switch' statements.                                   */
/*          3. No 'do..while' statements.                                */
/*          4. Expressions are restricted.                               */
/*               - There are no multiple expressions joined by 'or',     */
/*                'and' operations.                                      */
/*          5. No library calls.                                         */
/*               - All the functions needed are implemented in the       */
/*                 source file.                                          */
/*                                                                       */
/*                                                                       */
/*************************************************************************/
/*                                                                       */
/*  FILE: select.c                                                       */
/*  SOURCE : Numerical Recipes in C - The Second Edition                 */
/*                                                                       */
/*  DESCRIPTION :                                                        */
/*                                                                       */
/*     A function to select the Nth largest number in the floating poi-  */
/*     nt array select_arr[].                                                   */
/*     The parameters to function select are k and n. Then the function  */
/*     selects k-th largest number out of n original numbers.            */
/*                                                                       */
/*  REMARK :                                                             */
/*                                                                       */
/*  EXECUTION TIME :                                                     */
/*                                                                       */
/*                                                                       */
/*************************************************************************/

#define SWAP(a,b) temp=(a);(a)=(b);(b)=temp;

float select_arr[20] = {
  5, 4, 10.3, 1.1, 5.7, 100, 231, 111, 49.5, 99,
  10, 150, 222.22, 101, 77, 44, 35, 20.54, 99.99, 888.88
};

unsigned long select_k;


float select_f(unsigned long k, unsigned long n)
{
	unsigned long i,ir,j,l,mid;
	float a,temp;
	int flag, flag2;

	l=1;
	ir=n-1;
	flag = flag2 = 0; 
	while (!flag) {
		if (ir <= l+1) {
			if (ir == l+1) 
			  if (select_arr[ir] < select_arr[l]) {
			    SWAP(select_arr[l],select_arr[ir])
			      }
			flag = 1;
		} else if (!flag) {
			mid=(l+ir) >> 1;
			SWAP(select_arr[mid],select_arr[l+1])
			if (select_arr[l+1] > select_arr[ir]) {
				SWAP(select_arr[l+1],select_arr[ir])
			}
			if (select_arr[l] > select_arr[ir]) {
				SWAP(select_arr[l],select_arr[ir])
			}
			if (select_arr[l+1]> select_arr[l]) {
				SWAP(select_arr[l+1],select_arr[l])
			}
			i=l+1;
			j=ir;
			a=select_arr[l];
			while (!flag2) {
				i++; 
				while (select_arr[i] < a) i++;
				j--; 
				while (select_arr[j] > a) j--;
				if (j < i) flag2 = 1;
				if (!flag2) SWAP(select_arr[i],select_arr[j]);
				
			}
			select_arr[l]=select_arr[j];
			select_arr[j]=a;
			if (j >= k) ir=j-1;
			if (j <= k) l=i;
		}

	}
	return select_arr[k];
}

void main()
{
  select_f(select_k, 20);
}

