/* $Id: insertsort.c,v 1.2 2005/04/04 11:34:58 csg Exp $ */

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
/*  FILE: insertsort.c                                                   */
/*  SOURCE : Public Domain Code                                          */
/*                                                                       */
/*  DESCRIPTION :                                                        */
/*                                                                       */
/*     Insertion sort for 10 integer numbers.                            */
/*     The integer array insertsort_a[] is initialized in main function.            */
/*                                                                       */
/*  REMARK :                                                             */
/*                                                                       */
/*  EXECUTION TIME :                                                     */
/*                                                                       */
/*                                                                       */
/*************************************************************************/

unsigned int insertsort_a[11];

void main()
{
  int  i,j, temp;

  i = 2;
  while(i <= 10){
      j = i;
      while (insertsort_a[j] < insertsort_a[j-1] && j-1>=0)
      {
            temp = insertsort_a[j];
            insertsort_a[j] = insertsort_a[j-1];
            insertsort_a[j-1] = temp;
            j--;
      }
      i++;
}
}


