// MDRipper.h : main header file for the PROJECT_NAME application
//
#pragma once

#include <sstream>
#include <iomanip>
#include <vector>
using namespace std;

typedef struct _versionDATA
{
	string unitID;
	DWORD addrCCRYPTVALUE;
	DWORD addrPASSWORDREC;
	DWORD addrACCOUNTREC;
	DWORD sizeACCOUNTREC;
	DWORD offsetACCOUNTUSER;
	DWORD offsetACCOUNTPASSWORD;

	// default constructor
	_versionDATA()
	{
		unitID = "";
		addrCCRYPTVALUE = 0;
		addrPASSWORDREC = 0;
		addrACCOUNTREC = 0;
		sizeACCOUNTREC = 0;
		offsetACCOUNTUSER = 0;
		offsetACCOUNTPASSWORD = 0;
	};
	// constructor
	_versionDATA(const string  _unitID,
		const DWORD _addrCCRYPTVALUE,
		const DWORD _addrPASSWORDREC,
		const DWORD _addrACCOUNTREC,
		const DWORD _sizeACCOUNTREC,
		const DWORD _offsetACCOUNTUSER,
		const DWORD _offsetACCOUNTPASSWORD)
	{
		unitID = _unitID;
		addrCCRYPTVALUE = _addrCCRYPTVALUE;
		addrPASSWORDREC = _addrPASSWORDREC;
		addrACCOUNTREC = _addrACCOUNTREC;
		sizeACCOUNTREC = _sizeACCOUNTREC;
		offsetACCOUNTUSER = _offsetACCOUNTUSER;
		offsetACCOUNTPASSWORD = _offsetACCOUNTPASSWORD;
	};
} 	versionDATA;

vector<versionDATA> releases(3);

class ErrorClass
{
public:
	ErrorClass(int err, char* title, DWORD errorword = 0, char* string = "")
	{
		ErrorType = err;
		ErrorWord = errorword;
		strcpy_s(ErrorTitle, title);
		strcpy_s(ErrorString, string);
	};

	~ErrorClass()
	{

	};
	int ErrorType;
	DWORD ErrorWord;
	char ErrorString[255];
	char ErrorTitle[255];
};

enum errorclass { SYSTEM_ERROR = 1, SYSTEM_ERROR2, APPLICATION_ERROR, APPLICATION_ERROR2 };
enum BC { BC8 = 0, BC10, BC11 };
