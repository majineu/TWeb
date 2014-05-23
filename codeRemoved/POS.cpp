#include <ctime>
#include "_IO.h"
#include "Lexicon.h"
#include "POS.h"
#include "util.h"
#include "Config.h"
#include "Template.h"
#include "BaseExtractor.h"
#include "EnExtractor.h"
#include "ChExtractor.h"
using namespace EasyFirstPOS;

void CountWords(vector<SSentence *> & vSen, 
								map<wstring, int> & counter)
{
	counter.clear();
	for (size_t i = 0; i < vSen.size(); ++i)
	{
		for (int w = 0; w < vSen[i]->Length(); ++w)
			if (counter.find(vSen[i]->GenForm(w)) == counter.end())
				counter[vSen[i]->GenForm(w)] = 1;
			else
				counter[vSen[i]->GenForm(w)] ++;
	}
}

map<wstring, int> rawWordCounter;

void CountWordsRawForm(vector<SSentence *> &vSen, 
											 map<wstring, int> & counter)
{
	if (counter.size() != 0)
		return;
	for (size_t i = 0; i < vSen.size(); ++i)
		for (int w = 0; w < vSen[i]->Length(); ++w)
		{
			if (counter.find(vSen[i]->Word(w)) == counter.end())
				counter[vSen[i]->Word(w)] = 1;
			else
				counter[vSen[i]->Word(w)] ++;
		}
}

int EvalSen(SSentence *pGuess, SSentence *pGold)
{
	int nCorr = 0;
	for (int wind = 0; wind < pGuess->Length(); ++wind)
	{
		if (wcscmp(pGuess->Tag(wind), pGold->Tag(wind)) == 0)
			++nCorr;
	}
	return nCorr;
}

CBaseExtractor * 
CreateExtractor(CTemplateManager &tmpMgr, 
								CWordTagManager &lexicon,
								CWordIDMap &widMap, 
								int l, 	int r, 
								bool bInsert, bool fVerbose,
								bool eng)
{
  if (eng == true)
    return new CEnFeatureExtractor(&tmpMgr, &lexicon, &widMap, 
																	 l, r, bInsert, fVerbose); 
  else
    return new CChFeatureExtractor(&tmpMgr, &lexicon, &widMap, 
																	 l, r, bInsert, fVerbose); 
}

CBaseExtractor * LoadExtractor(const string & strPath,
									 CTemplateManager & tmpMgr,
									 CWordTagManager &lexicon,
									 CWordIDMap &widMap,
									 int l, int r, int bInsert,
									 bool fVerbose)
{
	fprintf(stderr, "\nLoading from %s\n", strPath.c_str());
	FILE *fp = fopen(strPath.c_str(), "r");
	if (fp == NULL)
	{
		fprintf(stderr, "Loading %s failed\n", strPath.c_str());
		exit(0);
	}

	char buf[1024];
	fgets(buf, 1024, fp);
	CBaseExtractor *pExtor = NULL;
	if (string(buf) == CEnFeatureExtractor().GetName())
		pExtor = new CEnFeatureExtractor(&tmpMgr, &lexicon, &widMap, 
																		l, r, bInsert, fVerbose);
	
	else if (string (buf) == CChFeatureExtractor().GetName())
		pExtor = new CChFeatureExtractor(&tmpMgr, &lexicon, &widMap, 
																		 l, r, bInsert, fVerbose);
	else
	{
		fprintf(stderr, "Error: unknown extractor type %s\n", buf);
		exit(0);
	}

	pExtor->Load(fp);
	fclose(fp);
	return pExtor;
}



struct SEvalRecorder
{
	int nTotal;
	int nCorrect;
	int nOOV;
	int nOOVCorr;
	double accuracy;

	SEvalRecorder()
	{
		nTotal = 0;
		nCorrect = 0;
		accuracy = 0;
		nOOV = 0;
		nOOVCorr = 0;
	}
};

SEvalRecorder 
EvalSentences(vector<SSentence*> &rGuesses, 
							vector<SSentence*> &rGold,
							const string &strPath)
{
	if (rGuesses.size() != rGold.size())
	{
		fprintf(stderr, "Error: sentence number inconsistent, gold %d vs guess %d\n",
			(int)rGold.size(), (int)rGuesses.size());
		exit(0) ;
	}

	FILE *fp = fopen(("debuging/eval" + strPath + ".txt").c_str(), "w");
	SEvalRecorder rec;
	for (size_t i = 0; i < rGuesses.size(); ++i)
	{
		if (rGuesses[i]->Length() != rGold[i]->Length())
		{
			fprintf(stderr, "Error sentence length inconsistent %d vs %d\n",
				(int)rGuesses[i]->Length(), (int)rGold[i]->Length());
			exit(0);
		}

		SSentence *pGuess = rGuesses[i], *pGold = rGold[i];
		for (int wind = 0; wind < pGuess->Length(); ++wind)
		{
			bool isOOV = rawWordCounter.find(pGold->Word(wind)) == rawWordCounter.end();
			if (wcscmp(pGuess->Tag(wind), pGold->Tag(wind)) == 0)
			{
				rec.nCorrect++;
				rec.nOOVCorr += isOOV;
			}
//			rec.nOOV += isOOV;
		}
//		rec.nCorrect+= EvalSen(rGuesses[i], rGold[i]);
		rec.nTotal	+= rGold[i]->Length();


		wchar_t  tURL[] = L"ADD";
		if (fp != NULL)
		{
			// for debuging purpose
			SSentence *pSen = rGuesses[i];
			for (int k = 0; k < pSen->Length(); ++k)
			{
				if (pSen->GenForm(k) == wstring(L"url"))
					pSen->SetTag(k, tURL); 
				string word = CCodeConversion::UnicodeToUTF8(pSen->Word(k));
				string tag  = CCodeConversion::UnicodeToUTF8(pSen->Tag(k));

				if (wcscmp(pSen->Tag(k), rGold[i]->Tag(k)) == 0)
					fprintf(fp, "%s\t%s\n",word.c_str(), tag.c_str());
				else
					fprintf(fp, "%s\t%s  |||||||||||||| %s\n",
										word.c_str(), tag.c_str(), wstr2utf8(rGold[i]->Tag(k)).c_str());
			}
			fprintf(fp, "\n");
		}
	}

	if (fp != NULL)	
		fclose(fp);

	rec.accuracy = 100.0 * rec.nCorrect/rec.nTotal;	
	return rec;
}

int Tagging(vector<SSentence*> &rSenVec,   CTagger &rTagger)
{
	clock_t start = clock();
	int nOOV = 0;
	for (size_t i = 0; i < rSenVec.size(); ++i)
	{
		if (i != 0 && i % 500 == 0)
			fprintf(stderr, "Tagging %lu sentence\r", i);
		SSentence *pRes = rTagger.Tagging(rSenVec[i]);
		for (int k = 0; k < pRes->Length(); ++k)
		{
			rSenVec[i]->SetTag(k, pRes->Tag(k));
			if (rawWordCounter.find(rSenVec[i]->Word(k)) == rawWordCounter.end())
			{
				++nOOV;
				if (rTagger.GetLexicon()->SeenWord(rSenVec[i]->Word(k)) == true)
					fprintf(stderr, "inconsistent %s\n", wstr2utf8(rSenVec[i]->Word(k)).c_str());
			}
		}
	}

	double secs = 1.0*(clock() - start)/CLOCKS_PER_SEC;
	fprintf(stderr, "Total %d sentences, %.2f secs eclipsed, %.2f sens per sec\r", 
		(int)rSenVec.size(),  secs,	 rSenVec.size()/secs);
	return nOOV;
}


void GetCoverage(vector<SSentence*> &rSenVec, SWRRBM *pRBM)
{
	int nUnk = 0, total = 0;
	CWordIDMap *pMap = pRBM->m_pMap;
	int unkId = pMap->GetID("<unk>");
	for (size_t i= 0; i < rSenVec.size(); ++i)
	{
		SSentence *pSen = rSenVec[i];
		total += pSen->Length();
		for (int w = 0; w < pSen->Length(); ++w)
		{
			if (pMap->GetID(wstr2utf8(pSen->GenForm(w))) == unkId)
				nUnk ++;
		}
	}

	fprintf(stderr, "total %d, unk %d, coverage %.2f%%\n",
					total, nUnk,  100.0 * (total - nUnk)/total);
}

void InitEmbedding(vector<SSentence *> &rSenVec,
									SWRRBM *pRBM, 
									CEmbeddingNeural *pNN,
									int cutoff)
{
	fprintf(stderr, "Init embedding with gen form ... ");
	if (pNN == NULL)
	{
		fprintf(stderr, "Null Embedding neural, nothing to be done\n");
		return ;
	}
	CWordIDMap *pRBMMap = pRBM == nullptr ? nullptr: pRBM->m_pMap;
	CWordIDMap *pNNMap  = pNN->GetWIDMap();
	map<wstring, int> wCounter;
	CountWords(rSenVec, wCounter);

	// init lexicon
	if (pRBMMap != nullptr)
		for (size_t i = 0; i < pRBMMap->size(); ++i)
			pNNMap->Inc(pRBMMap->GetWord(i));

	for (map<wstring, int>::iterator iter = wCounter.begin(); 
			 iter != wCounter.end(); 	++iter)
	{
		if (iter->second > cutoff)
			pNNMap->Inc(wstr2utf8(iter->first));
	}

	// init word embedding from wrrbm
	if (pRBMMap != nullptr)
	{
		for (size_t i = 0; i < pRBMMap->size(); ++i)
			pNN->AddEmbedding(pRBMMap->GetWord(i).c_str(), 
												pRBM->GetEmbedding(pRBMMap->GetWord(i).c_str()));
	}
	pNN->CompleteEmbedding();
	fprintf(stderr, "Done\n");
}

static string BuildPath()
{
	string path = CConfig::strPrefix;
	string trainSubPath = CConfig::strTrainPath.substr(CConfig::strTrainPath.rfind("/") + 1);
	
	if (trainSubPath.find("verysmall") != string::npos)
		path += string("vSmall_");
	else if (trainSubPath.find("small") != string::npos)
		path += string("small_");
	else
		path += string("full_");

	if (CConfig::bRBMNN == true)
		path += string("RBMNN_");

	char buf[100];
	if (CConfig::bDBNNN == true)
		path += string("DBNNN_HLayer_") + string(itoa(CConfig::nDBN_HLayer, buf, 10)) + string("_");


	if (CConfig::bRegularizing == true)
	{
		path += string("Reg_S_") + 
						GetNameFromDir(CConfig::strRegSamplePath) + "_M_"; 
		sprintf(buf, "_w_%.2f_", CConfig::fRegWeight);
		path += buf;
	}
	path += CConfig::strTempPath.substr(CConfig::strTempPath.rfind("/") + 1) + string(".");
	path += CConfig::strDLPrefix.substr(CConfig::strDLPrefix.rfind("/") + 1) + string(".");
	path += string("cf_") + string(itoa(CConfig::nCutoff, buf, 10)) + string(".");
	path += string("bs_") + string(itoa(CConfig::nBS, buf, 10)) + string(".");
	path += string("nRt_") + string(itoa(CConfig::nRetry, buf, 10));

	sprintf(buf, "feedEbd%d_updateEBDNN%d_IntL2%.4f_SSample%d_ebdCutoff%d_l%dr%dh%d_cBook%d_margin%.2f_rate%.2f", 
					(int)CConfig::bFeedEbd,
					(int)CConfig::bUpdateEbdNN,
					CConfig::fIntNNL2,
					(int)CConfig::bSingleSampleUpdate,
					CConfig::nCutoff,		
					CConfig::nLeft, 		
					CConfig::nRight, 
					CConfig::nHLevel,  	CConfig::nCodeBookLen,
					CConfig::fMargin,		CConfig::fRate);


	path += buf;
	path += "outLayer_" + CConfig::strOutLayerType ;
	
	path += "HEmbedding_";
	for (size_t i = 0; i < CConfig::vEHSize.size(); ++i)
	{
		sprintf(buf, "%s_%d_", 
							CConfig::vEHType[i].c_str(),
							CConfig::vEHSize[i]); 
		path += buf;
	}
	
	path += "hInteger_";
	for (size_t i = 0; i < CConfig::vIHSize.size(); ++i)
	{
		sprintf(buf, "%s_%d_", CConfig::vIHType[i].c_str(),
							CConfig::vIHSize[i]);
		path += buf;
	}
	
	path += "hCombined_";
	for (size_t i = 0; i < CConfig::vCHSize.size(); ++i)
	{
		sprintf(buf, "%s_%d_", 
							CConfig::vCHType[i].c_str(),
							CConfig::vCHSize[i]);
		path += buf;
	}



	if (CConfig::nLowCount > 0)
		path += string("low_") + string(itoa(CConfig::nLowCount, buf, 10));

	if (CConfig::bDyOracle == true)
	{
		path += string("dyNext_");
		switch (CConfig::dyNextBeam)
		{
		case CConfig::GOLD_TOP:
			path += "gTop";
			break;
		case CConfig::GOLD_SURVIVE:
			path += "gSurvive";
			break;
		default:
			fprintf(stderr, "Error: Unsupport next beam configura\n");
			exit(0);
		}
	}

	if (CConfig::bSearn == true)
		path += string("_searn");

	if (CConfig::nRareWord > 0)
		path += string("_rare_") + itoa(CConfig::nRareWord, buf, 10);
	return path;
}



void CompleteEmbedding(vector<SSentence *> &trainSen,
								 			 CEmbeddingNeural *pNN,
											 int cutoff)
{
	fprintf(stderr, "Completing embeddings with cutoff %d\n", cutoff);
	map<wstring, int> gCounter;
	CountWords(trainSen, gCounter);

	// initialize word id map of the embedding neural
	CWordIDMap *pNNMap  = pNN->GetWIDMap();
	for (size_t i = 0; i < trainSen.size(); ++i)
		for (int w = 0; w < trainSen[i]->Length(); ++w)
			if (gCounter[trainSen[i]->GenForm(w)] > cutoff)
				pNNMap->Inc(wstr2utf8(trainSen[i]->GenForm(w)).c_str());
	pNN->CompleteEmbedding();
}

void InitGID(CWordIDMap *pWIDMap, vector<SSentence *> &vSen)
{
	for (size_t i = 0; i < vSen.size(); ++i)
	{
		SSentence *pSen = vSen[i];
		for (int idx = 0; idx < pSen->Length(); ++idx)
			pSen->SetGID(idx, pWIDMap->GetID(wstr2utf8(pSen->GForm(idx))));
	}
}



void Train(vector<SSentence *> &senTrain,		 
					 vector<SSentence *> &senTrainGoldTag,	
					 vector<vector<SSentence *> > &senDev,
					 vector<vector<SSentence *> > &senDevGold,	 
					 vector<string>	&devNames,
					 vector<vector<SSentence *> > &senTest,
					 vector<vector<SSentence *> > &senTestGold,	 
					 vector<string>	&testNames,
					 EasyFirstPOS::CTagger &tagger) 
{
	CPool trainingPool;
	vector<SSentence *> senTrainSubsetGold, senTrainSubset;
	for (size_t i = 0; i < senTrainGoldTag.size(); i += 12)
		senTrainSubsetGold.push_back(senTrainGoldTag[i]);
	senTrainSubset = SSentence::CopySentences(senTrainSubsetGold, trainingPool);

	int nSkip = 0;
	double bestDevAcc = 0, bestTestAccByDev	= 0, bestTestAcc = 0; 
	int	bestDevIter = 0,   bestTestIter = 0;

	string path		  = BuildPath();
	fprintf(stderr, "\npath: %s\n", path.c_str());

	string logPath	  = path+ ".log";
	FILE *fpLog = fopen((CConfig::strLogDir + "/" + logPath).c_str(), "w");
	if (fpLog == NULL)
		fprintf(stderr, "Warning: Log file open failed\n");
	
	
	// OK, here we go...
	clock_t totalStart = 0;
	CBaseExtractor &fExtor = *tagger.GetExtractor();
	CTemplateManager &tpMgr  = *fExtor.GetTempMgr();
	CWordTagManager &lexicon = *fExtor.GetLexicon();
		
	CIntInputNeural *pNN  = tagger.GetIntNN();
	CCombinedNN			*pCnn = tagger.GetCNN(); 
	for (int nIter = 0; nIter < CConfig::nRound; ++ nIter)
	{
		fprintf(stderr, "\n-----------------------Training %d round----------------------\n", nIter);
		vector<int> randomIds = Shuffel((int)senTrainGoldTag.size());
		nSkip = 0;
//		tagger.GetScorer()->SetAvgMode(false);

		if (nIter < 4 && pNN != NULL)
			pNN->SetIMode(true);
		
//		tagger.GetScorer()->SetInsertMode(true);
		tagger.GetExtractor()->SetInsertMode(true);
		tagger.ResetStatis();
	
		clock_t start = clock();
		for (size_t i = 0; i < randomIds.size(); ++ i)
		{
			int senId	= randomIds[i];
			SSentence *pSen = tagger.TrainGreedy(senTrain[senId], senTrainGoldTag[senId],
																					 CConfig::fRate,  CConfig::fMargin);
			if (pSen != NULL && EvalSen(pSen,  senTrainGoldTag[senId]) != pSen->Length())
				fprintf(stderr, "Error: Sentence %d, incorrect result returned\n", senId);
			
			if (i != 0 && i % 200 == 0)
			{
				CTagger::SStatics statics = tagger.GetStatics();
				fprintf(stderr, "%lu sen, %d skipped, %d retry\r", i, 
						nSkip, statics.nRetry);
			}
		}

		CTagger::SStatics statics = tagger.GetStatics();
		double secs = (double)(clock() - start)/CLOCKS_PER_SEC;
		fprintf(stderr, "%d sen, %d skipped, %d retry, %.2f secs\n",
			(int)randomIds.size(),	nSkip,	statics.nRetry,		secs);

		fprintf(stderr, "%d invalid, %d no valid, %d traceback\n",
			statics.nInvalid,	statics.nNoValid,  statics.nTraceBack);

//		tagger.GetScorer()->SetAvgMode(true);
//		tagger.GetScorer()->SetInsertMode(false);
		tagger.GetExtractor()->SetInsertMode(false);
		
		if (pNN != NULL)
			pNN->SetIMode(false);	

		// evaluate gold sentences
		fprintf(stderr, "\nEvaluating on training set ....\n");
		Tagging(senTrainSubset, tagger);
		SEvalRecorder eRec = EvalSentences(senTrainSubset, senTrainSubsetGold, "");
		fprintf(stderr, "Total %d, correct %d, accuracy %.2f%%\n", 
			eRec.nTotal,  eRec.nCorrect,  100.0 * eRec.nCorrect/eRec.nTotal);

		if (fpLog != NULL)
			fprintf(fpLog, "Iter %d, trainAcc %.2f%%, ", nIter, eRec.accuracy);


		// evaluate dev set
		double total = 0, totalCrr = 0, avgAcc = 0.0;
		fprintf(stderr, "Evaluating on dev set epoch %d\n", nIter);
		if (fpLog != NULL)
			fprintf(fpLog, "Evaluating on dev set epoch %d\n", nIter);
		vector<double> vTotal, vCorr, vOOV, vOOVCorr;
		for (size_t i = 0; i < senDev.size(); ++i)
		{
			vOOV.push_back(Tagging(senDev[i], tagger));
			eRec = EvalSentences(senDev[i], senDevGold[i], path + devNames[i]);
			vTotal.push_back(eRec.nTotal);
			vCorr.push_back(eRec.nCorrect);
			vOOVCorr.push_back(eRec.nOOVCorr);

			if (devNames[i].find("wsj") == devNames[i].npos)
			{
				total += eRec.nTotal;
				totalCrr += eRec.nCorrect;
			}
		}

		for (size_t i = 0; i < senDev.size(); ++i)
		{
			fprintf(stderr, "%-16s:", devNames[i].c_str());
			fprintf(stderr, "%-4.2f%%, %-4.2f%%, %-4.2f%%, oov %-6d %.2f %-6d/%-6d\n", 
							100.0 * vCorr[i]/vTotal[i],
							100.0 * (vCorr[i] - vOOVCorr[i])/(vTotal[i] - vOOV[i]),
							100.0 * vOOVCorr[i]/vOOV[i],
							(int)vOOV[i], 100.0* vOOV[i]/vTotal[i], (int)vCorr[i], (int)vTotal[i]); 
			if (fpLog != NULL)
			{
				fprintf(fpLog, "%-16s:", devNames[i].c_str());
				fprintf(fpLog, " %-6d/%-6d, accuracy %.2f%%\n", 
							(int)vCorr[i], (int)vTotal[i], 100.0 * vCorr[i]/vTotal[i]);
			}
		}
		
		avgAcc = 100.0 * totalCrr/ total;
		fprintf(stderr, "Average accuracy (wsj part excluded) %.2f%%\n", avgAcc);
		if (fpLog != NULL)
			fprintf(fpLog, "Average accuracy (wsj part excluded) %.2f%%\n", avgAcc);
		
		if (bestDevAcc < avgAcc)
		{
			bestDevAcc = avgAcc;
			bestDevIter = nIter;
		}


		// evaluate on test set
		total = 0, totalCrr = 0, avgAcc = 0.0;
		fprintf(stderr, "Evaluating on test set epoch %d\n", nIter);
		if (fpLog != NULL)
			fprintf(fpLog, "Evaluating on test set epoch %d\n", nIter);
		vTotal.clear(), vCorr.clear(), vOOV.clear(), vOOVCorr.clear();
		for (size_t i = 0; i < senTest.size(); ++i)
		{
			vOOV.push_back(Tagging(senTest[i], tagger));
			eRec = EvalSentences(senTest[i], senTestGold[i], path + testNames[i]);
			vTotal.push_back(eRec.nTotal);
			vCorr.push_back(eRec.nCorrect);
			vOOVCorr.push_back(eRec.nOOVCorr);

			if (testNames[i].find("wsj") == testNames[i].npos)
			{
				total += eRec.nTotal;
				totalCrr += eRec.nCorrect;
			}
		}

		for (size_t i = 0; i < senTest.size(); ++i)
		{
			fprintf(stderr, "%-16s:", testNames[i].c_str());
			fprintf(stderr, "%-4.2f%%, %-4.2f%%, %-4.2f%%, oov %-6d %.2f %-6d/%-6d\n", 
							100.0 * vCorr[i]/vTotal[i],
							100.0 * (vCorr[i] - vOOVCorr[i])/(vTotal[i] - vOOV[i]),
							100.0 * vOOVCorr[i]/vOOV[i],
							(int)vOOV[i], 100.0* vOOV[i]/vTotal[i], (int)vCorr[i], (int)vTotal[i]); 
			if (fpLog != NULL)
			{
				fprintf(fpLog, "%-20s:", testNames[i].c_str());
				fprintf(fpLog, " %-6d/%-6d, accuracy %.2f%%\n", 
							(int)vCorr[i], (int)vTotal[i], 100.0 * vCorr[i]/vTotal[i]);
			}
		}


		avgAcc = 100.0 * totalCrr/ total;
		fprintf(stderr, "Average accuracy (wsj part excluded) %.2f%%\n", avgAcc);
		if (fpLog != NULL)
			fprintf(fpLog, "Average accuracy (wsj part excluded) %.2f%%\n", avgAcc);

		if (bestDevIter == nIter)
			bestTestAcc = avgAcc;
	}

	
	double secs = (double)(clock() - totalStart)/CLOCKS_PER_SEC;
	if (fpLog != NULL)
	{
		fprintf(fpLog, "\nbestDevIter %d, bestDevAcc %.2f%%, bestTestAccByDev %.2f%%\nbestTestIter %d, bestTestAcc %.2f%%\n",
			bestDevIter, bestDevAcc, bestTestAccByDev, bestTestIter, bestTestAcc);
		fprintf(fpLog, "%.2f secs or %.2f hours eclipsed\n", secs, secs/3600);
		fflush(fpLog);
		fclose(fpLog);
	}
	fprintf(stderr, "\nbestDevIter %d, bestDevAcc %.2f%%, bestTestAccByDev %.2f%%\nbestTestIter %d, bestTestAcc %.2f%%\n",
		bestDevIter, bestDevAcc, bestTestAccByDev, bestTestIter, bestTestAcc);
	fprintf(stderr, "%.2f secs or %.2f hours eclipsed\n", secs, secs/3600);

	// save model file 
	if (CConfig::bJK == false)
	{
		string modelPath  = CConfig::strModelPath + path + ".model"; 
		string fCptPath	  = CConfig::strModelPath + path + ".cpt"; 

		string lexPath	  = CConfig::strModelPath + path + ".lex"; 
		string confPath   = CConfig::strModelPath + path + ".confg";
		string featurePath= CConfig::strModelPath + path + ".feature"; 
		string tempPath   = CConfig::strModelPath + path + ".temp";
	
		pCnn->Save(modelPath);
		fExtor.Save(fCptPath.c_str());
		CConfig::SaveConfig(confPath.c_str());			// save configuration
		tpMgr.Save(tempPath.c_str());					// save templates 
		lexicon.Save(lexPath.c_str());					// save lexicon and cutoff
	}

}


string BuildJKPath()
{
	string path = "JKs\\";
	path += CConfig::strTempPath;

	char nFordBuf[5];
	path += string("_") + itoa(CConfig::nFordJK, nFordBuf, 10);
	path += string("_fold");
	return path;
}

string GetFileNameFromDir(string path)
{
	return path.substr(path.rfind('/') + 1, path.find('.') - path.rfind('/') - 1);
}


void Training(const char * pszConfigPath)
{
	if (CConfig::ReadConfig(pszConfigPath) == false)
	{
		fprintf(stderr, "Reading config file failed\n");
		return ;
	}

	// reading training, dev and test data
	CPool trainingPool;
	vector<SSentence *> senTrain,   senTrainGoldTag;
	vector<vector<SSentence *> > senDev,	senDevGold;
	vector<vector<SSentence *> > senTest,	senTestGold;
	vector<string> devNames, testNames;

	senTrainGoldTag = CReader::ReadSentences(CConfig::strTrainPath.c_str(),  trainingPool);
	senTrain		= SSentence::CopySentences(senTrainGoldTag,  trainingPool);

	// load dev and test set
	for (size_t i = 0; i < CConfig::vDevPaths.size(); ++i)
	{
		devNames.push_back(GetFileNameFromDir(CConfig::vDevPaths[i]));
		senDevGold.push_back(CReader::ReadSentences(CConfig::vDevPaths[i].c_str(),  trainingPool));
		senDev.push_back(SSentence::CopySentences(senDevGold.back(), trainingPool));		// tags are not copied
	}
	
	for (size_t i = 0; i < CConfig::vTestPaths.size(); ++i)
	{
		testNames.push_back(GetFileNameFromDir(CConfig::vTestPaths[i]));
		senTestGold.push_back(CReader::ReadSentences(CConfig::vTestPaths[i].c_str(),  trainingPool));
		senTest.push_back(SSentence::CopySentences(senTestGold.back(), trainingPool));		// tags are not copied
	}


	// extract training sub-set
	CTemplateManager tpMgr;
	if (tpMgr.ReadingTemplate(CConfig::strTempPath.c_str()) == false)
	{
		fprintf(stderr, "Reading template failed %s\n", CConfig::strTempPath.c_str());
		return;
	}
	bool featureVerbose = false;
	bool tagVerbose = CConfig::bTagVerbose;
	bool insertMode	= true;

	CountWordsRawForm(senTrain, rawWordCounter);

	SWRRBM *pRBM = nullptr;
	if (CConfig::strDLPrefix != CConfig::strNull)
		pRBM = SWRRBM::RBMFromFile(CConfig::strDLPrefix.c_str());

	if (pRBM != nullptr)
	{
		fprintf(stderr, "Train embedding set:");
		GetCoverage(senTrain, pRBM);
		
		for (size_t i = 0; i < senDev.size(); ++i)
		{
			fprintf(stderr, "%-15s:", devNames[i].c_str());
			GetCoverage(senDev[i], pRBM);
		}
		
		for (size_t i = 0; i < senTest.size(); ++i)
		{
			fprintf(stderr, "%-15s:", testNames[i].c_str());
			GetCoverage(senTest[i], pRBM);
		}
	}
	
  CFeatureIDMap::InitClassLabels(senTrainGoldTag);

	// initialize integer-Neural
	vector<pair<int, string>> typeVec;
	for (size_t i = 0; i < CConfig::vIHType.size(); ++i)
		typeVec.push_back(pair<int, string>(CConfig::vIHSize[i], 		CConfig::vIHType[i]));
	CIntInputNeural intNN = CIntInputNeural(typeVec, CConfig::fIntNNL2);
	intNN.SetIMode(true);

	

	// initialize embedding-Neural
	CEmbeddingNeural *pENN = NULL;
	if (CConfig::bRBMNN == true)
		pENN = new CEmbeddingNeural(pRBM, CConfig::bFeedEbd);
//	else if (CConfig::bDBNNN == true)
//	{
//		assert(pDBN != nullptr);
//		pENN = new CEmbeddingNeural(pDBN, CConfig::nDBN_HLayer, CConfig::nDBN_HFeed);
//	}
	else
	{
		typeVec.clear();
		for (size_t i = 0; i < CConfig::vEHSize.size(); ++i)
			typeVec.push_back(pair<int, string>(CConfig::vEHSize[i],
																					CConfig::vEHType[i]));
		
		pENN = new CEmbeddingNeural(CConfig::nLeft, CConfig::nRight, 
																pRBM->m_wDim,	typeVec);
	}
	CEmbeddingNeural &ebdNN = *pENN;


	// initialize HigherLevel
	typeVec.clear();
	typeVec.push_back(pair<int, string>(intNN.GetOutputDim() + ebdNN.GetHFeedDim(), "linear"));
	for (size_t i = 0; i < CConfig::vCHSize.size(); ++i)
		typeVec.push_back(pair<int, string>(CConfig::vCHSize[i], 		CConfig::vCHType[i]));
	typeVec.push_back(pair<int, string>(CFeatureIDMap::GetLabelNum(), CConfig::strOutLayerType));
	CNeuralNet NN(typeVec);


	CWordTagManager  lexicon(CConfig::nCutoff);
	lexicon.InitWordTagManager(senTrainGoldTag);

	if (CConfig::bRBMNN == true || CConfig::bDBNNN == true)
		CompleteEmbedding(senTrain, &ebdNN, CConfig::nEbdCutoff);
	else
		InitEmbedding(senTrain,  pRBM, &ebdNN, CConfig::nEbdCutoff);

	InitGID(ebdNN.GetWIDMap(), senTrain);
	for (size_t i = 0; i < CConfig::vDevPaths.size(); ++i)
		InitGID(ebdNN.GetWIDMap(), senDev[i]);
	
	for (size_t i = 0; i < CConfig::vTestPaths.size(); ++i)
		InitGID(ebdNN.GetWIDMap(), senTest[i]);
	

	int nLeft  = CConfig::bRBMNN ? pRBM->m_nLeft  : CConfig::nLeft;
	int nRight = CConfig::bRBMNN ? pRBM->m_nRight : CConfig::nRight;

	CBaseExtractor *pExtractor = CreateExtractor(tpMgr, lexicon, 
															 *ebdNN.GetWIDMap(),
															 nLeft,			//ebdNN.GetLeft(), 
															 nRight,		//ebdNN.GetRight(),
															 insertMode, featureVerbose, 
															 CConfig::bEnTagger);
	
	// initialize the combinedNN
	CCombinedNN combNN(&intNN, &ebdNN, &NN);
  if (pExtractor == NULL)
  {
    fprintf(stderr, "Error: new feature extractor failed\n");
    exit(0);
  }

  CBaseExtractor &fExtor = *pExtractor;
	fprintf(stderr, "Total number of labels %lu\n", 
					CFeatureIDMap::GetLabelNum());
	
	CTagger tagger(tagVerbose, CConfig::bIntNN);
	tagger.Init(&fExtor, // &scorer,  
							&intNN,  &ebdNN,  &combNN, 	
							&lexicon,  CConfig::bSingleSampleUpdate, 
							CConfig::nBS, 	CConfig::nGoldBS);
	
	Train(senTrain, senTrainGoldTag, 
				senDev,		senDevGold, 	devNames,
				senTest, 	senTestGold, 	testNames,
				tagger);
  delete pExtractor;
}

