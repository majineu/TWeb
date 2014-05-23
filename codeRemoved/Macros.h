#ifndef  _MACROS_H
#define _MACROS_H


#include <cstdlib>
#include <cstring>
#include <float.h>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
using std::vector;
using std::map;
using std::wstring;

#define _MIN_DOUBLE_SCORE  -1.3e100
#define _CRT_SECURE_NO_WARNINGS
#define DISABLE_CODE 0

#define CHK_NEW_FAILED( p )\
	if( p == NULL )\
{\
	printf("Error: out of memory\n%s: line%d\n",__FILE__, __LINE__);\
	exit(0);\
}

inline void CHK_OPEN_FAILED(FILE *fp, const wchar_t *path)
{
	if (fp == NULL)
	{
		fwprintf(stderr, L"Error: failed to open %s\n", path);
		exit(0);
	}
}


inline void CHK_OPEN_FAILED(FILE *fp, const char *path)
{
	if (fp == NULL)
	{
		fprintf(stderr, "Error: failed to open %s\n", path);
		exit(0);
	}
}




inline void removeNewLine( wchar_t * buf )
{
	if (buf[wcslen(buf) - 1] == L'\n')
		buf[wcslen(buf) - 1] = L'\0';

	if (buf[wcslen(buf) - 1] == L'\r')
		buf[wcslen(buf) - 1] = L'\0';
}

inline void Check(bool exp, const wchar_t *massage)
{
	if (!exp)
	{
		fwprintf(stderr, massage);
		exit(0);
	}
}


inline void removeNewLine(char * buf)
{
	if (buf[strlen(buf) - 1] == '\n')
		buf[strlen(buf) - 1] = '\0';

	if (buf[strlen(buf) - 1] == '\r')
		buf[strlen(buf) - 1] = '\0';
}



struct mapWSTRCmp
{
	bool operator ()(std::pair<std::wstring, int> &l, std::pair<std::wstring, int> &r)
	{
		return l.second > r.second;
	}
};

template<typename t1, typename t2, typename t3>
void mapSort(std::map<t1, t2> &rmap, vector<std::pair<t1, t2> > &vec)
{
	typename map<t1, t2>::iterator iter = rmap.begin(), end = rmap.end();

	for (; iter != end; ++iter)
		vec.push_back(std::pair<t1,t2>(iter->first, iter->second));
	
	std::sort(vec.begin(), vec.end(), t3());
}




#if defined(_MSC_VER) || defined(__BORLANDC__)
/* In linux, we can use finite directly,
 * In windows, we have to use _finite instead
 */
inline int finite(double x) { return _finite(x); }
inline int nan(double x){return _isnan(x);}
#endif

#endif
