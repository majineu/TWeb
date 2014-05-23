#ifndef __EMBEDDING_DICT_H__
#define __EMBEDDING_DICT_H__
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unordered_map>
#include <cmath>
#include <cstring>
#include <cassert>
#include "util.h"
#include "Pool.h"


using std::unordered_map;
using std::string;
typedef std::unordered_map<string, int> DICT_TYPE;
#define NUM_RESEARSE 3
class CWordIDMap
{
public:
	const static int s_unkID = 0;
	const static int s_bID   = 1;
	const static int s_eID   = 2;
	CWordIDMap()
	{
		m_vWord.push_back("<unk>");
		m_vWord.push_back("<s>");
		m_vWord.push_back("</s>");
		m_vCount.resize(NUM_RESEARSE, 1);
		for (size_t i = 0; i < m_vWord.size(); ++i)
			m_dict[m_vWord[i]] = i;
	}

	void SaveDict(const char *pszPath)
	{
		FILE *fp = fopen(pszPath, "w");
		assert(fp);
		for (size_t i = 0; i < m_vWord.size(); ++i)
			fprintf(fp, "%s %d\n", m_vWord[i].c_str(), m_vCount[i]);
		fclose(fp);
	}



	void LoadDict(const char *pszPath)
	{
		FILE *fp = fopen(pszPath, "r");
		assert(fp);
		char buf[65536], key[1024];
		int count = 0;
		DICT_TYPE:: iterator iter = m_dict.end();
		while (fgets(buf, 65535, fp) != NULL)
		{
			for (int i = 0; i < (int)strlen(buf); ++i)
			{
				if (buf[i] == '\r' || buf[i] == '\n')
				{
					buf[i] = 0;
					break;
				}
			}

			sscanf(buf, "%s %d", key, &count);
			iter = m_dict.find(key);
			if (iter == m_dict.end())
			{
				m_dict[key] = m_vWord.size();
				m_vWord.push_back(key);
				m_vCount.push_back(count);
			}
			else
				m_vCount[iter->second] = count;
		}
		fprintf(stderr, "WordIDMap size %d\n", (int)m_dict.size());
		fclose(fp);
	}

	void ExtractDictFromText(const char *pszDict)
	{
		char buf[65536];
		FILE *fp = fopen(pszDict, "r");
		if (fp == NULL)
		{
			fprintf(stderr, "Error: open %s failed\n", pszDict);
			exit(0);
		}
		int lineNum = 0;
		DICT_TYPE:: iterator iter = m_dict.end();
		while (fgets(buf, 65535, fp) != NULL)
		{
			if (++lineNum % 10000 == 0)
				fprintf(stderr, "Processing %d line\r", lineNum);
			
			vector<char *> words = Split(buf, " \r\t\n");
			for (size_t i = 0; i < words.size(); ++i)
			{
				iter = m_dict.find(words[i]);
				if (iter == m_dict.end())
				{
					m_dict[words[i]] = m_vWord.size();
					m_vWord.push_back(words[i]);
					m_vCount.push_back(1);
				}
				else
					m_vCount[iter->second] += 1;
			}
		}
		
		fprintf(stderr, "\ntotal %d line, dict size %lu\n", lineNum, m_dict.size());
		fclose(fp);
	}
	
	int Inc(const string &word, bool inMode = true)
	{
		DICT_TYPE::iterator iter = m_dict.find(word);
		if (iter == m_dict.end())
		{
			if (inMode == true)
			{
				int id = (int)m_dict.size();
				m_dict[word] = id;
				m_vWord.push_back(word);
				m_vCount.push_back(1);
				return id;
			}
			else
				return m_dict["<unk>"];
		}
		else
		{
			m_vCount[iter->second] += 1;
			return iter->second;
		}
	}



	int GetID(const string & key)
	{
		DICT_TYPE ::iterator iter = m_dict.find(key);
		if (iter != m_dict.end())
			return iter->second;

		// unk id is 0;
		return s_unkID;
	}


	bool operator !=(const CWordIDMap &rMap) const	{return !(*this == rMap);}
	bool operator ==(const CWordIDMap &rMap) const
	{
		if (m_dict.size() != rMap.m_dict.size())
		{
			fprintf(stderr, "dict size inconsistent\n");
			return false;
		}
		
		if (m_vWord.size() != rMap.m_vWord.size())
		{
			fprintf(stderr, "word vector size inconsistent\n");
			return false;
		}
		
		if (m_vCount.size() != rMap.m_vCount.size())
		{
			fprintf(stderr, "counter size inconsistent\n");
			return false;
		}

		for (size_t i = 0; i < m_vWord.size(); ++i)
		{
			if (m_vWord[i] != rMap.m_vWord[i])
			{
				fprintf(stderr, "w %lu %s vs %s\n",
						i, m_vWord[i].c_str(), rMap.m_vWord[i].c_str());
				return false;
			}

			string key = m_vWord[i];
			if (m_dict.find(key) == m_dict.end())
			{
				fprintf(stderr, "%s not found in dict 1\n",
						key.c_str());

				exit(0);
			}

			if (rMap.m_dict.find(key) == rMap.m_dict.end())
			{
				fprintf(stderr, "%s not found in dict 2\n",
						key.c_str());
				exit(0);
			}

			const DICT_TYPE::const_iterator it1 = m_dict.find(key);
			const DICT_TYPE::const_iterator it2 = rMap.m_dict.find(key);
			int id1 = it1->second, id2 = it2->second;
			if (id1 != id2)
			{
				fprintf(stderr, "%s inconsistent id %d vs %d\n",
					key.c_str(),	id1, id2);
				return false;
			}

			if (m_vCount[id1] != rMap.m_vCount[id1])
			{
				fprintf(stderr, "count %s, id %lu %d vs %d\n",
						key.c_str(), i,  m_vCount[id1],  rMap.m_vCount[id1]);
				return false;
			}
		}
		return true;
	}
	
	void Check(int ID)						{assert(ID >= 0 && ID < (int)m_dict.size());}
	string GetWord(int ID)				{Check(ID); return m_vWord[ID];}
	int GetCount(int ID)					{Check(ID); return m_vCount[ID];}
	size_t size()									{return m_vCount.size();}
	vector<int> & GetCount()			{return m_vCount;}
	DICT_TYPE * GetDict()					{return &m_dict;}
	static int UnkID()										{return s_unkID;}
	static int SBegID()									{return s_bID;}
	static int SEndID()									{return s_eID;}

private:
	DICT_TYPE 				m_dict;
	vector<string> 		m_vWord;
	vector<int>  			m_vCount;
};


#endif  /*__EMBEDDING_DICT_H__*/
