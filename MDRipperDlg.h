#pragma once

#include <windows.h>
#include <vector>
#include "controls.h"

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

struct MatchPair
{
	uint64_t posPFM;
	uint64_t posICM;
};


class Ripper
{
public:
	Ripper(Edit* p_eOutput);
	~Ripper() {}
	Edit* p_eOutput = nullptr;
	std::vector<MatchPair>  GetICMCandidates(HANDLE hf);
	void LoadICMFile(HANDLE hf, uint64_t start, uint64_t end);
private:
	void GetPasswordData(HANDLE tf);
	char header[41]{};
};

class Controller
{
public:
	Controller(HWND hwnd);
	~Controller() {}
	HWND GetHwnd() const { return m_hwnd; }
	void Command(HWND hwnd, int controlID, int command);
	void FileOpen(HWND hwnd);
	void OnScanComplete(std::vector<MatchPair>* results);
	bool IsBusy() const { return m_busy; }
	Ripper m_rRipper;
	HWND m_hEdit = nullptr;
	WNDPROC m_oldEditProc = nullptr;
private:
	Edit m_eOutput;
	Button m_bLoad;
	Button m_bClose;
	HWND m_hwnd;
	bool m_busy = false;
};
