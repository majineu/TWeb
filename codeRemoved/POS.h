//#include "Scorer.h"
#include "EasyFirstPOS.h"


void	Training(const char *pszConfigPath);
void	Tagging(const string & modelPath,   const string & testPath,  const string & resPath,  bool disHis);
void	TaggingGoldHis(const string & modelPath,   const string & testPath,  const string &goldPath,  
		  				const string & resPath,     bool disHis);
void	ModelDebuger(const string & modelPath);
int		EvalSen(SSentence *pGuess, SSentence *pGold);




