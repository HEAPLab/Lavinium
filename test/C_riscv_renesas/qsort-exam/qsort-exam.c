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
/*  FILE: qsort-exam.c                                                   */
/*  SOURCE : Numerical Recipes in C - The Second Edition                 */
/*                                                                       */
/*  DESCRIPTION :                                                        */
/*                                                                       */
/*     Non-recursive version of quick sort algorithm.                    */
/*     This example sorts 20 floating point numbers, sort_arr[].              */
/*                                                                       */
/*  REMARK :                                                             */
/*                                                                       */
/*  EXECUTION TIME :                                                     */
/*                                                                       */
/*                                                                       */
/*************************************************************************/
#define SWAP(a,b) temp=(a);(a)=(b);(b)=temp;
#define M 7
#define NSTACK 50

float sort_arr[20] = {
  5, 4, 10.3, 1.1, 5.7, 100, 231, 111, 49.5, 99,
  10, 150, 222.22, 101, 77, 44, 35, 20.54, 99.99, 88.88
};

int sort_istack[100];

void sort_f(unsigned long n)
{
	unsigned long i,ir=n,j,k,l=1;
	int jstack=0;
	int flag;
	float a,temp;

	flag = 0;
	for (;;) {
		if (ir-l < M) {
			for (j=l+1;j<=ir;j++) {
				a=sort_arr[j];
				for (i=j-1;i>=l;i--) {
					if (sort_arr[i] <= a) break;
					sort_arr[i+1]=sort_arr[i];
				}
				sort_arr[i+1]=a;
			}
			if (jstack == 0) break;
			ir=sort_istack[jstack--];
			l=sort_istack[jstack--];
		} else {
			k=(l+ir) >> 1;
			SWAP(sort_arr[k],sort_arr[l+1])
			if (sort_arr[l] > sort_arr[ir]) {
				SWAP(sort_arr[l],sort_arr[ir])
			}
			if (sort_arr[l+1] > sort_arr[ir]) {
				SWAP(sort_arr[l+1],sort_arr[ir])
			}
			if (sort_arr[l] > sort_arr[l+1]) {
				SWAP(sort_arr[l],sort_arr[l+1])
			}
			i=l+1;
			j=ir;
			a=sort_arr[l+1];
			for (;;) {
				i++; while (sort_arr[i] < a) i++;
				j--; while (sort_arr[j] > a) j--;
				if (j < i) break;
				SWAP(sort_arr[i],sort_arr[j]);
			}
			sort_arr[l+1]=sort_arr[j];
			sort_arr[j]=a;
			jstack += 2;

			if (ir-i+1 >= j-l) {
				sort_istack[jstack]=ir;
				sort_istack[jstack-1]=i;
				ir=j-1;
			} else {
				sort_istack[jstack]=j-1;
				sort_istack[jstack-1]=l;
				l=i;
			}
		}
	}
}

void main()
{
  sort_f(20);
}

