#ifndef __RBM_UTIL_H__
#define __RBM_UTIL_H__

#include<cstdlib>
#include<cstdio>
#include<cmath>
#include<vector>
#include<algorithm>
#include "util.h"
using std::vector;

double *scalarMult(double *pV, int len, double scalar);
double dotProduct(double *pV1, int len1,  double *pV2, int len2);
void getCol(double **pMatrix, int rNum, int cNum, int cIdx, double *pRes);
void biSampling(double *pProbs, double *pValues, int len);
void biSampling(double *pProbs, vector<double> &rValues, int len);
void disVec(double *pVec, int len, FILE *fp = stderr);
void disSparseVec(double *pVec, int len, FILE *fp = stderr);
void vecInc(double *p1, double *p2, int len);
void normalize(double *pVec, int len);
void disVec(double *p, double thres, double upperBound, int window, int dim, FILE *fp);
double logsumexp(double *pData, int len); 
inline void stopNan(double val)
{
	if (isnan(val))
	{
		fprintf(stderr, "find nan\n");
		fgetc(stdin);
	}
}

inline double sigmoid(double x)
{
  return 1.0 / (1.0 + exp(-x));
}

template<class t>
inline vector<t> shuffleData(vector<t> & rVec)
{
	vector<t> res(rVec.begin(), rVec.end());
	std::random_shuffle(res.begin(), res.end());
	return res;
}

inline double uniform(double min, double max)
{
	return rand() / (RAND_MAX + 1.0) * (max - min) + min;
}
#endif  /*__UTIL_H__*/
