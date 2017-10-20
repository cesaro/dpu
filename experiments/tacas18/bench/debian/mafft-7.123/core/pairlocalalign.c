#include "mltaln.h"

#define DEBUG 0
#define IODEBUG 0
#define SCOREOUT 0


#define NODIST -9999

static char *whereispairalign;
static char *laraparams;
static char foldalignopt[1000];
static int stdout_align;
static int stdout_dist;
static int store_localhom;
static int store_dist;
static int nadd;
static int laste;
static int lastm;
static int lastsubopt;
static int lastonce;

typedef struct _lastres
{
	int score;
	int start1;
	int start2;
	char *aln1;
	char *aln2;
} Lastres;

typedef struct _reg
{
	int start;
	int end;
} Reg;

typedef struct _aln
{
	int nreg;
	Reg *reg1;
	Reg *reg2;
} Aln;

typedef struct _lastresx
{
	int score;
	int naln;
	Aln *aln;
} Lastresx;

#ifdef enablemultithread
typedef struct _jobtable
{
	int i;
	int j;
} Jobtable;

typedef struct _thread_arg
{
	int thread_no;
	int njob;
	Jobtable *jobpospt;
	char **name;
	char **seq;
	char **dseq;
	int *thereisxineachseq;
	LocalHom **localhomtable;
	double **distancemtx;
	double *selfscore;
	char ***bpp;
	Lastresx **lastresx;
	int alloclen;
	pthread_mutex_t *mutex_counter;
	pthread_mutex_t *mutex_stdout;
} thread_arg_t;
#endif

typedef struct _lastcallthread_arg
{
	int nq, nd;
	char **dseq;
	char **qseq;
	Lastresx **lastresx;
#ifdef enablemultithread
	int thread_no;
	int *kshare;
	pthread_mutex_t *mutex;
#endif
} lastcallthread_arg_t;

static void t2u( char *seq )
{
	while( *seq )
	{
		if     ( *seq == 'A' ) *seq = 'a';
		else if( *seq == 'a' ) *seq = 'a';
		else if( *seq == 'T' ) *seq = 'u';
		else if( *seq == 't' ) *seq = 'u';
		else if( *seq == 'U' ) *seq = 'u';
		else if( *seq == 'u' ) *seq = 'u';
		else if( *seq == 'G' ) *seq = 'g';
		else if( *seq == 'g' ) *seq = 'g';
		else if( *seq == 'C' ) *seq = 'c';
		else if( *seq == 'c' ) *seq = 'c';
		else *seq = 'n';
		seq++;
	}
}

static int removex( char *d, char *m )
{
	int val = 0;
	while( *m != 0 )
	{
		if( *m == 'X' || *m == 'x' ) 
		{
			m++;
			val++;
		}
		else 
		{
			*d++ = *m++;
		}
	}
	*d = 0;
	return( val );
}

static void putlocalhom_last( char *s1, char *s2, LocalHom *localhompt, Lastresx *lastresx )
{
	char *pt1, *pt2;
	int naln, nreg;
	int iscore;
	int isumscore;
	int sumoverlap;
	LocalHom *tmppt = localhompt;
	LocalHom *tmppt2;
	LocalHom *localhompt0;
	Reg *rpt1, *rpt2;
	Aln *apt;
	int nlocalhom = 0;
	int len;

//	printf(  "s1=%s\n", s1 );
//	printf(  "s2=%s\n", s2 );


	naln = lastresx->naln;
	apt = lastresx->aln;

	if( naln == 0 ) return;
	while( naln-- )
	{
		rpt1 = apt->reg1;
		rpt2 = apt->reg2;
		nreg = apt->nreg;
		isumscore = 0;
		sumoverlap = 0;
		while( nreg-- )
		{
			if( nlocalhom++ > 0 )
			{
//				printf(  "reallocating ...\n" );
				tmppt->next = (LocalHom *)calloc( 1, sizeof( LocalHom ) );
//				printf(  "done\n" );
				tmppt = tmppt->next;
				tmppt->next = NULL;
			}
			tmppt->start1 = rpt1->start;
			tmppt->start2 = rpt2->start;
			tmppt->end1   = rpt1->end;
			tmppt->end2   = rpt2->end;
			if( rpt1 == apt->reg1 ) localhompt0 = tmppt; // ?
	
//			printf(  "in putlocalhom, reg1: %d-%d (nreg=%d)\n", rpt1->start, rpt1->end, lastresx->nreg );
//			printf(  "in putlocalhom, reg2: %d-%d (nreg=%d)\n", rpt2->start, rpt2->end, lastresx->nreg );
	
			len = tmppt->end1 - tmppt->start1 + 1;
	
//			printf(  "tmppt->start1=%d\n", tmppt->start1 );
//			printf(  "tmppt->start2=%d\n", tmppt->start2 );

//			printf(  "s1+tmppt->start1=%*.*s\n", len, len, s1+tmppt->start1 );
//			printf(  "s2+tmppt->start2=%*.*s\n", len, len, s2+tmppt->start2 );
	
			pt1 = s1 + tmppt->start1;
			pt2 = s2 + tmppt->start2;
			iscore = 0;
			while( len-- )
			{
				iscore += n_dis[(int)amino_n[(int)*pt1++]][(int)amino_n[(int)*pt2++]]; // - offset はいらないかも
//				printf(  "len=%d, %c-%c, iscore(0) = %d\n", len, *(pt1-1), *(pt2-1), iscore );
			}
	
			if( divpairscore )
			{
				tmppt->overlapaa   = tmppt->end2-tmppt->start2+1;
				tmppt->opt = (double)iscore / tmppt->overlapaa * 5.8 / 600;
			}
			else
			{
				isumscore += iscore;
				sumoverlap += tmppt->end2-tmppt->start2+1;
			}
			rpt1++;
			rpt2++;
		}
#if 0
		printf(  "iscore (1)= %d\n", iscore );
		printf(  "al1: %d - %d\n", start1, end1 );
		printf(  "al2: %d - %d\n", start2, end2 );
#endif

		if( !divpairscore )
		{
			for( tmppt2=localhompt0; tmppt2; tmppt2=tmppt2->next )
			{
				tmppt2->overlapaa = sumoverlap;
				tmppt2->opt = (double)isumscore * 5.8 / ( 600 * sumoverlap );
//				printf(  "tmpptr->opt = %f\n", tmppt->opt );
			}
		}
		apt++;
	}
}

static int countcomma( char *s )
{
	int v = 0;
	while( *s ) if( *s++ == ',' ) v++;
	return( v );
}

static float recallpairfoldalign( char **mseq1, char **mseq2, int m1, int m2, int *of1pt, int *of2pt, int alloclen )
{
	static FILE *fp = NULL;
	float value;
	char *aln1;
	char *aln2;
	int of1tmp, of2tmp;

	if( fp == NULL )
	{
		fp = fopen( "_foldalignout", "r" );
		if( fp == NULL )
		{
			printf(  "Cannot open _foldalignout\n" );
			exit( 1 );
		}
	}

	aln1 = calloc( alloclen, sizeof( char ) );
	aln2 = calloc( alloclen, sizeof( char ) );

	readpairfoldalign( fp, *mseq1, *mseq2, aln1, aln2, m1, m2, &of1tmp, &of2tmp, alloclen );

	if( strstr( foldalignopt, "-global") )
	{
		printf(  "Calling G__align11\n" );
		value = G__align11( mseq1, mseq2, alloclen, outgap, outgap );
		*of1pt = 0;
		*of2pt = 0;
	}
	else
	{
		printf(  "Calling L__align11\n" );
		value = L__align11( mseq1, mseq2, alloclen, of1pt, of2pt );
	}

//	value = (float)naivepairscore11( *mseq1, *mseq2, penalty ); // nennnotame

	if( aln1[0] == 0 )
	{
		printf(  "FOLDALIGN returned no alignment between %d and %d.  Sequence alignment is used instead.\n", m1+1, m2+1 );
	}
	else
	{
		strcpy( *mseq1, aln1 );
		strcpy( *mseq2, aln2 );
		*of1pt = of1tmp;
		*of2pt = of2tmp;
	}

//	value = naivepairscore11( *mseq1, *mseq2, penalty ); // v6.511 ha kore wo tsukau, global nomi dakara.

//	fclose( fp ); // saigo dake yatta houga yoi.

//	printf(  "*mseq1 = %s\n", *mseq1 );
//	printf(  "*mseq2 = %s\n", *mseq2 );


	free( aln1 );
	free( aln2 );

	return( value );
}

static void block2reg( char *block, Reg *reg1, Reg *reg2, int start1, int start2 )
{
	Reg *rpt1, *rpt2;
	char *tpt, *npt;
	int pos1, pos2;
	int len, glen1, glen2;
	pos1 = start1;
	pos2 = start2;
	rpt1 = reg1;
	rpt2 = reg2;
	while( block )
	{
		block++;
//		printf(  "block = %s\n", block );
		tpt = strchr( block, ':' );
		npt = strchr( block, ',' );
		if( !tpt || tpt > npt )
		{
			len = atoi( block );
			reg1->start = pos1;
			reg2->start = pos2;
			pos1 += len - 1;
			pos2 += len - 1;
			reg1->end = pos1;
			reg2->end = pos2;
//			printf(  "in loop reg1: %d-%d\n", reg1->start, reg1->end );
//			printf(  "in loop reg2: %d-%d\n", reg2->start, reg2->end );
			reg1++;
			reg2++;
		}
		else
		{
			sscanf( block, "%d:%d", &glen1, &glen2 );
			pos1 += glen1 + 1;
			pos2 += glen2 + 1;
		}
		block = npt;

	}
	reg1->start = reg1->end = reg2->start = reg2->end = -1;
	
	while( rpt1->start != -1 )
	{
//		printf(  "reg1: %d-%d\n", rpt1->start, rpt1->end );
//		printf(  "reg2: %d-%d\n", rpt2->start, rpt2->end );
		rpt1++;
		rpt2++;
	}
//	*apt1 = *apt2 = 0;
//	printf(  "aln1 = %s\n", aln1 );
//	printf(  "aln2 = %s\n", aln2 );
}


static void readlastresx_singleq( FILE *fp, int n1, int nameq, Lastresx **lastresx )
{
	char *gett;
	Aln *tmpaln;
	int prevnaln, naln, nreg;
#if 0
	int i, pstart, pend, end1, end2;
#endif
	int score, name1, start1, alnSize1, seqSize1;
	int        name2, start2, alnSize2, seqSize2;
	char strand1, strand2;
	int includeintoscore;
	gett = calloc( 10000, sizeof( char ) );

//	printf(  "seq2[0] = %s\n", seq2[0] );
//	printf(  "seq1[0] = %s\n", seq1[0] );

	while( 1 )
	{
		fgets( gett, 9999, fp );
		if( feof( fp ) ) break;
		if( gett[0] == '#' ) continue;
//		printf(  "gett = %s\n", gett );
		if( gett[strlen(gett)-1] != '\n' )
		{
			printf(  "Too long line?\n" );
			exit( 1 );
		}

		sscanf( gett, "%d %d %d %d %c %d %d %d %d %c %d", 
					&score, &name1, &start1, &alnSize1, &strand1, &seqSize1,
					        &name2, &start2, &alnSize2, &strand2, &seqSize2 );

		if( alg == 'R' && name2 <= name1 ) continue;
		if( name2 != nameq )
		{
			printf(  "BUG!!!\n" );
			exit( 1 );
		}

//		if( lastresx[name1][name2].score ) continue; // dame!!!!


		prevnaln = lastresx[name1][name2].naln;
#if 0
		for( i=0; i<prevnaln; i++ )
		{
			nreg = lastresx[name1][name2].aln[i].nreg;

			pstart = lastresx[name1][name2].aln[i].reg1[0].start + 0;
			pend   = lastresx[name1][name2].aln[i].reg1[nreg-1].end - 0;
			end1 = start1 + alnSize1;
//			printf(  "pstart = %d, pend = %d\n", pstart, pend );
			if( pstart <= start1 && start1 <= pend && pend - start1 > 1 ) break;
			if( pstart <= end1   && end1   <= pend && end1 - pstart > 1 ) break;

			pstart = lastresx[name1][name2].aln[i].reg2[0].start + 0;
			pend   = lastresx[name1][name2].aln[i].reg2[nreg-1].end - 0;
			end2 = start2 + alnSize2;
//			printf(  "pstart = %d, pend = %d\n", pstart, pend );
			if( pstart <= start2 && start2 <= pend && pend - start2 > 1 ) break;
			if( pstart <= end2   && end2   <= pend && end2 - pstart > 1 ) break;
		}
		includeintoscore = ( i == prevnaln );
#else
		if( prevnaln ) includeintoscore = 0;
		else includeintoscore = 1;
#endif
		if( !includeintoscore && !lastsubopt )
			continue;

		naln = prevnaln + 1;
		lastresx[name1][name2].naln = naln;
//		printf(  "OK! add this alignment to hat3, %d-%d, naln = %d->%d\n", name1, name2, prevnaln, naln );

		if( ( tmpaln = (Aln *)realloc( lastresx[name1][name2].aln, (naln) * sizeof( Aln ) ) ) == NULL ) // yoyu nashi
		{
			printf(  "Cannot reallocate lastresx[][].aln\n" );
			exit( 1 );
		}
		else
			lastresx[name1][name2].aln = tmpaln;

		nreg = countcomma( gett )/2 + 1;
		lastresx[name1][name2].aln[prevnaln].nreg = nreg;
//		lastresx[name1][name2].aln[naln].nreg = -1;
//		lastresx[name1][name2].aln[naln].reg1 = NULL;
//		lastresx[name1][name2].aln[naln].reg2 = NULL;
//		printf(  "name1=%d, name2=%d, nreg=%d, prevnaln=%d\n", name1, name2, nreg, prevnaln );

		if( ( lastresx[name1][name2].aln[prevnaln].reg1 = (Reg *)calloc( nreg+1, sizeof( Reg ) ) ) == NULL ) // yoyu nashi
		{
			printf(  "Cannot reallocate lastresx[][].reg2\n" );
			exit( 1 );
		}

		if( ( lastresx[name1][name2].aln[prevnaln].reg2 = (Reg *)calloc( nreg+1, sizeof( Reg ) ) ) == NULL ) // yoyu nashi
		{
			printf(  "Cannot reallocate lastresx[][].reg2\n" );
			exit( 1 );
		}

//		lastresx[name1][name2].aln[prevnaln].reg1[0].start = -1; // iranai?
//		lastresx[name1][name2].aln[prevnaln].reg2[0].start = -1; // iranai?
		block2reg( strrchr( gett, '\t' ), lastresx[name1][name2].aln[prevnaln].reg1, lastresx[name1][name2].aln[prevnaln].reg2, start1, start2 );

		if( includeintoscore )
		{
			if( lastresx[name1][name2].score ) score += penalty;
			lastresx[name1][name2].score += score;
		}

//		printf(  "score(%d,%d) = %d\n", name1, name2, lastresx[name1][name2].score );
	}
	free( gett );
}

#ifdef enablemultithread
#if 0
static void readlastresx_group( FILE *fp, Lastresx **lastresx )
{
	char *gett;
	Aln *tmpaln;
	int prevnaln, naln, nreg;
#if 0
	int i, pstart, pend, end1, end2;
#endif
	int score, name1, start1, alnSize1, seqSize1;
	int        name2, start2, alnSize2, seqSize2;
	char strand1, strand2;
	int includeintoscore;
	gett = calloc( 10000, sizeof( char ) );

//	printf(  "seq2[0] = %s\n", seq2[0] );
//	printf(  "seq1[0] = %s\n", seq1[0] );

	while( 1 )
	{
		fgets( gett, 9999, fp );
		if( feof( fp ) ) break;
		if( gett[0] == '#' ) continue;
//		printf(  "gett = %s\n", gett );
		if( gett[strlen(gett)-1] != '\n' )
		{
			printf(  "Too long line?\n" );
			exit( 1 );
		}

		sscanf( gett, "%d %d %d %d %c %d %d %d %d %c %d", 
					&score, &name1, &start1, &alnSize1, &strand1, &seqSize1,
					        &name2, &start2, &alnSize2, &strand2, &seqSize2 );

		if( alg == 'R' && name2 <= name1 ) continue;

//		if( lastresx[name1][name2].score ) continue; // dame!!!!

		prevnaln = lastresx[name1][name2].naln;
#if 0
		for( i=0; i<prevnaln; i++ )
		{
			nreg = lastresx[name1][name2].aln[i].nreg;

			pstart = lastresx[name1][name2].aln[i].reg1[0].start + 0;
			pend   = lastresx[name1][name2].aln[i].reg1[nreg-1].end - 0;
			end1 = start1 + alnSize1;
//			printf(  "pstart = %d, pend = %d\n", pstart, pend );
			if( pstart <= start1 && start1 <= pend && pend - start1 > 3 ) break;
			if( pstart <= end1   && end1   <= pend && end1 - pstart > 3 ) break;

			pstart = lastresx[name1][name2].aln[i].reg2[0].start + 0;
			pend   = lastresx[name1][name2].aln[i].reg2[nreg-1].end - 0;
			end2 = start2 + alnSize2;
//			printf(  "pstart = %d, pend = %d\n", pstart, pend );
			if( pstart <= start2 && start2 <= pend && pend - start2 > 3 ) break;
			if( pstart <= end2   && end2   <= pend && end2 - pstart > 3 ) break;
		}
		includeintoscore = ( i == prevnaln );
#else
		if( prevnaln ) includeintoscore = 0;
		else includeintoscore = 1;
#endif
		if( !includeintoscore && !lastsubopt )
			continue;

		naln = prevnaln + 1;
		lastresx[name1][name2].naln = naln;
//		printf(  "OK! add this alignment to hat3, %d-%d, naln = %d->%d\n", name1, name2, prevnaln, naln );

		if( ( tmpaln = (Aln *)realloc( lastresx[name1][name2].aln, (naln) * sizeof( Aln ) ) ) == NULL ) // yoyu nashi
		{
			printf(  "Cannot reallocate lastresx[][].aln\n" );
			exit( 1 );
		}
		else
			lastresx[name1][name2].aln = tmpaln;



		nreg = countcomma( gett )/2 + 1;
		lastresx[name1][name2].aln[prevnaln].nreg = nreg;
//		lastresx[name1][name2].aln[naln].nreg = -1;
//		lastresx[name1][name2].aln[naln].reg1 = NULL;
//		lastresx[name1][name2].aln[naln].reg2 = NULL;
//		printf(  "name1=%d, name2=%d, nreg=%d, prevnaln=%d\n", name1, name2, nreg, prevnaln );

		if( ( lastresx[name1][name2].aln[prevnaln].reg1 = (Reg *)calloc( nreg+1, sizeof( Reg ) ) ) == NULL ) // yoyu nashi
		{
			printf(  "Cannot reallocate lastresx[][].reg2\n" );
			exit( 1 );
		}

		if( ( lastresx[name1][name2].aln[prevnaln].reg2 = (Reg *)calloc( nreg+1, sizeof( Reg ) ) ) == NULL ) // yoyu nashi
		{
			printf(  "Cannot reallocate lastresx[][].reg2\n" );
			exit( 1 );
		}

//		lastresx[name1][name2].aln[prevnaln].reg1[0].start = -1; // iranai?
//		lastresx[name1][name2].aln[prevnaln].reg2[0].start = -1; // iranai?
		block2reg( strrchr( gett, '\t' ), lastresx[name1][name2].aln[prevnaln].reg1, lastresx[name1][name2].aln[prevnaln].reg2, start1, start2 );

		if( includeintoscore )
		{
			if( lastresx[name1][name2].score ) score += penalty;
			lastresx[name1][name2].score += score;
		}

//		printf(  "score(%d,%d) = %d\n", name1, name2, lastresx[name1][name2].score );
	}
	free( gett );
}
#endif
#endif

static void readlastresx( FILE *fp, int n1, int n2, Lastresx **lastresx, char **seq1, char **seq2 )
{
	char *gett;
	Aln *tmpaln;
	int prevnaln, naln, nreg;
#if 0
	int i, pstart, pend, end1, end2;
#endif
	int score, name1, start1, alnSize1, seqSize1;
	int        name2, start2, alnSize2, seqSize2;
	char strand1, strand2;
	int includeintoscore;
	gett = calloc( 10000, sizeof( char ) );

//	printf(  "seq2[0] = %s\n", seq2[0] );
//	printf(  "seq1[0] = %s\n", seq1[0] );

	while( 1 )
	{
		fgets( gett, 9999, fp );
		if( feof( fp ) ) break;
		if( gett[0] == '#' ) continue;
//		printf(  "gett = %s\n", gett );
		if( gett[strlen(gett)-1] != '\n' )
		{
			printf(  "Too long line?\n" );
			exit( 1 );
		}

		sscanf( gett, "%d %d %d %d %c %d %d %d %d %c %d", 
					&score, &name1, &start1, &alnSize1, &strand1, &seqSize1,
					        &name2, &start2, &alnSize2, &strand2, &seqSize2 );

		if( alg == 'R' && name2 <= name1 ) continue;

//		if( lastresx[name1][name2].score ) continue; // dame!!!!

		prevnaln = lastresx[name1][name2].naln;
#if 0
		for( i=0; i<prevnaln; i++ )
		{
			nreg = lastresx[name1][name2].aln[i].nreg;

			pstart = lastresx[name1][name2].aln[i].reg1[0].start + 0;
			pend   = lastresx[name1][name2].aln[i].reg1[nreg-1].end - 0;
			end1 = start1 + alnSize1;
//			printf(  "pstart = %d, pend = %d\n", pstart, pend );
			if( pstart <= start1 && start1 <= pend && pend - start1 > 3 ) break;
			if( pstart <= end1   && end1   <= pend && end1 - pstart > 3 ) break;

			pstart = lastresx[name1][name2].aln[i].reg2[0].start + 0;
			pend   = lastresx[name1][name2].aln[i].reg2[nreg-1].end - 0;
			end2 = start2 + alnSize2;
//			printf(  "pstart = %d, pend = %d\n", pstart, pend );
			if( pstart <= start2 && start2 <= pend && pend - start2 > 3 ) break;
			if( pstart <= end2   && end2   <= pend && end2 - pstart > 3 ) break;
		}
		includeintoscore = ( i == prevnaln );
#else
		if( prevnaln ) includeintoscore = 0;
		else includeintoscore = 1;
#endif
		if( !includeintoscore && !lastsubopt )
			continue;

		naln = prevnaln + 1;
		lastresx[name1][name2].naln = naln;
//		printf(  "OK! add this alignment to hat3, %d-%d, naln = %d->%d\n", name1, name2, prevnaln, naln );

		if( ( tmpaln = (Aln *)realloc( lastresx[name1][name2].aln, (naln) * sizeof( Aln ) ) ) == NULL ) // yoyu nashi
		{
			printf(  "Cannot reallocate lastresx[][].aln\n" );
			exit( 1 );
		}
		else
			lastresx[name1][name2].aln = tmpaln;



		nreg = countcomma( gett )/2 + 1;
		lastresx[name1][name2].aln[prevnaln].nreg = nreg;
//		lastresx[name1][name2].aln[naln].nreg = -1;
//		lastresx[name1][name2].aln[naln].reg1 = NULL;
//		lastresx[name1][name2].aln[naln].reg2 = NULL;
//		printf(  "name1=%d, name2=%d, nreg=%d, prevnaln=%d\n", name1, name2, nreg, prevnaln );

		if( ( lastresx[name1][name2].aln[prevnaln].reg1 = (Reg *)calloc( nreg+1, sizeof( Reg ) ) ) == NULL ) // yoyu nashi
		{
			printf(  "Cannot reallocate lastresx[][].reg2\n" );
			exit( 1 );
		}

		if( ( lastresx[name1][name2].aln[prevnaln].reg2 = (Reg *)calloc( nreg+1, sizeof( Reg ) ) ) == NULL ) // yoyu nashi
		{
			printf(  "Cannot reallocate lastresx[][].reg2\n" );
			exit( 1 );
		}

//		lastresx[name1][name2].aln[prevnaln].reg1[0].start = -1; // iranai?
//		lastresx[name1][name2].aln[prevnaln].reg2[0].start = -1; // iranai?
		block2reg( strrchr( gett, '\t' ), lastresx[name1][name2].aln[prevnaln].reg1, lastresx[name1][name2].aln[prevnaln].reg2, start1, start2 );

		if( includeintoscore )
		{
			if( lastresx[name1][name2].score ) score += penalty;
			lastresx[name1][name2].score += score;
		}

//		printf(  "score(%d,%d) = %d\n", name1, name2, lastresx[name1][name2].score );
	}
	free( gett );
}

#ifdef enablemultithread
#if 0
static void *lastcallthread_group( void *arg )
{
	lastcallthread_arg_t *targ = (lastcallthread_arg_t *)arg;
	int k, i;
	int nq = targ->nq;
	int nd = targ->nd;
#ifdef enablemultithread
	int thread_no = targ->thread_no;
	int *kshare = targ->kshare; 
#endif
	Lastresx **lastresx = targ->lastresx;
	char **dseq = targ->dseq;
	char **qseq = targ->qseq;
	char command[5000];
	FILE *lfp;
	int msize;
	int klim;
	int qstart, qend, shou, amari;
	char kd[1000];

	if( nthread )
	{
		shou = nq / nthread;
		amari = nq - shou * nthread;
		printf(  "shou: %d, amari: %d\n", shou, amari );

		qstart = thread_no * shou;
		if( thread_no - 1 < amari ) qstart += thread_no;
		else qstart += amari;

		qend = qstart + shou - 1;
		if( thread_no < amari ) qend += 1;
		printf(  "%d: %d-%d\n", thread_no, qstart, qend );
	}
	k = -1;
	while( 1 )
	{
		if( nthread )
		{
			if( qstart > qend ) break;
			if( k == thread_no ) break;
			printf(  "\n%d-%d / %d (thread %d)                    \n", qstart, qend, nq, thread_no );
			k = thread_no;
		}
		else
		{
			k++;
			if( k == nq ) break;
			printf(  "\r%d / %d                    \r", k, nq );
		}

		if( alg == 'R' ) // if 'r' -> calllast_fast
		{
			printf(  "Not supported\n" );
			exit( 1 );
		}
		else // 'r'
		{
			kd[0] = 0;
		}
		
		sprintf( command, "_q%d", k );
		lfp = fopen( command, "w" );
		if( !lfp )
		{
			printf(  "Cannot open %s", command );
			exit( 1 );
		}
		for( i=qstart; i<=qend; i++ )
			fprintf( lfp, ">%d\n%s\n", i, qseq[i] );
		fclose( lfp );
	
//		if( alg == 'R' ) msize = MAX(10,k+nq);
//			else msize = MAX(10,nd+nq);
		if( alg == 'R' ) msize = MAX(10,k*lastm);
			else msize = MAX(10,nd*lastm);

//		printf(  "Calling lastal from lastcallthread, msize = %d, k=%d\n", msize, k );
//		sprintf( command, "grep '>' _db%sd", kd );
//		system( command );
		sprintf( command, "%s/lastal -m %d -e %d -f 0 -s 1 -p _scoringmatrixforlast -a %d -b %d _db%sd _q%d > _lastres%d", whereispairalign, msize, laste, -penalty, -penalty_ex, kd, k, k );
		if( system( command ) ) exit( 1 );
	
		sprintf( command, "_lastres%d", k );
		lfp = fopen( command, "r" );
		if( !lfp )
		{
			printf(  "Cannot read _lastres%d", k );
			exit( 1 );
		}
//		readlastres( lfp, nd, nq, lastres, dseq, qseq );
//		printf(  "Reading lastres\n" );
		readlastresx_group( lfp, lastresx );
		fclose( lfp );
	}
	return( NULL );
}
#endif
#endif

static void *lastcallthread( void *arg )
{
	lastcallthread_arg_t *targ = (lastcallthread_arg_t *)arg;
	int k, i;
	int nq = targ->nq;
	int nd = targ->nd;
#ifdef enablemultithread
	int thread_no = targ->thread_no;
	int *kshare = targ->kshare; 
#endif
	Lastresx **lastresx = targ->lastresx;
	char **dseq = targ->dseq;
	char **qseq = targ->qseq;
	char command[5000];
	FILE *lfp;
	int msize;
	int klim;
	char kd[1000];

	k = -1;
	while( 1 )
	{

#ifdef enablemultithread
		if( nthread )
		{
			pthread_mutex_lock( targ->mutex );
			k = *kshare;
			if( k == nq )
			{
				pthread_mutex_unlock( targ->mutex );
				break;
			}
			printf(  "\r%d / %d (thread %d)                    \r", k, nq, thread_no );
			++(*kshare);
			pthread_mutex_unlock( targ->mutex );
		}
		else
#endif
		{
			k++;
			if( k == nq ) break;
			printf(  "\r%d / %d                    \r", k, nq );
		}

		if( alg == 'R' ) // if 'r' -> calllast_fast
		{
			klim = MIN( k, njob-nadd );
//			klim = k; // dochira demo yoi
			if( klim == k ) 
			{
				sprintf( command, "_db%dd", k );
				lfp = fopen( command, "w" );
				if( !lfp )
				{
					printf(  "Cannot open _db." );
					exit( 1 );
				}
				for( i=0; i<klim; i++ ) fprintf( lfp, ">%d\n%s\n", i, dseq[i] );
				fclose( lfp );

//				sprintf( command, "md5sum _db%dd > /dev/tty", k );
//				system( command );

				if( dorp == 'd' ) 
					sprintf( command, "%s/lastdb _db%dd _db%dd", whereispairalign, k, k );
				else
					sprintf( command, "%s/lastdb -p _db%dd _db%dd", whereispairalign, k, k );
				system( command );
				sprintf( kd, "%d", k );
			}
			else // calllast_fast de tsukutta nowo riyou
			{
				kd[0] = 0;
//				printf(  "klim=%d, njob=%d, nadd=%d, skip!\n", klim, njob, nadd );
			}
		}
		else // 'r'
		{
			kd[0] = 0;
		}
		
		sprintf( command, "_q%d", k );
		lfp = fopen( command, "w" );
		if( !lfp )
		{
			printf(  "Cannot open %s", command );
			exit( 1 );
		}
		fprintf( lfp, ">%d\n%s\n", k, qseq[k] );
		fclose( lfp );
	
//		if( alg == 'R' ) msize = MAX(10,k+nq);
//			else msize = MAX(10,nd+nq);
		if( alg == 'R' ) msize = MAX(10,k*lastm);
			else msize = MAX(10,nd*lastm);

//		printf(  "Calling lastal from lastcallthread, msize = %d, k=%d\n", msize, k );
//		sprintf( command, "grep '>' _db%sd", kd );
//		system( command );
		sprintf( command, "%s/lastal -m %d -e %d -f 0 -s 1 -p _scoringmatrixforlast -a %d -b %d _db%sd _q%d > _lastres%d", whereispairalign, msize, laste, -penalty, -penalty_ex, kd, k, k );
		if( system( command ) ) exit( 1 );
	
		sprintf( command, "_lastres%d", k );
		lfp = fopen( command, "r" );
		if( !lfp )
		{
			printf(  "Cannot read _lastres%d", k );
			exit( 1 );
		}
//		readlastres( lfp, nd, nq, lastres, dseq, qseq );
//		printf(  "Reading lastres\n" );
		readlastresx_singleq( lfp, nd, k, lastresx );
		fclose( lfp );
	}
	return( NULL );
}


static void calllast_fast( int nd, char **dseq, int nq, char **qseq, Lastresx **lastresx )
{
	int i, j;
	FILE *lfp;
	char command[1000];

	lfp = fopen( "_scoringmatrixforlast", "w" );
	if( !lfp )
	{
		printf(  "Cannot open _scoringmatrixforlast" );
		exit( 1 );
	}
	if( dorp == 'd' ) 
	{
		fprintf( lfp, "      " );
		for( j=0; j<4; j++ ) fprintf( lfp, " %c ", amino[j] );
		fprintf( lfp, "\n" );
		for( i=0; i<4; i++ )
		{
			fprintf( lfp, "%c ", amino[i] );
			for( j=0; j<4; j++ ) fprintf( lfp, " %d ", n_dis[i][j] );
			fprintf( lfp, "\n" );
		}
	}
	else
	{
		fprintf( lfp, "      " );
		for( j=0; j<20; j++ ) fprintf( lfp, " %c ", amino[j] );
		fprintf( lfp, "\n" );
		for( i=0; i<20; i++ )
		{
			fprintf( lfp, "%c ", amino[i] );
			for( j=0; j<20; j++ ) fprintf( lfp, " %d ", n_dis[i][j] );
			fprintf( lfp, "\n" );
		}
	}
	fclose( lfp );

//	if( alg == 'r' ) // if 'R' -> lastcallthread, kokonoha nadd>0 no toki nomi shiyou
	{
		sprintf( command, "_dbd" );
		lfp = fopen( command, "w" );
		if( !lfp )
		{
			printf(  "Cannot open _dbd" );
			exit( 1 );
		}
		if( alg == 'R' )
			j = njob-nadd;
		else
			j = nd;
		for( i=0; i<j; i++ ) fprintf( lfp, ">%d\n%s\n", i, dseq[i] );

		fclose( lfp );
		if( dorp == 'd' ) 
			sprintf( command, "%s/lastdb _dbd _dbd", whereispairalign );
		else
			sprintf( command, "%s/lastdb -p _dbd _dbd", whereispairalign );
		system( command );
	}

#ifdef enablemultithread
	if( nthread )
	{
		pthread_t *handle;
		pthread_mutex_t mutex;
		lastcallthread_arg_t *targ;
		int *ksharept;
		targ = (lastcallthread_arg_t *)calloc( nthread, sizeof( lastcallthread_arg_t ) );
		handle = calloc( nthread, sizeof( pthread_t ) );
		ksharept = calloc( 1, sizeof(int) );
		*ksharept = 0;
		pthread_mutex_init( &mutex, NULL );
		for( i=0; i<nthread; i++ )
		{
			targ[i].thread_no = i;
			targ[i].kshare = ksharept;
			targ[i].nq = nq;
			targ[i].nd = nd;
			targ[i].dseq = dseq;
			targ[i].qseq = qseq;
			targ[i].lastresx = lastresx;
			targ[i].mutex = &mutex;
			pthread_create( handle+i, NULL, lastcallthread, (void *)(targ+i) );
		}

		for( i=0; i<nthread; i++ )
		{
			pthread_join( handle[i], NULL );
		}
		pthread_mutex_destroy( &mutex );
		free( handle );
		free( targ );
		free( ksharept );
	}
	else
#endif
	{
		lastcallthread_arg_t *targ;
		targ = (lastcallthread_arg_t *)calloc( 1, sizeof( lastcallthread_arg_t ) );
		targ[0].nq = nq;
		targ[0].nd = nd;
		targ[0].dseq = dseq;
		targ[0].qseq = qseq;
		targ[0].lastresx = lastresx;
		lastcallthread( targ );
		free( targ );
	}

}

static void calllast_once( int nd, char **dseq, int nq, char **qseq, Lastresx **lastresx )
{
	int i, j;
	char command[5000];
	FILE *lfp;
	int msize;
	int res;

	printf(  "nq=%d\n", nq );

	lfp = fopen( "_db", "w" );
	if( !lfp )
	{
		printf(  "Cannot open _db" );
		exit( 1 );
	}
	for( i=0; i<nd; i++ ) fprintf( lfp, ">%d\n%s\n", i, dseq[i] );
	fclose( lfp );

	if( dorp == 'd' ) 
	{
		sprintf( command, "%s/lastdb _db _db", whereispairalign );
		system( command );
		lfp = fopen( "_scoringmatrixforlast", "w" );
		if( !lfp )
		{
			printf(  "Cannot open _scoringmatrixforlast" );
			exit( 1 );
		}
		fprintf( lfp, "      " );
		for( j=0; j<4; j++ ) fprintf( lfp, " %c ", amino[j] );
		fprintf( lfp, "\n" );
		for( i=0; i<4; i++ )
		{
			fprintf( lfp, "%c ", amino[i] );
			for( j=0; j<4; j++ ) fprintf( lfp, " %d ", n_dis[i][j] );
			fprintf( lfp, "\n" );
		}
		fclose( lfp );
#if 0
		sprintf( command, "lastex -s 2 -a %d -b %d -p _scoringmatrixforlast -E 10000 _db.prj _db.prj > _lastex", -penalty, -penalty_ex );
		system( command );
		lfp = fopen( "_lastex", "r" );
		fgets( command, 4999, lfp );
		fgets( command, 4999, lfp );
		fgets( command, 4999, lfp );
		fgets( command, 4999, lfp );
		laste = atoi( command );
		fclose( lfp );
		printf(  "laste = %d\n", laste );
		sleep( 10 );
#else
//		laste = 5000;
#endif
	}
	else
	{
		sprintf( command, "%s/lastdb -p _db _db", whereispairalign );
		system( command );
		lfp = fopen( "_scoringmatrixforlast", "w" );
		if( !lfp )
		{
			printf(  "Cannot open _scoringmatrixforlast" );
			exit( 1 );
		}
		fprintf( lfp, "      " );
		for( j=0; j<20; j++ ) fprintf( lfp, " %c ", amino[j] );
		fprintf( lfp, "\n" );
		for( i=0; i<20; i++ )
		{
			fprintf( lfp, "%c ", amino[i] );
			for( j=0; j<20; j++ ) fprintf( lfp, " %d ", n_dis[i][j] );
			fprintf( lfp, "\n" );
		}
		fclose( lfp );
//		printf(  "Not written yet\n" );
	}

	lfp = fopen( "_q", "w" );
	if( !lfp )
	{
		printf(  "Cannot open _q" );
		exit( 1 );
	}
	for( i=0; i<nq; i++ )
	{
		fprintf( lfp, ">%d\n%s\n", i, qseq[i] );
	}
	fclose( lfp );

	msize = MAX(10,nd*lastm);

//	printf(  "Calling lastal from calllast_once, msize=%d\n", msize );
	sprintf( command, "%s/lastal -v -m %d -e %d -f 0 -s 1 -p _scoringmatrixforlast -a %d -b %d _db _q > _lastres", whereispairalign, msize, laste, -penalty, -penalty_ex );
//	sprintf( command, "lastal -v -m %d -e %d -f 0 -s 1 -p _scoringmatrixforlast -a %d -b %d _db _q > _lastres", 1, laste, -penalty, -penalty_ex );
//	sprintf( command, "lastal -v -e 40 -f 0 -s 1 -p _scoringmatrixforlast -a %d -b %d _db _q > _lastres", -penalty, -penalty_ex );
	res = system( command );
	if( res )
	{
		printf(  "LAST aborted\n" );
		exit( 1 );
	}

	lfp = fopen( "_lastres", "r" );
	if( !lfp )
	{
		printf(  "Cannot read _lastres" );
		exit( 1 );
	}
//	readlastres( lfp, nd, nq, lastres, dseq, qseq );
	printf(  "Reading lastres\n" );
	readlastresx( lfp, nd, nq, lastresx, dseq, qseq );
	fclose( lfp );
}

static void callfoldalign( int nseq, char **mseq )
{
	FILE *fp;
	int i;
	int res;
	static char com[10000];

	for( i=0; i<nseq; i++ )
		t2u( mseq[i] );

	fp = fopen( "_foldalignin", "w" );
	if( !fp )
	{
		printf(  "Cannot open _foldalignin\n" );
		exit( 1 );
	}
	for( i=0; i<nseq; i++ )
	{
		fprintf( fp, ">%d\n", i+1 );
		fprintf( fp, "%s\n", mseq[i] );
	}
	fclose( fp );

	sprintf( com, "env PATH=%s  foldalign210 %s _foldalignin > _foldalignout ", whereispairalign, foldalignopt );
	res = system( com );
	if( res )
	{
		printf(  "Error in foldalign\n" );
		exit( 1 );
	}

}

static void calllara( int nseq, char **mseq, char *laraarg )
{
	FILE *fp;
	int i;
	int res;
	static char com[10000];

//	for( i=0; i<nseq; i++ )

	fp = fopen( "_larain", "w" );
	if( !fp )
	{
		printf(  "Cannot open _larain\n" );
		exit( 1 );
	}
	for( i=0; i<nseq; i++ )
	{
		fprintf( fp, ">%d\n", i+1 );
		fprintf( fp, "%s\n", mseq[i] );
	}
	fclose( fp );


//	printf(  "calling LaRA\n" );
	sprintf( com, "env PATH=%s:/bin:/usr/bin mafft_lara -i _larain -w _laraout -o _lara.params %s", whereispairalign, laraarg );
	res = system( com );
	if( res )
	{
		printf(  "Error in lara\n" );
		exit( 1 );
	}
}

static float recalllara( char **mseq1, char **mseq2, int alloclen )
{
	static FILE *fp = NULL;
	static char *ungap1;
	static char *ungap2;
	static char *ori1;
	static char *ori2;
//	int res;
	static char com[10000];
	float value;


	if( fp == NULL )
	{
		fp = fopen( "_laraout", "r" );
		if( fp == NULL )
		{
			printf(  "Cannot open _laraout\n" );
			exit( 1 );
		}
		ungap1 = AllocateCharVec( alloclen );
		ungap2 = AllocateCharVec( alloclen );
		ori1 = AllocateCharVec( alloclen );
		ori2 = AllocateCharVec( alloclen );
	}


	strcpy( ori1, *mseq1 );
	strcpy( ori2, *mseq2 );

	fgets( com, 999, fp );
	myfgets( com, 9999, fp );
	strcpy( *mseq1, com );
	myfgets( com, 9999, fp );
	strcpy( *mseq2, com );

	gappick0( ungap1, *mseq1 );
	gappick0( ungap2, *mseq2 );
	t2u( ungap1 );
	t2u( ungap2 );
	t2u( ori1 );
	t2u( ori2 );

	if( strcmp( ungap1, ori1 ) || strcmp( ungap2, ori2 ) )
	{
		printf(  "SEQUENCE CHANGED!!\n" );
		printf(  "*mseq1  = %s\n", *mseq1 );
		printf(  "ungap1  = %s\n", ungap1 );
		printf(  "ori1    = %s\n", ori1 );
		printf(  "*mseq2  = %s\n", *mseq2 );
		printf(  "ungap2  = %s\n", ungap2 );
		printf(  "ori2    = %s\n", ori2 );
		exit( 1 );
	}

	value = (float)naivepairscore11( *mseq1, *mseq2, penalty );

//	fclose( fp ); // saigo dake yatta houga yoi.

	return( value );
}


static float calldafs_giving_bpp( char **mseq1, char **mseq2, char **bpp1, char **bpp2, int alloclen, int i, int j )
{
	FILE *fp;
	int res;
	char *com;
	float value;
	char *dirname;


	dirname = calloc( 100, sizeof( char ) );
	com = calloc( 1000, sizeof( char ) );
	sprintf( dirname, "_%d-%d", i, j );
	sprintf( com, "rm -rf %s", dirname );
	system( com );
	sprintf( com, "mkdir %s", dirname );
	system( com );


	sprintf( com, "%s/_bpporg", dirname );
	fp = fopen( com, "w" );
	if( !fp )
	{
		printf(  "Cannot write to %s/_bpporg\n", dirname );
		exit( 1 );
	}
	fprintf( fp, ">a\n" );
	while( *bpp1 )
		fprintf( fp, "%s", *bpp1++ );

	fprintf( fp, ">b\n" );
	while( *bpp2 )
		fprintf( fp, "%s", *bpp2++ );
	fclose( fp );

	sprintf( com, "tr -d '\\r' < %s/_bpporg > %s/_bpp", dirname, dirname );
	system( com ); // for cygwin, wakaran

	t2u( *mseq1 );
	t2u( *mseq2 );

	sprintf( com, "%s/_dafsinorg", dirname );
	fp = fopen( com, "w" );
	if( !fp )
	{
		printf(  "Cannot open %s/_dafsinorg\n", dirname );
		exit( 1 );
	}
	fprintf( fp, ">1\n" );
//	fprintf( fp, "%s\n", *mseq1 );
	write1seq( fp, *mseq1 );
	fprintf( fp, ">2\n" );
//	fprintf( fp, "%s\n", *mseq2 );
	write1seq( fp, *mseq2 );
	fclose( fp );

	sprintf( com, "tr -d '\\r' < %s/_dafsinorg > %s/_dafsin", dirname, dirname );
	system( com ); // for cygwin, wakaran

	sprintf( com, "_dafssh%s", dirname );
	fp = fopen( com, "w" );
	fprintf( fp, "cd %s\n", dirname );
	fprintf( fp, "%s/dafs --mafft-in _bpp _dafsin > _dafsout 2>_dum\n", whereispairalign );
	fprintf( fp, "exit $tatus\n" );
	fclose( fp );

	sprintf( com, "tr -d '\\r' < _dafssh%s > _dafssh%s.unix", dirname, dirname );
	system( com ); // for cygwin, wakaran

	sprintf( com, "sh _dafssh%s.unix 2>_dum%s", dirname, dirname );
	res = system( com );
	if( res )
	{
		printf(  "Error in dafs\n" );
		exit( 1 );
	}

	sprintf( com, "%s/_dafsout", dirname );

	fp = fopen( com, "r" );
	if( !fp )
	{
		printf(  "Cannot open %s/_dafsout\n", dirname );
		exit( 1 );
	}

	myfgets( com, 999, fp ); // nagai kanousei ga arunode
	fgets( com, 999, fp );
	myfgets( com, 999, fp ); // nagai kanousei ga arunode
	fgets( com, 999, fp );
	load1SeqWithoutName_new( fp, *mseq1 );
	fgets( com, 999, fp );
	load1SeqWithoutName_new( fp, *mseq2 );

	fclose( fp );

//	printf(  "*mseq1 = %s\n", *mseq1 );
//	printf(  "*mseq2 = %s\n", *mseq2 );

	value = (float)naivepairscore11( *mseq1, *mseq2, penalty );

#if 0
	sprintf( com, "rm -rf %s > /dev/null 2>&1", dirname );
	if( system( com ) )
	{
		printf(  "retrying to rmdir\n" );
		usleep( 2000 );
		system( com );
	}
#endif

	free( dirname );
	free( com );


	return( value );
}

static float callmxscarna_giving_bpp( char **mseq1, char **mseq2, char **bpp1, char **bpp2, int alloclen, int i, int j )
{
	FILE *fp;
	int res;
	char *com;
	float value;
	char *dirname;


	dirname = calloc( 100, sizeof( char ) );
	com = calloc( 1000, sizeof( char ) );
	sprintf( dirname, "_%d-%d", i, j );
	sprintf( com, "rm -rf %s", dirname );
	system( com );
	sprintf( com, "mkdir %s", dirname );
	system( com );


	sprintf( com, "%s/_bpporg", dirname );
	fp = fopen( com, "w" );
	if( !fp )
	{
		printf(  "Cannot write to %s/_bpporg\n", dirname );
		exit( 1 );
	}
	fprintf( fp, ">a\n" );
	while( *bpp1 )
		fprintf( fp, "%s", *bpp1++ );

	fprintf( fp, ">b\n" );
	while( *bpp2 )
		fprintf( fp, "%s", *bpp2++ );
	fclose( fp );

	sprintf( com, "tr -d '\\r' < %s/_bpporg > %s/_bpp", dirname, dirname );
	system( com ); // for cygwin, wakaran

	t2u( *mseq1 );
	t2u( *mseq2 );

	sprintf( com, "%s/_mxscarnainorg", dirname );
	fp = fopen( com, "w" );
	if( !fp )
	{
		printf(  "Cannot open %s/_mxscarnainorg\n", dirname );
		exit( 1 );
	}
	fprintf( fp, ">1\n" );
//	fprintf( fp, "%s\n", *mseq1 );
	write1seq( fp, *mseq1 );
	fprintf( fp, ">2\n" );
//	fprintf( fp, "%s\n", *mseq2 );
	write1seq( fp, *mseq2 );
	fclose( fp );

	sprintf( com, "tr -d '\\r' < %s/_mxscarnainorg > %s/_mxscarnain", dirname, dirname );
	system( com ); // for cygwin, wakaran

#if 0
	sprintf( com, "cd %s; %s/mxscarnamod -readbpp _mxscarnain > _mxscarnaout 2>_dum", dirname, whereispairalign );
#else
	sprintf( com, "_mxscarnash%s", dirname );
	fp = fopen( com, "w" );
	fprintf( fp, "cd %s\n", dirname );
	fprintf( fp, "%s/mxscarnamod -readbpp _mxscarnain > _mxscarnaout 2>_dum\n", whereispairalign );
	fprintf( fp, "exit $tatus\n" );
	fclose( fp );
//sleep( 10000 );

	sprintf( com, "tr -d '\\r' < _mxscarnash%s > _mxscarnash%s.unix", dirname, dirname );
	system( com ); // for cygwin, wakaran

	sprintf( com, "sh _mxscarnash%s.unix 2>_dum%s", dirname, dirname );
#endif
	res = system( com );
	if( res )
	{
		printf(  "Error in mxscarna\n" );
		exit( 1 );
	}

	sprintf( com, "%s/_mxscarnaout", dirname );

	fp = fopen( com, "r" );
	if( !fp )
	{
		printf(  "Cannot open %s/_mxscarnaout\n", dirname );
		exit( 1 );
	}

	fgets( com, 999, fp );
	load1SeqWithoutName_new( fp, *mseq1 );
	fgets( com, 999, fp );
	load1SeqWithoutName_new( fp, *mseq2 );

	fclose( fp );

//	printf(  "*mseq1 = %s\n", *mseq1 );
//	printf(  "*mseq2 = %s\n", *mseq2 );

	value = (float)naivepairscore11( *mseq1, *mseq2, penalty );

#if 0
	sprintf( com, "rm -rf %s > /dev/null 2>&1", dirname );
	if( system( com ) )
	{
		printf(  "retrying to rmdir\n" );
		usleep( 2000 );
		system( com );
	}
#endif

	free( dirname );
	free( com );


	return( value );
}

static void readhat4( FILE *fp, char ***bpp )
{
	char oneline[1000];
	int bppsize;
	int onechar;
//	double prob;
//	int posi, posj;

	bppsize = 0;
//	printf(  "reading hat4\n" );
	onechar = getc(fp);
//	printf(  "onechar = %c\n", onechar );
	if( onechar != '>' )
	{
		printf(  "Format error\n" );
		exit( 1 );
	}
	ungetc( onechar, fp );
	fgets( oneline, 999, fp );
	while( 1 )
	{
		onechar = getc(fp);
		ungetc( onechar, fp );
		if( onechar == '>' || onechar == EOF )
		{
//			printf(  "Next\n" );
			*bpp = realloc( *bpp, (bppsize+2) * sizeof( char * ) );
			(*bpp)[bppsize] = NULL;
			break;
		}
		fgets( oneline, 999, fp );
//		printf(  "oneline=%s\n", oneline );
//		sscanf( oneline, "%d %d %f", &posi, &posj, &prob );
//		printf(  "%d %d -> %f\n", posi, posj, prob );
		*bpp = realloc( *bpp, (bppsize+2) * sizeof( char * ) );
		(*bpp)[bppsize] = calloc( 100, sizeof( char ) );
		strcpy( (*bpp)[bppsize], oneline );
		bppsize++;
	}
}

static void preparebpp( int nseq, char ***bpp )
{
	FILE *fp;
	int i;

	fp = fopen( "hat4", "r" );
	if( !fp )
	{
		printf(  "Cannot open hat4\n" );
		exit( 1 );
	}
	for( i=0; i<nseq; i++ )
		readhat4( fp, bpp+i );
	fclose( fp );
}

void arguments( int argc, char *argv[] )
{
    int c;

	nthread = 1;
	laste = 5000;
	lastm = 3;
	nadd = 0;
	lastsubopt = 0;
	lastonce = 0;
	foldalignopt[0] = 0;
	laraparams = NULL;
	inputfile = NULL;
	fftkeika = 0;
	pslocal = -1000.0;
	constraint = 0;
	nblosum = 62;
	fmodel = 0;
	calledByXced = 0;
	devide = 0;
	use_fft = 0;
	fftscore = 1;
	fftRepeatStop = 0;
	fftNoAnchStop = 0;
    weight = 3;
    utree = 1;
	tbutree = 1;
    refine = 0;
    check = 1;
    cut = 0.0;
    disp = 0;
    outgap = 1;
    alg = 'A';
    mix = 0;
	tbitr = 0;
	scmtd = 5;
	tbweight = 0;
	tbrweight = 3;
	checkC = 0;
	treemethod = 'x';
	contin = 0;
	scoremtx = 1;
	kobetsubunkatsu = 0;
	divpairscore = 0;
	stdout_align = 0;
	stdout_dist = 0;
	store_dist = 1;
	store_localhom = 1;
	dorp = NOTSPECIFIED;
	ppenalty = NOTSPECIFIED;
	ppenalty_OP = NOTSPECIFIED;
	ppenalty_ex = NOTSPECIFIED;
	ppenalty_EX = NOTSPECIFIED;
	poffset = NOTSPECIFIED;
	kimuraR = NOTSPECIFIED;
	pamN = NOTSPECIFIED;
	geta2 = GETA2;
	fftWinSize = NOTSPECIFIED;
	fftThreshold = NOTSPECIFIED;
	RNAppenalty = NOTSPECIFIED;
	RNApthr = NOTSPECIFIED;

    while( --argc > 0 && (*++argv)[0] == '-' )
	{
   printf ("out!!\n");
        while ( ( c = *++argv[0] ) )
		{
   printf ("in!!\n");
            switch( c )
            {
				case 'i':
					inputfile = *++argv;
					printf(  "inputfile = %s\n", inputfile );
					--argc;
					goto nextoption;
				case 'f':
					ppenalty = (int)( atof( *++argv ) * 1000 - 0.5 );
					--argc;
					goto nextoption;
				case 'g':
					ppenalty_ex = (int)( atof( *++argv ) * 1000 - 0.5 );
					--argc;
					goto nextoption;
				case 'O':
					ppenalty_OP = (int)( atof( *++argv ) * 1000 - 0.5 );
					--argc;
					goto nextoption;
				case 'E':
					ppenalty_EX = (int)( atof( *++argv ) * 1000 - 0.5 );
					--argc;
					goto nextoption;
				case 'h':
					poffset = (int)( atof( *++argv ) * 1000 - 0.5 );
					--argc;
					goto nextoption;
				case 'k':
					kimuraR = myatoi( *++argv );
//					printf(  "kimuraR = %d\n", kimuraR );
					--argc;
					goto nextoption;
				case 'b':
					nblosum = myatoi( *++argv );
					scoremtx = 1;
//					printf(  "blosum %d\n", nblosum );
					--argc;
					goto nextoption;
				case 'j':
					pamN = myatoi( *++argv );
					scoremtx = 0;
					TMorJTT = JTT;
					printf(  "jtt %d\n", pamN );
					--argc;
					goto nextoption;
				case 'm':
					pamN = myatoi( *++argv );
					scoremtx = 0;
					TMorJTT = TM;
					printf(  "TM %d\n", pamN );
					--argc;
					goto nextoption;
#if 0
				case 'l':
					ppslocal = (int)( atof( *++argv ) * 1000 + 0.5 );
					pslocal = (int)( 600.0 / 1000.0 * ppslocal + 0.5);
//					printf(  "ppslocal = %d\n", ppslocal );
//					printf(  "pslocal = %d\n", pslocal );
					--argc;
					goto nextoption;
#else
				case 'l':
					if( atof( *++argv ) < 0.00001 ) store_localhom = 0;
					--argc;
					goto nextoption;
#endif
				case 'd':
					whereispairalign = *++argv;
					printf(  "whereispairalign = %s\n", whereispairalign );
					--argc; 
					goto nextoption;
				case 'p':
					laraparams = *++argv;
					printf(  "laraparams = %s\n", laraparams );
					--argc; 
					goto nextoption;
				case 'C':
					nthread = myatoi( *++argv );
					printf(  "nthread = %d\n", nthread );
					--argc; 
					goto nextoption;
				case 'I':
					nadd = myatoi( *++argv );
					printf(  "nadd = %d\n", nadd );
					--argc;
					goto nextoption;
				case 'w':
					lastm = myatoi( *++argv );
					printf(  "lastm = %d\n", lastm );
					--argc;
					goto nextoption;
				case 'e':
					laste = myatoi( *++argv );
					printf(  "laste = %d\n", laste );
					--argc;
					goto nextoption;
				case 'K': // Hontou ha iranai. disttbfast.c, tbfast.c to awaserutame.
					break;
				case 'c':
					stdout_dist = 1;
					break;
				case 'n':
					stdout_align = 1;
					break;
				case 'x':
					store_localhom = 0;
					store_dist = 0;
					break;
#if 1
				case 'a':
					fmodel = 1;
					break;
#endif
#if 0
				case 'r':
					fmodel = -1;
					break;
#endif
				case 'D':
					dorp = 'd';
					break;
				case 'P':
					dorp = 'p';
					break;
#if 0
				case 'e':
					fftscore = 0;
					break;
				case 'O':
					fftNoAnchStop = 1;
					break;
#endif
#if 0
				case 'Q':
					calledByXced = 1;
					break;
				case 'x':
					disp = 1;
					break;
				case 'a':
					alg = 'a';
					break;
				case 'S':
					alg = 'S';
					break;
#endif
				case 'Q':
					lastonce = 1;
					break;
				case 'S':
					lastsubopt = 1;
					break;
				case 't':
					alg = 't';
					store_localhom = 0;
					break;
				case 'L':
					alg = 'L';
					break;
				case 'Y':
					alg = 'Y'; // nadd>0 no toki nomi. moto no hairetsu to atarashii hairetsuno alignmnt -> L;
					break;
				case 's':
					alg = 's';
					break;
				case 'G':
					alg = 'G';
					break;
				case 'B':
					alg = 'B';
					break;
				case 'T':
					alg = 'T';
					break;
				case 'H':
					alg = 'H';
					break;
				case 'M':
					alg = 'M';
					break;
				case 'R':
					alg = 'R';
					break;
				case 'r':
					alg = 'r'; // nadd>0 no toki nomi. moto no hairetsu to atarashii hairetsuno alignmnt -> R, last
					break;
				case 'N':
					alg = 'N';
					break;
				case 'A':
					alg = 'A';
					break;
				case 'V':
					alg = 'V';
					break;
				case 'F':
					use_fft = 1;
					break;
				case 'v':
					tbrweight = 3;
					break;
				case 'y':
					divpairscore = 1;
					break;
/* Modified 01/08/27, default: user tree */
				case 'J':
					tbutree = 0;
					break;
/* modification end. */
				case 'o':
//					foldalignopt = *++argv;
					strcat( foldalignopt, " " );
					strcat( foldalignopt, *++argv );
					printf(  "foldalignopt = %s\n", foldalignopt );
					--argc; 
					goto nextoption;
#if 0
				case 'z':
					fftThreshold = myatoi( *++argv );
					--argc; 
					goto nextoption;
				case 'w':
					fftWinSize = myatoi( *++argv );
					--argc;
					goto nextoption;
				case 'Z':
					checkC = 1;
					break;
#endif
                default:
                    printf(  "illegal option %c\n", c );
                    argc = 0;
                    break;
            }
		}
		nextoption:
			;
	}
    if( argc == 1 )
    {
        cut = atof( (*argv) );
        argc--;
    }
    if( argc != 0 ) 
    {
        printf(  "options: Check source file !\n" );
        exit( 1 );
    }
	if( tbitr == 1 && outgap == 0 )
	{
		printf(  "conflicting options : o, m or u\n" );
		exit( 1 );
	}
}

int countamino( char *s, int end )
{
	int val = 0;
	while( end-- )
		if( *s++ != '-' ) val++;
	return( val );
}

#if enablemultithread
static void *athread( void *arg ) // alg='R', alg='r' -> tsukawarenai.
{
	thread_arg_t *targ = (thread_arg_t *)arg;
	int i, ilim, j, jst;
	int off1, off2, dum1, dum2, thereisx;
	int intdum;
	double bunbo;
	float pscore = 0.0; // by D.Mathog
	double *effarr1;
	double *effarr2;
	char **mseq1, **mseq2, **distseq1, **distseq2, **dumseq1, **dumseq2;
	char **aseq;

// thread_arg
	int thread_no = targ->thread_no;
	int njob = targ->njob;
	Jobtable *jobpospt = targ->jobpospt;
	char **name = targ->name;
	char **seq = targ->seq;
	char **dseq = targ->dseq;
	int *thereisxineachseq = targ->thereisxineachseq;
	LocalHom **localhomtable = targ->localhomtable;
	double **distancemtx = targ->distancemtx;
	double *selfscore = targ->selfscore;
	char ***bpp = targ->bpp;
	Lastresx **lastresx = targ->lastresx;
	int alloclen = targ->alloclen;

//	printf(  "thread %d start!\n", thread_no );

	effarr1 = AllocateDoubleVec( 1 );
	effarr2 = AllocateDoubleVec( 1 );
	mseq1 = AllocateCharMtx( njob, 0 );
	mseq2 = AllocateCharMtx( njob, 0 );
	if( alg == 'N' )
	{
		dumseq1 = AllocateCharMtx( 1, alloclen+10 );
		dumseq2 = AllocateCharMtx( 1, alloclen+10 );
	}
	distseq1 = AllocateCharMtx( 1, 0 );
	distseq2 = AllocateCharMtx( 1, 0 );
	aseq = AllocateCharMtx( 2, alloclen+10 );

	if( alg == 'Y' || alg == 'r' ) ilim = njob - nadd;
	else ilim = njob - 1;

	while( 1 )
	{
		pthread_mutex_lock( targ->mutex_counter );
		j = jobpospt->j;
		i = jobpospt->i;
		j++;
		if( j == njob )
		{
			i++;

			if( alg == 'Y' || alg == 'r' ) jst = njob - nadd;
			else jst = i + 1;
			j = jst; 

			if( i == ilim )
			{
//				printf(  "thread %d end!\n", thread_no );
				pthread_mutex_unlock( targ->mutex_counter );

				if( commonIP ) FreeIntMtx( commonIP );
				commonIP = NULL;
				if( commonJP ) FreeIntMtx( commonJP );
				commonJP = NULL;
				Falign( NULL, NULL, NULL, NULL, 0, 0, 0, NULL, NULL, 0, NULL );
				G__align11( NULL, NULL, 0, 0, 0 ); // 20130603
				G__align11_noalign( NULL, 0, 0, NULL, NULL, 0 );
				L__align11( NULL, NULL, 0, NULL, NULL );
				L__align11_noalign( NULL, NULL );
				genL__align11( NULL, NULL, 0, NULL, NULL );
				free( effarr1 );
				free( effarr2 );
				free( mseq1 );
				free( mseq2 );
				if( alg == 'N' )
				{
					FreeCharMtx( dumseq1 );
					FreeCharMtx( dumseq2 );
				}
				free( distseq1 );
				free( distseq2 );
				FreeCharMtx( aseq  );
				return( NULL );
			}
		}
		jobpospt->j = j;
		jobpospt->i = i;
		pthread_mutex_unlock( targ->mutex_counter );


		if( j == i+1 || j % 100 == 0 ) 
		{
			printf(  "% 5d / %d (by thread %3d) \r", i, njob-nadd, thread_no );
//			printf(  "% 5d - %5d / %d (thread %d)\n", i, j, njob, thread_no );
		}


		if( strlen( seq[i] ) == 0 || strlen( seq[j] ) == 0 )
		{
			if( store_dist )
			{
				if( alg == 'Y' || alg == 'r' ) distancemtx[i][j-(njob-nadd)] = 3.0;
				else distancemtx[i][j] = 3.0;
			}
			if( stdout_dist) 
			{
				pthread_mutex_lock( targ->mutex_stdout );
				printf(  "%d %d d=%.3f\n", i+1, j+1, 3.0 );
				pthread_mutex_unlock( targ->mutex_stdout );
			}
			continue;
		}

		strcpy( aseq[0], seq[i] );
		strcpy( aseq[1], seq[j] );
//		clus1 = conjuctionfortbfast( pair, i, aseq, mseq1, effarr1, effarr, indication1 );
//		clus2 = conjuctionfortbfast( pair, j, aseq, mseq2, effarr2, effarr, indication2 );
//		printf(  "Skipping conjuction..\n" );

		effarr1[0] = 1.0;
		effarr2[0] = 1.0;
		mseq1[0] = aseq[0];
		mseq2[0] = aseq[1];

		thereisx = thereisxineachseq[i] + thereisxineachseq[j];
//		strcpy( distseq1[0], dseq[i] ); // nen no tame
//		strcpy( distseq2[0], dseq[j] ); // nen no tame
		distseq1[0] = dseq[i];
		distseq2[0] = dseq[j];

//		printf(  "mseq1 = %s\n", mseq1[0] );
//		printf(  "mseq2 = %s\n", mseq2[0] );
	
#if 0
		printf(  "group1 = %.66s", indication1 );
		printf(  "\n" );
		printf(  "group2 = %.66s", indication2 );
		printf(  "\n" );
#endif
//		for( l=0; l<clus1; l++ ) printf(  "## STEP-eff for mseq1-%d %f\n", l, effarr1[l] );

		if( use_fft )
		{
			pscore = Falign( mseq1, mseq2, effarr1, effarr2, 1, 1, alloclen, &intdum, NULL, 0, NULL );
//			printf(  "pscore (fft) = %f\n", pscore );
			off1 = off2 = 0;
		}
		else
		{
			switch( alg )
			{
				case( 'R' ):
					if( nadd && njob-nadd <= j && njob-nadd <= i ) // new sequence doushi ha mushi
						pscore = 0.0;
					else
						pscore = (float)lastresx[i][j].score; // all pair
					break;
				case( 'r' ):
					if( nadd == 0 || ( i < njob-nadd && njob-nadd <= j ) )
						pscore = (float)lastresx[i][j-(njob-nadd)].score;
					else
						pscore = 0.0;
					break;
				case( 'L' ):
					if( nadd && njob-nadd <= j && njob-nadd <= i ) // new sequence doushi ha mushi
						pscore = 0.0;
					else
					{
						if( store_localhom )
						{
							pscore = L__align11( mseq1, mseq2, alloclen, &off1, &off2 );
							if( thereisx ) pscore = L__align11_noalign( distseq1, distseq2 ); // uwagaki
						}
						else
							pscore = L__align11_noalign( distseq1, distseq2 );
					}
//					pscore = G__align11_noalign( amino_dis, penalty, penalty_ex, distseq1, distseq2, alloclen ); // CHUUI!!!!!!
					break;
				case( 'Y' ):
					if( nadd == 0 || ( i < njob-nadd && njob-nadd <= j ) ) // new sequence vs exiting sequence nomi keisan
					{
						if( store_localhom )
						{
							pscore = L__align11( mseq1, mseq2, alloclen, &off1, &off2 );
							if( thereisx ) pscore = L__align11_noalign( distseq1, distseq2 ); // uwagaki
						}
						else
							pscore = L__align11_noalign( distseq1, distseq2 );
					}
					else
						pscore = 0.0;
					break;
				case( 'A' ):
					if( store_localhom )
					{
						pscore = G__align11( mseq1, mseq2, alloclen, outgap, outgap );
						if( thereisx ) pscore = G__align11_noalign( amino_dis, penalty, penalty_ex, distseq1, distseq2, alloclen ); // uwagaki
					}
					else
						pscore = G__align11_noalign( amino_dis, penalty, penalty_ex, distseq1, distseq2, alloclen ); // uwagaki
					off1 = off2 = 0;
					break;
				case( 'N' ):
//					pscore = G__align11_noalign( amino_dis, penalty, penalty_ex, mseq1, mseq2, alloclen );
					pscore = genL__align11( mseq1, mseq2, alloclen, &off1, &off2 );
					if( thereisx )
					{
						strcpy( dumseq1[0], distseq1[0] );
						strcpy( dumseq2[0], distseq2[0] );
						pscore = genL__align11( dumseq1, dumseq2, alloclen, &dum1, &dum2 ); // uwagaki
					}
					break;
				case( 't' ):
					off1 = off2 = 0;
//					pscore = G__align11_noalign( amino_dis, penalty, penalty_ex, mseq1, mseq2, alloclen );
					pscore = G__align11_noalign( amino_dis, penalty, penalty_ex, distseq1, distseq2, alloclen ); // tsuneni distseq shiyou
					break;
				case( 's' ):
					pscore = callmxscarna_giving_bpp( mseq1, mseq2, bpp[i], bpp[j], alloclen, i, j );
					off1 = off2 = 0;
					break;
				case( 'G' ):
					pscore = calldafs_giving_bpp( mseq1, mseq2, bpp[i], bpp[j], alloclen, i, j );
					off1 = off2 = 0;
					break;
#if 0 
				case( 'a' ):
					pscore = Aalign( mseq1, mseq2, effarr1, effarr2, clus1, clus2, alloclen );
					off1 = off2 = 0;
					break;
				case( 'K' ):
					pscore = genG__align11( mseq1, mseq2, alloclen );
					off1 = off2 = 0;
					break;
				case( 'H' ):
					pscore = recallpairfoldalign( mseq1, mseq2, i, j, &off1, &off2, alloclen );
					break;
				case( 'B' ):
				case( 'T' ):
					pscore = recalllara( mseq1, mseq2, alloclen );
					off1 = off2 = 0;
					break;
				case( 'M' ):
					pscore = MSalign11( mseq1, mseq2, alloclen );
					break;
#endif
				default:
					ErrorExit( "\n\nERROR IN SOURCE FILE\n\n" );
			}
		}

		if( alg == 't' || ( mseq1[0][0] != 0 && mseq2[0][0] != 0  ) ) // 't' no jouken ha iranai to omou. if( ( mseq1[0][0] != 0 && mseq2[0][0] != 0  ) )
		{
#if SCOREOUT
			printf(  "score = %10.2f (%d,%d)\n", pscore, i, j );
#endif
//			if( pscore > 0.0 && ( nadd == 0 || ( alg != 'Y' && alg != 'r' ) || ( i < njob-nadd && njob-nadd <= j ) ) ) x-ins-i de seido teika
			if( ( nadd == 0 || ( alg != 'Y' && alg != 'r' ) || ( i < njob-nadd && njob-nadd <= j ) ) )
			{
				if( !store_localhom )
					;
				else if( alg == 'R' )
					putlocalhom_last( mseq1[0], mseq2[0], localhomtable[i]+j, lastresx[i]+j );
				else if( alg == 'r' )
					putlocalhom_last( mseq1[0], mseq2[0], localhomtable[i]+j-(njob-nadd), lastresx[i]+j-(njob-nadd) );// ?????
				else if( alg == 'H' )
					putlocalhom_ext( mseq1[0], mseq2[0], localhomtable[i]+j, off1, off2, (int)pscore, strlen( mseq1[0] ) );
				else if( alg == 'Y' )
					putlocalhom2( mseq1[0], mseq2[0], localhomtable[i]+j-(njob-nadd), off1, off2, (int)pscore, strlen( mseq1[0] ) );
				else if( alg != 'S' && alg != 'V' )
					putlocalhom2( mseq1[0], mseq2[0], localhomtable[i]+j, off1, off2, (int)pscore, strlen( mseq1[0] ) );
			}

			if( (bunbo=MIN( selfscore[i], selfscore[j] )) == 0.0 )
				pscore = 2.0;
			else if( bunbo < pscore ) // mondai ari
				pscore = 0.0;
			else
				pscore = ( 1.0 - pscore / bunbo ) * 2.0;
		}
		else
		{
			pscore = 2.0;
		}

#if 1 // mutex
		if( stdout_align )
		{
			pthread_mutex_lock( targ->mutex_stdout );
			if( alg != 't' )
			{
				printf(  "sequence %d - sequence %d, pairwise distance = %10.5f\n", i+1, j+1, pscore );
				printf(  ">%s\n", name[i] );
				write1seq( stdout, mseq1[0] );
				printf(  ">%s\n", name[j] );
				write1seq( stdout, mseq2[0] );
				printf(  "\n" );
			}
			pthread_mutex_unlock( targ->mutex_stdout );
		}
		if( stdout_dist )
		{
			pthread_mutex_lock( targ->mutex_stdout );
			if( j == i+1 ) printf(  "%d %d d=%.3f\n", i+1, i+1, 0.0 );
			printf(  "%d %d d=%.3f\n", i+1, j+1, pscore );
			pthread_mutex_unlock( targ->mutex_stdout );
		}
#endif // mutex
		if( store_dist )
		{
			if( alg == 'Y' || alg == 'r' ) distancemtx[i][j-(njob-nadd)] = pscore;
			else distancemtx[i][j] = pscore;
		}
	}
}
#endif

static void pairalign( char **name, int *nlen, char **seq, char **aseq, char **dseq, int *thereisxineachseq, char **mseq1, char **mseq2, int alloclen, Lastresx **lastresx )
{
	int i, j, ilim, jst, jj;
	int off1, off2, dum1, dum2, thereisx;
	float pscore = 0.0; // by D.Mathog
	FILE *hat2p, *hat3p;
	double **distancemtx;
	double *selfscore;
	double *effarr1;
	double *effarr2;
	char *pt;
	char *hat2file = "hat2";
	LocalHom **localhomtable = NULL, *tmpptr;
	int intdum;
	double bunbo;
	char ***bpp = NULL; // mxscarna no toki dake
	char **distseq1, **distseq2;
	char **dumseq1, **dumseq2;

	if( store_localhom )
	{
		if( alg == 'Y' || alg == 'r' )
		{
			ilim = njob - nadd;
			jst = nadd;
		}
		else
		{
			ilim = njob;
			jst = njob;
		}
		localhomtable = (LocalHom **)calloc( ilim, sizeof( LocalHom *) );
		for( i=0; i<ilim; i++)
		{
			localhomtable[i] = (LocalHom *)calloc( jst, sizeof( LocalHom ) );
			for( j=0; j<jst; j++)
			{
				localhomtable[i][j].start1 = -1;
				localhomtable[i][j].end1 = -1;
				localhomtable[i][j].start2 = -1; 
				localhomtable[i][j].end2 = -1; 
				localhomtable[i][j].opt = -1.0;
				localhomtable[i][j].next = NULL;
				localhomtable[i][j].nokori = 0;
			}
		}
	}

	if( store_dist )
	{
		if( alg == 'Y' || alg == 'r' )
			distancemtx = AllocateDoubleMtx( njob, nadd );
		else
			distancemtx = AllocateDoubleMtx( njob, njob );
	}
	else distancemtx = NULL;

	if( alg == 'N' )
	{
		dumseq1 = AllocateCharMtx( 1, alloclen+10 );
		dumseq2 = AllocateCharMtx( 1, alloclen+10 );
	}
	distseq1 = AllocateCharMtx( 1, 0 ); // muda
	distseq2 = AllocateCharMtx( 1, 0 ); // muda

	selfscore = AllocateDoubleVec( njob );
	effarr1 = AllocateDoubleVec( njob );
	effarr2 = AllocateDoubleVec( njob );

#if 0
	printf(  "##### fftwinsize = %d, fftthreshold = %d\n", fftWinSize, fftThreshold );
#endif

#if 0
	for( i=0; i<njob; i++ )
		printf(  "TBFAST effarr[%d] = %f\n", i, effarr[i] );
#endif


//	writePre( njob, name, nlen, aseq, 0 );

	if( alg == 'R' )
	{
		printf(  "Calling last (http://last.cbrc.jp/)\n" );
		if( lastonce )
			calllast_once( njob, seq, njob, seq, lastresx );
		else
			calllast_fast( njob, seq, njob, seq, lastresx );
		printf(  "done.\n" );
//		nthread = 0; // igo multithread nashi
	}
	if( alg == 'r' )
	{
		printf(  "Calling last (http://last.cbrc.jp/)\n" );
		printf(  "nadd=%d\n", nadd );
#if 1 // last_fast ha, lastdb ga muda
		if( lastonce )
			calllast_once( njob-nadd, seq, nadd, seq+njob-nadd, lastresx );
		else
			calllast_fast( njob-nadd, seq, nadd, seq+njob-nadd, lastresx );
#else
		calllast_once( njob-nadd, seq, nadd, seq+njob-nadd, lastresx );
#endif

		printf(  "nadd=%d\n", nadd );
		printf(  "done.\n" );
//		nthread = 0; // igo multithread nashi
	}

	if( alg == 'H' )
	{
		printf(  "Calling FOLDALIGN with option '%s'\n", foldalignopt );
		callfoldalign( njob, seq );
		printf(  "done.\n" );
	}
	if( alg == 'B' )
	{
		printf(  "Running LARA (Bauer et al. http://www.planet-lisa.net/)\n" );
		calllara( njob, seq, "" );
		printf(  "done.\n" );
	}
	if( alg == 'T' )
	{
		printf(  "Running SLARA (Bauer et al. http://www.planet-lisa.net/)\n" );
		calllara( njob, seq, "-s" );
		printf(  "done.\n" );
	}
	if( alg == 's' )
	{
		printf(  "Preparing bpp\n" );
//		bpp = AllocateCharCub( njob, nlenmax, 0 );
		bpp = calloc( njob, sizeof( char ** ) );
		preparebpp( njob, bpp );
		printf(  "done.\n" );
		printf(  "Running MXSCARNA (Tabei et al. http://www.ncrna.org/software/mxscarna)\n" );
	}
	if( alg == 'G' )
	{
		printf(  "Preparing bpp\n" );
//		bpp = AllocateCharCub( njob, nlenmax, 0 );
		bpp = calloc( njob, sizeof( char ** ) );
		preparebpp( njob, bpp );
		printf(  "done.\n" );
		printf(  "Running DAFS (Sato et al. http://www.ncrna.org/)\n" );
	}

	for( i=0; i<njob; i++ )
	{
		pscore = 0.0;
		for( pt=seq[i]; *pt; pt++ )
			pscore += amino_dis[(int)*pt][(int)*pt];
		selfscore[i] = pscore;
//		printf(  "selfscore[%d] = %f\n", i, selfscore[i] );
	}

#if enablemultithread
	if( nthread > 0 ) // alg=='r' || alg=='R' -> nthread:=0 (sukoshi ue)
	{
		Jobtable jobpos;
		pthread_t *handle;
		pthread_mutex_t mutex_counter;
		pthread_mutex_t mutex_stdout;
		thread_arg_t *targ;

		if( alg == 'Y' || alg == 'r' ) jobpos.j = njob - nadd - 1;
		else jobpos.j = 0;
		jobpos.i = 0;

		targ = calloc( nthread, sizeof( thread_arg_t ) );
		handle = calloc( nthread, sizeof( pthread_t ) );
		pthread_mutex_init( &mutex_counter, NULL );
		pthread_mutex_init( &mutex_stdout, NULL );

		for( i=0; i<nthread; i++ )
		{
			targ[i].thread_no = i;
			targ[i].njob = njob;
			targ[i].jobpospt = &jobpos;
			targ[i].name = name;
			targ[i].seq = seq;
			targ[i].dseq = dseq;
			targ[i].thereisxineachseq = thereisxineachseq;
			targ[i].localhomtable = localhomtable;
			targ[i].distancemtx = distancemtx;
			targ[i].selfscore = selfscore;
			targ[i].bpp = bpp; 
			targ[i].lastresx = lastresx;
			targ[i].alloclen = alloclen;
			targ[i].mutex_counter = &mutex_counter;
			targ[i].mutex_stdout = &mutex_stdout;

//			athread( (void *)targ );
			pthread_create( handle+i, NULL, athread, (void *)(targ+i) );
//			pthread_create( handle+i, NULL, bthread, (void *)(targ+i) );
		}


		for( i=0; i<nthread; i++ )
		{
			pthread_join( handle[i], NULL );
		}
		pthread_mutex_destroy( &mutex_counter );
		pthread_mutex_destroy( &mutex_stdout );
		free( handle );
		free( targ );
	}
	else
#endif
	{
		if( alg == 'Y' || alg == 'r' ) ilim = njob - nadd;
		else ilim = njob - 1;
		for( i=0; i<ilim; i++ ) 
		{
			if( stdout_dist) printf(  "%d %d d=%.3f\n", i+1, i+1, 0.0 );
			printf(  "% 5d / %d\r", i, njob-nadd );
			fflush( stdout );

			if( alg == 'Y' || alg == 'r' ) jst = njob - nadd;
			else jst = i + 1;
			for( j=jst; j<njob; j++ )
			{
	
				if( strlen( seq[i] ) == 0 || strlen( seq[j] ) == 0 )
				{
					if( store_dist ) 
					{
						if( alg == 'Y' || alg == 'r' ) distancemtx[i][j-(njob-nadd)] = 3.0;
						else distancemtx[i][j] = 3.0;
					}
					if( stdout_dist) printf(  "%d %d d=%.3f\n", i+1, j+1, 3.0 );
					continue;
				}
	
				strcpy( aseq[0], seq[i] );
				strcpy( aseq[1], seq[j] );
//				clus1 = conjuctionfortbfast( pair, i, aseq, mseq1, effarr1, effarr, indication1 );
//				clus2 = conjuctionfortbfast( pair, j, aseq, mseq2, effarr2, effarr, indication2 );
//				printf(  "Skipping conjuction..\n" );

				effarr1[0] = 1.0;
				effarr2[0] = 1.0;
				mseq1[0] = aseq[0];
				mseq2[0] = aseq[1];

				thereisx = thereisxineachseq[i] + thereisxineachseq[j];
//				strcpy( distseq1[0], dseq[i] ); // nen no tame
//				strcpy( distseq2[0], dseq[j] ); // nen no tame
				distseq1[0] = dseq[i];
				distseq2[0] = dseq[j];

	//			printf(  "mseq1 = %s\n", mseq1[0] );
	//			printf(  "mseq2 = %s\n", mseq2[0] );
		
#if 0
				printf(  "group1 = %.66s", indication1 );
				printf(  "\n" );
				printf(  "group2 = %.66s", indication2 );
				printf(  "\n" );
#endif
	//			for( l=0; l<clus1; l++ ) printf(  "## STEP-eff for mseq1-%d %f\n", l, effarr1[l] );
	
				if( use_fft )
				{
					pscore = Falign( mseq1, mseq2, effarr1, effarr2, 1, 1, alloclen, &intdum, NULL, 0, NULL );
//					printf(  "pscore (fft) = %f\n", pscore );
					off1 = off2 = 0;
				}
				else
				{
					switch( alg )
					{
						case( 't' ):
//							pscore = G__align11_noalign( amino_dis, penalty, penalty_ex, mseq1, mseq2, alloclen );
							pscore = G__align11_noalign( amino_dis, penalty, penalty_ex, distseq1, distseq2, alloclen ); // tsuneni distseq shiyou
							off1 = off2 = 0;
							break;
						case( 'A' ):
							if( store_localhom )
							{
								pscore = G__align11( mseq1, mseq2, alloclen, outgap, outgap );
								if( thereisx ) pscore = G__align11_noalign( amino_dis, penalty, penalty_ex, distseq1, distseq2, alloclen ); // uwagaki
							}
							else
								pscore = G__align11_noalign( amino_dis, penalty, penalty_ex, distseq1, distseq2, alloclen ); // uwagaki
							off1 = off2 = 0;
							break;
						case( 'N' ):
//							pscore = G__align11_noalign( amino_dis, penalty, penalty_ex, mseq1, mseq2, alloclen );
							pscore = genL__align11( mseq1, mseq2, alloclen, &off1, &off2 );
							if( thereisx )
							{
								strcpy( dumseq1[0], distseq1[0] );
								strcpy( dumseq2[0], distseq2[0] );
								pscore = genL__align11( dumseq1, dumseq2, alloclen, &dum1, &dum2 ); // uwagaki
							}
							break;
						case( 'R' ):
							if( nadd && njob-nadd <= j && njob-nadd <= i ) // new sequence doushi ha mushi
								pscore = 0.0;
							else
								pscore = (float)lastresx[i][j].score; // all pair
							break;
						case( 'r' ):
							if( nadd == 0 || ( i < njob-nadd && njob-nadd <= j ) )
								pscore = (float)lastresx[i][j-(njob-nadd)].score;
							else
								pscore = 0.0;
							break;
						case( 'L' ):
							if( nadd && njob-nadd <= j && njob-nadd <= i ) // new sequence doushi ha mushi
								pscore = 0.0;
							else
							{
								if( store_localhom )
								{
									pscore = L__align11( mseq1, mseq2, alloclen, &off1, &off2 ); // all pair
									if( thereisx ) pscore = L__align11_noalign( distseq1, distseq2 ); // all pair
								}
								else
									pscore = L__align11_noalign( distseq1, distseq2 ); // all pair
							}
//							pscore = G__align11_noalign( amino_dis, penalty, penalty_ex, distseq1, distseq2, alloclen ); // CHUUI!!!!!!
							break;
						case( 'Y' ):
							if( nadd == 0 || ( i < njob-nadd && njob-nadd <= j ) ) // new sequence vs exiting sequence nomi keisan
							{
								if( store_localhom )
								{
									pscore = L__align11( mseq1, mseq2, alloclen, &off1, &off2 );
									if( thereisx ) pscore = L__align11_noalign( distseq1, distseq2 ); // uwagaki
								}
								else
									pscore = L__align11_noalign( distseq1, distseq2 );
							}
							else
								pscore = 0.0;
							break;
						case( 'a' ):
							pscore = Aalign( mseq1, mseq2, effarr1, effarr2, 1, 1, alloclen );
							off1 = off2 = 0;
							break;
#if 0
						case( 'K' ):
							pscore = genG__align11( mseq1, mseq2, alloclen );
							off1 = off2 = 0;
							break;
#endif
						case( 'H' ):
							pscore = recallpairfoldalign( mseq1, mseq2, i, j, &off1, &off2, alloclen );
							break;
						case( 'B' ):
						case( 'T' ):
							pscore = recalllara( mseq1, mseq2, alloclen );
							off1 = off2 = 0;
							break;
						case( 's' ):
							pscore = callmxscarna_giving_bpp( mseq1, mseq2, bpp[i], bpp[j], alloclen, i, j );
							off1 = off2 = 0;
							break;
						case( 'G' ):
							pscore = calldafs_giving_bpp( mseq1, mseq2, bpp[i], bpp[j], alloclen, i, j );
							off1 = off2 = 0;
							break;
						case( 'M' ):
							pscore = MSalign11( mseq1, mseq2, alloclen );
							break;
						default:
							ErrorExit( "ERROR IN SOURCE FILE" );
					}
				}
	
				if( alg == 't' || ( mseq1[0][0] != 0 && mseq2[0][0] != 0  ) ) // 't' no jouken ha iranai to omou. if( ( mseq1[0][0] != 0 && mseq2[0][0] != 0  ) )
				{
#if SCOREOUT
					printf(  "score = %10.2f (%d,%d)\n", pscore, i, j );
#endif
//					if( pscore > 0.0 && ( nadd == 0 || ( alg != 'Y' && alg != 'r' ) || ( i < njob-nadd && njob-nadd <= j ) ) ) // x-ins-i de seido teika
					if( ( nadd == 0 || ( alg != 'Y' && alg != 'r' ) || ( i < njob-nadd && njob-nadd <= j ) ) )
					{
						if( !store_localhom )
							;
						else if( alg == 'R' )
							putlocalhom_last( mseq1[0], mseq2[0], localhomtable[i]+j, lastresx[i]+j );
						else if( alg == 'r' )
							putlocalhom_last( mseq1[0], mseq2[0], localhomtable[i]+j-(njob-nadd), lastresx[i]+j-(njob-nadd) );// ?????
						else if( alg == 'H' )
							putlocalhom_ext( mseq1[0], mseq2[0], localhomtable[i]+j, off1, off2, (int)pscore, strlen( mseq1[0] ) );
						else if( alg == 'Y' )
							putlocalhom2( mseq1[0], mseq2[0], localhomtable[i]+j-(njob-nadd), off1, off2, (int)pscore, strlen( mseq1[0] ) );
						else if( alg != 'S' && alg != 'V' )
							putlocalhom2( mseq1[0], mseq2[0], localhomtable[i]+j, off1, off2, (int)pscore, strlen( mseq1[0] ) );
					}

					if( (bunbo=MIN( selfscore[i], selfscore[j] )) == 0.0 )
						pscore = 2.0;
					else if( bunbo < pscore ) // mondai ari
						pscore = 0.0;
					else
						pscore = ( 1.0 - pscore / bunbo ) * 2.0;
				}
				else
				{
					pscore = 2.0;
				}
	
				if( stdout_align )
				{
					if( alg != 't' )
					{
						printf(  "sequence %d - sequence %d, pairwise distance = %10.5f\n", i+1, j+1, pscore );
						printf(  ">%s\n", name[i] );
						write1seq( stdout, mseq1[0] );
						printf(  ">%s\n", name[j] );
						write1seq( stdout, mseq2[0] );
						printf(  "\n" );
					}
				}
				if( stdout_dist ) printf(  "%d %d d=%.3f\n", i+1, j+1, pscore );
				if( store_dist) 
				{
					if( alg == 'Y' || alg == 'r' ) distancemtx[i][j-(njob-nadd)] = pscore;
					else distancemtx[i][j] = pscore;
				}
			}
		}
	}


	if( store_dist )
	{
		hat2p = fopen( hat2file, "w" );
		if( !hat2p ) ErrorExit( "Cannot open hat2." );
		if( alg == 'Y' || alg == 'r' )
			WriteHat2_part_pointer( hat2p, njob, nadd, name, distancemtx );
		else
			WriteHat2_pointer( hat2p, njob, name, distancemtx );
		fclose( hat2p );
	}

	hat3p = fopen( "hat3", "w" );
	if( !hat3p ) ErrorExit( "Cannot open hat3." );
	if( store_localhom )
	{
		printf(  "\n\n##### writing hat3\n" );
		if( alg == 'Y' || alg == 'r' )
			ilim = njob-nadd;	
		else
			ilim = njob-1;	
		for( i=0; i<ilim; i++ ) 
		{
			if( alg == 'Y' || alg == 'r' )
			{
				jst = njob-nadd;
				jj = 0;
			}
			else
			{
				jst = i+1;
				jj = i+1;
			}
			for( j=jst; j<njob; j++, jj++ )
			{
				for( tmpptr=localhomtable[i]+jj; tmpptr; tmpptr=tmpptr->next )
				{
//					printf(  "j=%d, jj=%d\n", j, jj );
					if( tmpptr->opt == -1.0 ) continue;
// tmptmptmptmptmp
//					if( alg == 'B' || alg == 'T' )
//						fprintf( hat3p, "%d %d %d %7.5f %d %d %d %d %p\n", i, j, tmpptr->overlapaa, 1.0, tmpptr->start1, tmpptr->end1, tmpptr->start2, tmpptr->end2, (void *)tmpptr->next ); 
//					else
						fprintf( hat3p, "%d %d %d %7.5f %d %d %d %d h\n", i, j, tmpptr->overlapaa, tmpptr->opt, tmpptr->start1, tmpptr->end1, tmpptr->start2, tmpptr->end2 );
//						fprintf( hat3p, "%d %d %d %7.5f %d %d %d %d h\n", i, j, tmpptr->overlapaa, tmpptr->opt, tmpptr->start1, tmpptr->end1, tmpptr->start2+1, tmpptr->end2+1 ); // zettai dame!!!!
				}
			}
		}
#if DEBUG
		printf(  "calling FreeLocalHomTable\n" );
#endif
		if( alg == 'Y' || alg == 'r' )
			FreeLocalHomTable_part( localhomtable, (njob-nadd), nadd );
		else
			FreeLocalHomTable( localhomtable, njob );
#if DEBUG
		printf(  "done. FreeLocalHomTable\n" );
#endif
	}
	fclose( hat3p );

	if( alg == 's' )
	{
		char **ptpt;
		for( i=0; i<njob; i++ )
		{
			ptpt = bpp[i];
			while( 1 )
			{
				if( *ptpt ) free( *ptpt );
				else break;
				ptpt++;
			}
			free( bpp[i] );
		}
		free( bpp );
	}
	free( selfscore );
	free( effarr1 );
	free( effarr2 );
	if( alg == 'N' )
	{
		FreeCharMtx( dumseq1 );
		FreeCharMtx( dumseq2 );
	}
	free( distseq1 );
	free( distseq2 );
	if( store_dist ) FreeDoubleMtx( distancemtx );
}

	 

int main( int argc, char *argv[] )
{
	int  *nlen, *thereisxineachseq;
	char **name, **seq;
	char **mseq1, **mseq2;
	char **aseq;
	char **bseq;
	char **dseq;
	int i, j, k;
	FILE *infp;
	char c;
	int alloclen;
	Lastresx **lastresx;

	arguments( argc, argv );
#ifndef enablemultithread
	nthread = 0;
#endif
	fprintf (stderr, "lastonce = %d\n", lastonce );

	if( inputfile )
	{
		infp = fopen( inputfile, "r" );
		if( !infp )
		{
			printf(  "Cannot open %s\n", inputfile );
			exit( 1 );
		}
	}
	else
		infp = stdin;

	getnumlen( infp );
	rewind( infp );

	if( njob < 2 )
	{
		printf(  "At least 2 sequences should be input!\n"
						 "Only %d sequence found.\n", njob ); 
		exit( 1 );
	}
	if( njob > M )
	{
		printf(  "The number of sequences must be < %d\n", M );
		printf(  "Please try the splittbfast program for such large data.\n" );
		exit( 1 );
	}

	if( ( alg == 'r' || alg == 'R' ) && dorp == 'p' )
	{
		printf(  "Not yet supported\n" );
		exit( 1 );
	}

	alloclen = nlenmax*2;
	seq = AllocateCharMtx( njob, alloclen+10 );
	aseq = AllocateCharMtx( 2, alloclen+10 );
	bseq = AllocateCharMtx( njob, alloclen+10 );
	dseq = AllocateCharMtx( njob, alloclen+10 );
	mseq1 = AllocateCharMtx( njob, 0 );
	mseq2 = AllocateCharMtx( njob, 0 );
	name = AllocateCharMtx( njob, B );
	nlen = AllocateIntVec( njob );
	thereisxineachseq = AllocateIntVec( njob );

	if( alg == 'R' )
	{
		lastresx = calloc( njob+1, sizeof( Lastresx * ) );
		for( i=0; i<njob; i++ ) 
		{
			lastresx[i] = calloc( njob+1, sizeof( Lastresx ) ); // muda
			for( j=0; j<njob; j++ ) 
			{
				lastresx[i][j].score = 0;
				lastresx[i][j].naln = 0;
				lastresx[i][j].aln = NULL;
			}
			lastresx[i][njob].naln = -1;
		}
		lastresx[njob] = NULL;
	}
	else if( alg == 'r' )
	{
//		printf(  "Allocating lastresx (%d), njob=%d, nadd=%d\n", njob-nadd+1, njob, nadd );
		lastresx = calloc( njob-nadd+1, sizeof( Lastresx * ) );
		for( i=0; i<njob-nadd; i++ )
		{
//			printf(  "Allocating lastresx[%d]\n", i );
			lastresx[i] = calloc( nadd+1, sizeof( Lastresx ) );
			for( j=0; j<nadd; j++ ) 
			{
//				printf(  "Initializing lastresx[%d][%d]\n", i, j );
				lastresx[i][j].score = 0;
				lastresx[i][j].naln = 0;
				lastresx[i][j].aln = NULL;
			}
			lastresx[i][nadd].naln = -1;
		}
		lastresx[njob-nadd] = NULL;
	}
	else
		lastresx = NULL;

#if 0
	Read( name, nlen, seq );
#else
	readData_pointer( infp, name, nlen, seq );
#endif
	fclose( infp );

	constants( njob, seq );

#if 0
	printf(  "params = %d, %d, %d\n", penalty, penalty_ex, offset );
#endif

	initSignalSM();

	initFiles();

//	WriteOptions( trap_g );

	c = seqcheck( seq );
	if( c )
	{
		printf(  "Illegal character %c\n", c );
		exit( 1 );
	}

//	writePre( njob, name, nlen, seq, 0 );


	for( i=0; i<njob; i++ ) 
	{
		gappick0( bseq[i], seq[i] );
		thereisxineachseq[i] = removex( dseq[i], bseq[i] );
	}

	pairalign( name, nlen, bseq, aseq, dseq, thereisxineachseq, mseq1, mseq2, alloclen, lastresx );

	fprintf( trap_g, "done.\n" );
#if DEBUG
	printf(  "closing trap_g\n" );
#endif
	fclose( trap_g );

//	writePre( njob, name, nlen, aseq, !contin );
#if 0
	writeData( stdout, njob, name, nlen, aseq );
#endif
#if IODEBUG
	printf(  "OSHIMAI\n" );
#endif
	SHOWVERSION;

	if( stdout_dist && nthread > 1 )
	{
		printf(  "\nThe order of distances is not identical to that in the input file, because of the parallel calculation.  Reorder them by yourself, using sort -n -k 2 | sort -n -k 1 -s\n" );
	}
	if( stdout_align && nthread > 1 )
	{
		printf(  "\nThe order of pairwise alignments is not identical to that in the input file, because of the parallel calculation.  Reorder them by yourself.\n" );
	}

#if 1
	if( lastresx ) 
	{
		for( i=0; lastresx[i]; i++ ) 
		{
			for( j=0; lastresx[i][j].naln!=-1; j++ ) 
			{
				for( k=0; k<lastresx[i][j].naln; k++ )
				{
					free( lastresx[i][j].aln[k].reg1 );
					free( lastresx[i][j].aln[k].reg2 );
				}
				free( lastresx[i][j].aln );
			}
			free( lastresx[i] );
		}
		free( lastresx );
	}
#endif
	FreeCharMtx( seq );
	FreeCharMtx( aseq );
	FreeCharMtx( bseq );
	FreeCharMtx( dseq );
	FreeCharMtx( name );
	free( mseq1 );
	free( mseq2 );
	free( nlen );
	free( thereisxineachseq );

	return( 0 );
}
