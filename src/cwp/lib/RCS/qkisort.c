/* Copyright (c) Colorado School of Mines, 2011.*/
/* All rights reserved.                       */

/*****************************************************************************
index sort functions based on C. A. R. Hoare's quicksort algorithm:

qkisort		sort an array of indices i[] so that 
		a[i[0]] <= a[i[1]] <= ... <= a[i[n-1]]

qkifind		partially sort an array of indices i[] so that the 
		index i[m] has the value it would have if the entire
		array of indices were sorted such that 
		a[i[0]] <= a[i[1]] <= ... <= a[i[n-1]]
*****************************************************************************/

#define NSTACK 50	/* maximum sort length is 2^NSTACK */
#define NSMALL 7	/* size of array for which insertion sort is fast */
#define FM 7875		/* constants used to generate random pivots */
#define FA 211
#define FC 1663

static void
qkipart (int n, float a[], int i[], int p, int q, int *j, int *k)
/*****************************************************************************
quicksort partition (FOR INTERNAL USE ONLY):
take the value x of a random element from the subarray a[p:q] of
a[0:n-1] and rearrange indices in the subarray i[p:q] in such a way
that there exist integers j and k with the following properties:
  p <= j < k <= q, provided that p < q
  a[i[l]] <= x,  for p <= l <= j
  a[i[l]] == x,  for j < l < k
  a[i[l]] >= x,  for k <= l <= q
note that this effectively partitions the subarray with bounds
[p:q] into lower and upper subarrays with bounds [p:j] and [k:q]
******************************************************************************
Input:
n		number of elements in a
a		array[p:q]
i		array[p:q] of indices to be rearranged
p		lower bound of subarray; must be less than q
q		upper bound of subarray; must be greater then p

Output:
i		array[p:q] of indices rearranged
j		upper bound of lower output subarray
k		lower bound of upper output subarray
******************************************************************************
Notes:
This function is adapted from procedure partition by
Hoare, C.A.R., 1961, Communications of the ACM, v. 4, p. 321.
******************************************************************************
Author:  Dave Hale, Colorado School of Mines, 01/13/89
*****************************************************************************/
{
	int pivot,left,right,temp;
	float apivot;
	static long int seed=0L;
 
	/* choose random pivot element between p and q, inclusive */
	seed = (seed*FA+FC)%FM;
	pivot = p+(q-p)*(float)seed/(float)FM;
	if (pivot<p) pivot = p;
	if (pivot>q) pivot = q;
	apivot = a[i[pivot]];

	/* initialize left and right pointers and loop until break */
	for (left=p,right=q;;) {
		/*
		 * increment left pointer until either
		 * (1) an element greater than the pivot element is found, or
		 * (2) the upper bound of the input subarray is reached
		 */
		while (a[i[left]]<=apivot && left<q) left++;
 
		/*
		 * decrement right pointer until either
		 * (1) an element less than the pivot element is found, or
		 * (2) the lower bound of the input subarray is reached
		 */
		while (a[i[right]]>=apivot && right>p) right--;
 
		/* if left pointer is still to the left of right pointer */
		if (left<right) {

			/* exchange left and right indices */
			temp = i[left];
			i[left++] = i[right];
			i[right--] = temp;
		} 
		/* else, if pointers are equal or have crossed, break */
		else break;
	}
	/* if left pointer has not crossed pivot */
	if (left<pivot) {

		/* exchange indices at left and pivot */
		temp = i[left];
		i[left++] = i[pivot];
		i[pivot] = temp;
	}
	/* else, if right pointer has not crossed pivot */
	else if (pivot<right) {

		/* exchange indices at pivot and right */
		temp = i[right];
		i[right--] = i[pivot];
		i[pivot] = temp;
	}
	/* left and right pointers have now crossed; set output bounds */
	*j = right;
	*k = left;
}

static void
qkiinss (int n, float a[], int i[], int p, int q)
/*****************************************************************************
quicksort insertion sort (FOR INTERNAL USE ONLY):
Sort a subarray of indices bounded by p and q so that
a[i[p]] <= a[i[p+1]] <= ... <= a[i[q]]
******************************************************************************
Input:
n		number of elements in a
a		subarray[p:q] containing elements
i		subarray[p:q] containing indices to be sorted
p		lower bound of subarray; must be less than q
q		upper bound of subarray; must be greater then p

Output:
i		subarray[p:q] of indices sorted
******************************************************************************
Notes:
Adapted from Sedgewick, R., 1983, Algorithms, Addison Wesley, p. 96.
******************************************************************************
Author:  Dave Hale, Colorado School of Mines, 01/13/89
*****************************************************************************/
{
	int j,k,ij;
	float aij;

	for (j=p+1; j<=q; j++) {
		for (ij=i[j],aij=a[ij],k=j; k>p && a[i[k-1]]>aij; k--)
			i[k] = i[k-1];
		i[k] = ij;
	}
}

void
qkisort (int n, float a[], int i[])
/*****************************************************************************
Sort an array of indices i[] so that 
a[i[0]] <= a[i[1]] <= ... <= a[i[n-1]]
******************************************************************************
Input:
n		number of elements in array a
a		array[n] elements
i		array[n] indices to be sorted

Output:
i		array[n] indices sorted
******************************************************************************
Notes:
n must be less than 2^NSTACK, where NSTACK is defined above.

This function is adapted from procedure quicksort by
Hoare, C.A.R., 1961, Communications of the ACM, v. 4, p. 321;
the main difference is that recursion is accomplished
explicitly via a stack array for efficiency; also, a simple
insertion sort is used to sort subarrays too small to be
partitioned efficiently.
******************************************************************************
Author:  Dave Hale, Colorado School of Mines, 01/13/89
*****************************************************************************/
{
	int pstack[NSTACK],qstack[NSTACK],j,k,p,q,top=0;

	/* initialize subarray lower and upper bounds to entire array */
	pstack[top] = 0;
	qstack[top++] = n-1;

	/* while subarrays remain to be sorted */
	while(top!=0) {

		/* get a subarray off the stack */
		p = pstack[--top];
		q = qstack[top];

		/* while subarray can be partitioned efficiently */
		while(q-p>NSMALL) {

			/* partition subarray into two subarrays */
			qkipart(n,a,i,p,q,&j,&k);

			/* save larger of the two subarrays on stack */
			if (j-p<q-k) {
				pstack[top] = k;
				qstack[top++] = q;
				q = j;
			} else {
				pstack[top] = p;
				qstack[top++] = j;
				p = k;
			}
		}
		/* use insertion sort to finish sorting small subarray */
		qkiinss(n,a,i,p,q);
	}
}

void
qkifind (int m, int n, float a[], int i[])
/*****************************************************************************
Partially sort an array of indices i[] so that the index i[m] has the
value it would have if the entire array of indices were sorted such that 
a[i[0]] <= a[i[1]] <= ... <= a[i[n-1]]
******************************************************************************
Input:
m		index of element to be found
n		number of elements in array a
a		array[n] elements
i		array[n] indices to be partially sorted

Output:
i		array[n] indices partially sorted sorted
******************************************************************************
Notes:
This function is adapted from procedure find by
Hoare, C.A.R., 1961, Communications of the ACM, v. 4, p. 321.
******************************************************************************
Author:  Dave Hale, Colorado School of Mines, 01/13/89
*****************************************************************************/
{
	int j,k,p,q;

	/* initialize subarray lower and upper bounds to entire array */
	p = 0;  q = n-1;

	/* while subarray can be partitioned efficiently */
	while(q-p>NSMALL) {

		/* partition subarray into two subarrays */
		qkipart(n,a,i,p,q,&j,&k);

		/* if desired value is in lower subarray, then */
		if (m<=j)
			q = j;

		/* else, if desired value is in upper subarray, then */
		else if (m>=k)
			p = k;
		
		/* else, desired value is between j and k */
		else
			return;
	}
			
	/* completely sort the small subarray with insertion sort */
	qkiinss(n,a,i,p,q);
}
