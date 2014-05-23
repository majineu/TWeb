#ifndef __RANDOM_GEN_H__
#define __RANDOM_GEN_H__
#include<vector>
#include<set>
#include<cstdio>
#include<cmath>
//#include "RBMUtil.h"
// O(1) multinomial distribution generator based on the alias 
// algorithm: 

inline double uniform(double min, double max)
{
	return rand() / (RAND_MAX + 1.0) * (max - min) + min;
}

using std::vector;
class CRandomGen
{
public:
	CRandomGen() {m_nOutcome = -1;}
	bool Init(vector<double> &rP)		// the input multinomial distribution
	{
		fprintf(stderr, "Initializing alias table .....");
		m_vP.clear();
		m_vP.insert(m_vP.begin(), rP.begin(), rP.end());
		if (rP.size() == 0)
		{
			fprintf(stderr, "Error: multinomial distribution with 0 outcomes\n");
			exit(0);
		}

		m_nOutcome = rP.size();
		vector<int> L, S;													// set for those larger, smaller than
		m_vQ.resize(m_nOutcome, 0);
		m_vL.resize(m_nOutcome, -1);
		for (int i = 0; i < m_nOutcome; ++i)
		{
			m_vQ[i] = m_nOutcome * rP[i];
			if (m_vQ[i] > 1.0)
				L.push_back(i);
			else
				S.push_back(i);
		}

		while (S.size() > 0)
		{
			int l = L.back();
			int s = S.back();
			m_vL[s] = l;													// the alias fo s is l
			S.pop_back();
			
			m_vQ[l] -= 1 - m_vQ[s];
			if (1.0 - m_vQ[l] > 1.0e-6)  					// disgusting numerical bugs
			{
				L.pop_back();
				S.push_back(l);					
			}

			if (S.size() == 0)
				m_vQ[l] = 1.0; 											// for the last element, always 1.0;   
		}

		int nUndefine = 0;
		for (int i = 0; i < m_nOutcome; ++i)
			if (m_vL[i] == -1)
				++nUndefine;

		fprintf(stderr, "done\n");
		// post condition
		return nUndefine == 1;
	}

	bool SaveGen(const char *pszPath)
	{
		FILE *fp = fopen(pszPath, "wb");
		if (fp == NULL)
		{
			fprintf(stderr, "Open file %s failed\n", pszPath);
			return false;
		}

		if (SaveGen(fp) == false)
		{
			fprintf(stderr, "Error: save generator failed\n");
			exit(0);
		}

		fclose(fp);
		return true;
	}

	
	bool SaveGen(FILE *fp)
	{
		if (fwrite(&m_nOutcome, sizeof(m_nOutcome), 1, fp) != 1)
			return false;

		if (fwrite(&m_vQ[0], sizeof(double), m_vQ.size(), fp) != m_vQ.size())
			return false;

		if (fwrite(&m_vL[0], sizeof(int), m_vL.size(), fp) != m_vL.size())
			return false;

		if (fwrite(&m_vP[0], sizeof(double), m_vL.size(), fp) != m_vP.size())
			return false;

		return true;
	}


	bool LoadGen(const char *pszPath)
	{
		FILE *fp = fopen(pszPath, "rb");
		if (fp == NULL)
		{
			fprintf(stderr, "Error: Open file %s failed\n", pszPath);
			exit(0);
		}

		if (LoadGen(fp) == false)
		{
			fprintf(stderr, "Error: Loading generator failed\n");
			exit(0);
		}
		fclose(fp);
		return true;
	}


	bool LoadGen(FILE *fp)
	{
		if (fread(&m_nOutcome, sizeof(m_nOutcome), 1, fp) != 1)
			return false;

		m_vQ.resize(m_nOutcome);
		m_vL.resize(m_nOutcome);
		m_vP.resize(m_nOutcome);
		if (fread(&m_vQ[0], sizeof(double), m_nOutcome, fp) != m_vQ.size())
			return false;

		if (fread(&m_vL[0], sizeof(int), m_nOutcome, fp) != m_vL.size())
			return false;

		if (fread(&m_vP[0], sizeof(double), m_nOutcome, fp) != m_vP.size())
			return false;
		return true;
	}


	void Display(FILE *fp)
	{
		for (int i= 0; i < m_nOutcome; ++i)
			fprintf(fp, "%d: %.2e, %-2d\n",i, m_vQ[i], m_vL[i]);
	}
	
	double GetProb(int oid)
	{
		if (oid < 0 || oid >= m_nOutcome)
		{
			fprintf(stderr, "oid out %d of range\n", oid);
			exit(0);
		}
		return m_vP[oid];
	}

	int Sample()
	{
		if (m_nOutcome == -1)
		{
			fprintf(stderr, "Error: using un-initialized generator\n");
			exit(0);
		}

		// make sure all numbers are larger than -1.0
		double U = uniform(- 1, m_nOutcome - 1) + 1.0e-10; 	//deal with the bug, U = -1.0
		int I = ceil(U);
		double p = (double)I - U;
		int retVal = p < m_vQ[I]? I: m_vL[I];
		if (retVal == -1)
		{
			fprintf(stderr, "U %.10e, I %d, p %4e, m_vQ[I], %.10e, m_vL[I] %d\n",
					U, I, p, m_vQ[I], m_vL[I]);
			fgetc(stdin);
		}
		return retVal;
	}

private:
	int m_nOutcome;
	vector<double>	m_vQ;				// binomial distribution
	vector<int>			m_vL;				// alias for each random variable
	vector<double>  m_vP;				// store the old multinomial
};


#endif  /*__RANDOM_GEN_H__*/
