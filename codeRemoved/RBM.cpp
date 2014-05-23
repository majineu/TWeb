#include <cassert>
#include "Config.h"
#include "RBM.h"
#include "RBMUtil.h"

SRBM::SRBM(int vNum, int hNum): 
m_vNum(vNum), m_hNum(hNum)
{
	// allocate space
	m_pVB = (double *) m_pool.Allocate(sizeof(double) * vNum);
	m_pHB = (double *) m_pool.Allocate(sizeof(double) * hNum);
	m_ppW = (double **)m_pool.Allocate(sizeof(double*) * vNum);
	for (int i = 0; i < vNum; ++i)
		m_ppW[i] =  (double *) m_pool.Allocate(sizeof(double) * hNum);

	// randomly initialize 
	InitRandom();
}



void SRBM::
CondProb(double *pIn, int len, vector<double> &rProbs,  PROB_TYPE pType)
{
  if (pType == V_GIVEN_H && len != m_hNum)
  {
    fprintf(stderr, "Error: inLength %d vs hNum %d\n", len, m_hNum);
    exit(0);
  }
  
  if (pType == H_GIVEN_V && len != m_vNum)
  {
    fprintf(stderr, "Error: inLength %d vs vNum %d\n", len, m_vNum);
    exit(0);
  }


	if (pType == H_GIVEN_V)
  {
		// compute p(h_i|v), using the ith column of W
		rProbs.resize(m_hNum);
    for (int i = 0; i < m_hNum; ++i)
    {
			double act = m_pHB[i];
			for (int k = 0; k < m_vNum; ++k)
				act += m_ppW[k][i] * pIn[k];
			
			rProbs[i] = sigmoid(act);

//      getCol(m_ppW, m_vNum, m_hNum, i, pWeight);
//			rProbs[i] = sigmoid(m_pHB[i] + dotProduct(pWeight, m_vNum, pIn, len));
#if 0			
			if (m_verbose == true && i % 2 == 0)
			{
      	fprintf(stderr, "\n\ncolumn %d:", i);
				disVec(pWeight, m_vNum);
				fprintf(stderr, "input vec:");
				disVec(pIn, m_vNum);
				fprintf(stderr, "\ndot Product:%.2f, x:%.2f, probability:%.2f\n", 
								dotProduct(pWeight, m_vNum, pIn,len),
								m_pHB[i] + dotProduct(pWeight, m_vNum, pIn,len),
								rProbs[i]);
			}
#endif
    }
  }
  else
	{
		// compute p(v_i|h), using the ith row of W
		rProbs.resize(m_vNum);
    for (int i = 0; i < m_vNum; ++i)
      rProbs[i] = sigmoid(m_pVB[i] + dotProduct(m_ppW[i], m_hNum, pIn, len));
  }
}


void SRBM::
Sampling(double *pIn,  int len,  vector<double> &rProbs,  
				 vector<double> &rSamples,  SRBM::PROB_TYPE TYPE)
{
	int resLen = TYPE == SRBM::H_GIVEN_V ? m_hNum: m_vNum;
	CondProb(pIn, len, rProbs, TYPE);
	biSampling(&rProbs[0], rSamples, resLen);
}


void SRBM::
Reconstruct(double *pIn, 	int len, 
						vector<double> &rProbs, 	vector<double>& rRes)
{
	vector<double> Qvec, HidVec;
	Sampling(pIn, len, Qvec, HidVec, H_GIVEN_V);
	Sampling(&Qvec[0], m_hNum, rProbs, rRes, V_GIVEN_H);
}


bool SRBM::
SaveRBM(FILE *fp)
{
	if (fwrite(&m_vNum, sizeof(m_vNum), 1, fp) != (size_t)1 ||
			fwrite(&m_hNum, sizeof(m_hNum), 1, fp) != (size_t)1)
	{
		fprintf(stderr, "Save m_vNum or m_hNum failed\n");
		return false;
	}
	
	if (fwrite(m_pVB, sizeof(*m_pVB), m_vNum, fp) != (size_t)m_vNum ||
			fwrite(m_pHB, sizeof(*m_pHB), m_hNum, fp) != (size_t)m_hNum)
	{
		fprintf(stderr, "Save bias vector failed\n");
		return false;
	}

	for (int i = 0; i < m_vNum; ++i)
	{
		if (fwrite(m_ppW[i], sizeof(**m_ppW), m_hNum, fp) != (size_t)m_hNum)
		{
			fprintf(stderr, "Save weight matrix failed\n");
			return false;
		}
	}
	return true;
}



bool SRBM::
LoadRBM(FILE *fp)
{
	if (fread(&m_vNum, sizeof(m_vNum), 1, fp) != (size_t)1||
			fread(&m_hNum, sizeof(m_hNum), 1, fp) != (size_t)1)
	{
		fprintf(stderr, "Load vNum, hNum failed\n");
		return false;
	}

	fprintf(stderr, "RBM %d ~ %d\n", m_vNum, m_hNum);

	if (fread(m_pVB, sizeof(*m_pVB), m_vNum, fp) != (size_t)m_vNum ||
			fread(m_pHB, sizeof(*m_pHB), m_hNum, fp) != (size_t)m_hNum)
	{
		fprintf(stderr, "Loading bias failed\n");
		return false;
	}

	for (int i = 0; i < m_vNum; ++i)
	{
		if (fread(m_ppW[i], sizeof(**m_ppW), m_hNum, fp) != (size_t)m_hNum)
		{
			fprintf(stderr, "Loading weight matrix failed\n");
			return false;
		}
	}

	return true;
}


void SRBM::InitRandom()
{
	assert(m_pVB != NULL);
	assert(m_pHB != NULL);
	assert(m_ppW != NULL);
	assert(m_vNum > 0 && m_hNum > 0);

	for (int i = 0; i < m_vNum; ++i)
		m_pVB[i] = 0.0; //uniform(-0.1, 0.1);

	for (int j = 0; j < m_hNum; ++j)
		m_pHB[j] = 0.0; //uniform(-0.1, 0.1);

	for (int i = 0; i < m_vNum; ++i)
	{
		for (int j = 0; j < m_hNum; ++j)
			m_ppW[i][j] = uniform(-0.1, 0.1);
	}
}


void SRBM::DisBias(FILE *fp)
{
	fprintf(fp, "\nH bias:");
	for (int i = 0; i < m_hNum; ++i)
		fprintf(fp, "[%d:%.2f]  ", i, m_pHB[i]);

	fprintf(fp, "\n");
}


void SRBM::DisW(FILE *fp)
{
	fprintf(fp, "weight matrix(W):\n");

	for (int i = 0; i < m_vNum; ++i)
	{
		if (m_vNum > 40 && i % 40 != 0)
			continue;
		fprintf(fp, "w[%d]:", i);
		for (int j = 0; j < m_hNum; ++j)
			fprintf(fp, "[%d:%-5.3e]  ", j, m_ppW[i][j]);

		fprintf(fp, "\n");
	}

	fprintf(fp, "\n");
}



