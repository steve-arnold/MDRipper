#pragma once

#include <windows.h>
#include <vector>
#include "controls.h"

constexpr auto MAX_PU = 1024;
constexpr auto ICM_NUMBER = 18;
#define ICM_ADDRESS         (ICM_NUMBER * 4)
#define PFM_NUMBER          (ICM_NUMBER + 1)
#define PFM_ADDRESS         (PFM_NUMBER * 4)
constexpr auto IDS_ABOUTBOX = 101;
constexpr auto PASSWORDLENGTH = 17;
constexpr auto ACCOUNTUSERNAMELENGTH = 20;
constexpr auto ACCOUNTPASSWORDLENGTH = 20;

class WinException
{
public:
	WinException(char* msg)
		: _err(GetLastError()), _msg(msg) {}
	DWORD GetError() const { return _err; }
	char* GetMessage() const { return _msg; }
private:
	DWORD _err;
	char* _msg;
};

// out of memory handler that throws WinException
int NewHandler(size_t size);

class Ripper
{
public:
	Ripper(Edit* p_eOutput);
	~Ripper() {}
	void FileLoad(HANDLE hf);
	Edit* p_eOutput;
private:
	bool GetFilePointers(HANDLE hf);
	bool GetICM(HANDLE hf);
	void SetPUName(char* name);
	void LoadICMFile(HANDLE hf, DWORD start, DWORD end);
	void GetPasswordData(HANDLE tf);
	DWORD ChangeEndian(DWORD source);

	LARGE_INTEGER pointEOF{}; // end of file address
	DWORD pointDST{}; // offset to Data Structure Table from end of file
	DWORD point3{}; // end of LIMDISP from end of file
	char header[41]{};
};

class Controller
{
public:
	Controller(HWND hwnd);
	~Controller() {}
	void Command(HWND hwnd, int controlID, int command);
	void FileOpen(HWND hwnd);
private:
	Edit        m_eOutput;
	Button      m_bLoad;
	Button      m_bClose;
	Ripper      m_rRipper;
};
