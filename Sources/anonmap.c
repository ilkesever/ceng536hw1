#include<stdio.h>
#include<signal.h>
#include<fcntl.h>
#include<sys/mman.h>
#include<errno.h>
#include<unistd.h>

/* parallel multiprocess multiplication with shared memory
   example. All slave process are descendants of same
   process, we don't need to mmap a real file, use
   MAP_ANON
*/

double *area;

double *mo1 , *mo2 , *mres;

#define mat(offset, cols, i, j) offset[(i)*cols+j]

void readmat(FILE *fp, double *off, int m, int n)
{
	int i, j;
	double *p=off;
		
	/* row-wise read doubles as p[0]*/
	for (i=0; i<m; i++)
		for (j=0; j<n; j++)  { 
		fscanf(fp,"%lf", p);
		p++;
	}
}

void multiplyrow(const double *mo1, const double *mo2, double *mres, 
		int i,  int r, int n) 
{
	int k, j;
	double s;

	for (j=0; j < n; j++) {
		s=0.0;
		for (k = 0; k < r ; k++) 
			s += mat(mo1, r, i, k)*mat(mo2, n, k, j);
		mat(mres, n, i, j) = s;
	}
}

int main(int argc, char *argv[])
{
	FILE *finp;
	int key;
	int m, r, n;
	int i,k,j;

	if (argc != 2) {
		fprintf(stderr,"usage: %s inputfile\n", argv[0]);
		return -1;
	}
	finp = fopen(argv[1], "r");
	if (finp == NULL) {
		perror("open");
		return 1;
	}
	/* read m, k, n from the file (m x r) * (r x n) = m x n */
	fscanf(finp, "%d %d %d", &m, &r, &n);

	
	/*  size stores both operands and the result */
	/* you don't need file descriptor, -1 makes it ANONYMOUS */
	area=(double *)mmap(0, (m*r+r*n+m*n)*sizeof(double), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS,-1,0);

	if (area==NULL) {
		perror("mmap:");
		return -3;
	}

	mo1 = area;		/* pointer offset for operand 1 */
	mo2 = area + m*r;		/* pointer offset for operand 2 */
	mres = mo2 + r*n;		/* pointer offset for result */

	readmat(finp, mo1, m, r);
	readmat(finp, mo2, r, n);
	fclose(finp);		/* close since not necessary anymore  */

	for (i=0; i<m ; i++) /* for each row of target */
		if (!fork()) {
			/* since mmap'ed area inherited and is shared, not copied */
			multiplyrow(mo1,mo2,mres, i, r, n); 
			sleep(2); /* to avoid instant completion */
			
			return 0;
		}	

	for (i=0; i<m ; i++)  /* wait for all children to terminate */
		wait(&k);

	for (i=0; i<m ; i++) {
		for (j=0; j<n ; j++)
			printf("%10.5f ", mat(mres, n, i, j));
		printf("\n");
	}

	return 0;
}




