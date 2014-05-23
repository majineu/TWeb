#ifndef __R_B_M_H__
#define __R_B_M_H__

#include<vector>
#include<cstdlib>
#include<cstring>

#include"util.h"
#include"Pool.h"

using std::vector;
struct SRBM
{
  enum PROB_TYPE{V_GIVEN_H, H_GIVEN_V};
public:
	// only shallow copy constructor
	SRBM(int vNum, int hNum, double ** W, double * VB, 
			 double * HB): m_vNum(vNum), m_hNum(hNum), m_ppW(W), 
			 m_pVB(VB), m_pHB(HB){}
	SRBM(int vNum, int hNum);
	void InitRandom();
	
	// main functions for a rbm
	void CondProb(double *pIn, int len, vector<double> &rProbs, PROB_TYPE pType);     // compute conditional probability
	void Sampling(double *pIn,  int len, vector<double> &rProbs,  
							 vector<double> &rSamples,  SRBM::PROB_TYPE TYPE);

	// reconstruct visuable units
	void Reconstruct(double *pIn, int len, vector<double> &rProbs, vector<double> &rRes);
	void DisBias(FILE *fp = stderr);
	void DisW(FILE *fp = stderr);
	void SetVerbose(bool verbose){m_verbose = verbose;}

	bool SaveRBM(FILE *fp);
	bool LoadRBM(FILE *fp);

	// data members
  int 		m_vNum;
  int 		m_hNum;
  double **m_ppW;
  double *m_pVB;      	// v bias
  double *m_pHB;      	// h bias
	bool 		m_verbose;
	CPool		m_pool;
private:
	SRBM(){memset(this, sizeof(*this), 0);}
};

#endif
