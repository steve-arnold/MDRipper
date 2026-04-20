#include "stdafx.h"
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")

HINSTANCE hInst = 0;
static char appAboutName[] = "About MDRipper";

// Globals
#define WM_SCAN_COMPLETE (WM_APP + 1)

struct ScanContext
{
	Controller* ctrl;
	HANDLE hFile;
};

int NewHandler(size_t size)
{
	UNREFERENCED_PARAMETER(size);
	throw WinException("Out of memory");
}

// Forward declarations of functions included in this code module:
INT_PTR CALLBACK DialogProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK EditSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
VOID CALLBACK ReadCb(DWORD, DWORD, LPOVERLAPPED);

INT WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);

	hInst = hInstance;
	_set_new_handler(&NewHandler);

	HWND hDialog = 0;

	hDialog = CreateDialog(hInst, MAKEINTRESOURCE(IDD_MDRipper_DIALOG), 0, static_cast<DLGPROC>(DialogProc));
	if (!hDialog)
	{
		char buf[100];
		wsprintf(buf, "Error x%x", GetLastError());
		MessageBox(0, buf, "CreateDialog", MB_ICONEXCLAMATION | MB_OK);
		return 1;
	}
	// Attach icon to main dialog
	const HANDLE hbicon = LoadImage(GetModuleHandle(0), MAKEINTRESOURCE(IDR_MAINFRAME),
		IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0);
	// load big icon
	if (hbicon)
		SendMessage(hDialog, WM_SETICON, ICON_BIG, LPARAM(hbicon));

	// load small icon
	const HANDLE hsicon = LoadImage(GetModuleHandle(0), MAKEINTRESOURCE(IDR_MAINFRAME),
		IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
	if (hsicon)
		SendMessage(hDialog, WM_SETICON, ICON_SMALL, LPARAM(hsicon));

	MSG  msg;
	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		// only handle non-dialog messages here
		if (!IsDialogMessage(hDialog, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return (int)msg.wParam;
}

DWORD WINAPI ScanThreadProc(LPVOID lpParam)
{
	ScanContext* ctx = static_cast<ScanContext*>(lpParam);

	Controller* ctrl = ctx->ctrl;
	HANDLE hf = ctx->hFile;

	std::vector<MatchPair>* results = new std::vector<MatchPair>;

	try
	{
		*results = ctrl->m_rRipper.GetICMCandidates(hf);

		if (!results->empty())
		{
			for (int count = 0; count < results->size(); count++) {
				ctrl->m_rRipper.LoadICMFile(hf, (*results)[count].posICM + 8, (*results)[count].posPFM + 8);
			}
		}
	}
	catch (...)
	{
		// Optional: handle/log errors
	}

	CloseHandle(hf);

	PostMessage(ctrl->GetHwnd(), WM_SCAN_COMPLETE, 0, (LPARAM)results);

	delete ctx;
	return 0;
}

Controller::Controller(HWND hwnd) :
	m_hwnd(hwnd),
	m_eOutput(hwnd, IDC_EDIT2, FALSE),
	m_bClose(hwnd, IDOK),
	m_bLoad(hwnd, IDC_LOAD),
	m_rRipper(&m_eOutput)
{
	m_hEdit = GetDlgItem(hwnd, IDC_EDIT2);

	SetWindowSubclass(m_hEdit, EditSubclassProc, 1, (DWORD_PTR)this);

	// set fixed font to make output columns neat
	HFONT hFont = (HFONT)GetStockObject(SYSTEM_FIXED_FONT);
	SendMessage(m_eOutput.Hwnd(), WM_SETFONT, reinterpret_cast<WPARAM>(hFont), MAKELPARAM(FALSE, 0));
}

void Controller::Command(HWND hwnd, int controlID, int command)
{
	UNREFERENCED_PARAMETER(command);
	switch (controlID)
	{
	case IDC_LOAD:
		FileOpen(hwnd);
		break;
	case IDOK:
		PostMessage(hwnd, WM_CLOSE, 0, 0);
		break;
	}
}

void Controller::FileOpen(HWND hwnd)
{
	try
	{

		HANDLE		hf;
		OPENFILENAME ofn;      // common dialog box structure
		char szFile[260]{};      // buffer for file name
		hf = INVALID_HANDLE_VALUE;    // file handle

		// Initialize OPENFILENAME
		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = hwnd;
		ofn.lpstrFile = szFile;
		ofn.lpstrFile[0] = '\0';
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = "All\0*.*\0";
		ofn.nFilterIndex = 1;
		ofn.lpstrFileTitle = NULL;
		ofn.nMaxFileTitle = 0;
		ofn.lpstrInitialDir = NULL;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

		// Display the Open dialog box.
		if (GetOpenFileName(&ofn) == TRUE)
		{
			hf = CreateFile(ofn.lpstrFile,
				GENERIC_READ,
				0,
				(LPSECURITY_ATTRIBUTES)NULL,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				(HANDLE)NULL);
			if (hf == INVALID_HANDLE_VALUE)
			{
				DWORD errorword = GetLastError();
				throw (ErrorClass(SYSTEM_ERROR, "Create File Error", errorword));
			}
			m_busy = true;
			SetClassLongPtr(m_hwnd, GCLP_HCURSOR, (LONG_PTR)LoadCursor(NULL, IDC_WAIT));
			SetCursor(LoadCursor(NULL, IDC_WAIT));
			EnableWindow(GetDlgItem(m_hwnd, IDC_LOAD), FALSE);
			EnableWindow(GetDlgItem(m_hwnd, IDOK), FALSE);
			auto* ctx = new ScanContext;
			ctx->ctrl = this;
			ctx->hFile = hf;

			HANDLE hThread = CreateThread(nullptr, 0, ScanThreadProc, ctx, 0, nullptr);
			if (hThread)
				CloseHandle(hThread);
		}
	}
	catch (ErrorClass error)
	{
		LPVOID lpMsgBuf = NULL;
		char title[255];
		DWORD dwFlags;       // source and processing options
		LPCVOID lpSource;    // pointer to  message source
		DWORD dwMessageId;   // requested message identifier
		strcpy_s(title, error.ErrorTitle);

		switch (error.ErrorType)
		{
		case SYSTEM_ERROR:          // error before file is opened
			dwFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM;
			lpSource = NULL;
			dwMessageId = error.ErrorWord;
			break;
		case APPLICATION_ERROR:
			dwFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING;
			lpSource = error.ErrorString;
			dwMessageId = error.ErrorWord;
			break;
		default:
			dwFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING;
			lpSource = "Unknown Error";
			dwMessageId = error.ErrorWord;
			break;
		}
		FormatMessage(
			dwFlags,        // source and processing options
			lpSource,       // pointer to  message source
			dwMessageId,    // requested message identifier
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR)&lpMsgBuf,
			0,
			NULL);
		// Display the string.
		MessageBox(hwnd, static_cast<char*>(lpMsgBuf), title, MB_OK | MB_ICONEXCLAMATION);
		// Free the buffer.
		LocalFree(lpMsgBuf);
	}
}

void Controller::OnScanComplete(std::vector<MatchPair>* results)
{
	m_busy = false;
	SetClassLongPtr(m_hwnd, GCLP_HCURSOR, (LONG_PTR)LoadCursor(NULL, IDC_ARROW));
	SetCursor(LoadCursor(NULL, IDC_ARROW));
	EnableWindow(GetDlgItem(m_hwnd, IDC_LOAD), TRUE);
	EnableWindow(GetDlgItem(m_hwnd, IDOK), TRUE);

	// You already output inside Ripper, so just clean up
	delete results;
}

INT_PTR CALLBACK DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	static Controller* control = 0;
	switch (message)
	{
	case WM_INITDIALOG:
		try
		{
			control = new Controller(hDlg);
			HMENU hSysMenu;
			hSysMenu = GetSystemMenu(hDlg, FALSE);
			AppendMenu(hSysMenu, MF_SEPARATOR, NULL, NULL); // add a system menu separator
			AppendMenu(hSysMenu, MF_STRING, IDS_ABOUTBOX, appAboutName); // add a system menu item
		}
		catch (WinException e)
		{
			MessageBox(0, e.GetMessage(), "Exception", MB_ICONEXCLAMATION | MB_OK);
		}
		catch (...)
		{
			MessageBox(0, "Unknown", "Exception", MB_ICONEXCLAMATION | MB_OK);
			return -1;
		}
		return TRUE;
	case WM_COMMAND:
		if (control)
			control->Command(hDlg, LOWORD(wParam), HIWORD(wParam));
		return TRUE;
	case WM_DESTROY:
		PostQuitMessage(0);
		return TRUE;
	case WM_CLOSE:
		DestroyWindow(hDlg);
		return TRUE;
	case WM_NCDESTROY:
	{
		if (control)
		{
			if (control->m_hEdit)
			{
				RemoveWindowSubclass(control->m_hEdit, EditSubclassProc, 1);
			}
			delete control;
			SetWindowLongPtr(hDlg, GWLP_USERDATA, 0);
		}
		return TRUE;
	}
	case WM_SYSCOMMAND:
	{
		switch (wParam)
		{
		case IDS_ABOUTBOX:
		{
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hDlg, About);
			break;
		}
		}
		break;
	}
	case WM_SCAN_COMPLETE:
		if (control)
			control->OnScanComplete(reinterpret_cast<std::vector<MatchPair>*>(lParam));
		return TRUE;
	}
	return FALSE;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch (message)
	{
		case WM_INITDIALOG:
			return (INT_PTR)TRUE;
		case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

LRESULT CALLBACK EditSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	Controller* dlg = reinterpret_cast<Controller*>(dwRefData);

	if (msg == WM_SETCURSOR && dlg && dlg->IsBusy())
	{
		SetCursor(LoadCursor(NULL, IDC_WAIT));
		return TRUE;
	}

	return DefSubclassProc(hWnd, msg, wParam, lParam);
}

// ReadFileEx callback function. Does nothing.
VOID CALLBACK ReadCb(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
{
	UNREFERENCED_PARAMETER(dwErrorCode);
	UNREFERENCED_PARAMETER(dwNumberOfBytesTransfered);
	UNREFERENCED_PARAMETER(lpOverlapped);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Standard Windows Dialog Based App stuff above this line, now we get to the important stuff

Ripper::Ripper(Edit* pEdit)
{
	// initializations... for each release, a software part number for the relevent ICM file for each release
	// the data are offsets in the ICM data file for the username and password records (no longer used)
	p_eOutput = pEdit;
	releases[BC8] = versionDATA(string("ICM     8/CAA 111 445/6"), 0x153, 0x418, 0, 0, 0, 0); // bc8 & bc9
	releases[BC10] = versionDATA(string("ICM     9/CAA 111 445/8  "), 0x17f, 0x516, 0x59e, 0x51, 0x08, 0x3c); // bc10
	releases[BC11] = versionDATA(string("ICM     10/CAA 111 445/01"), 0x197, 0x549, 0x5d1, 0x55, 0x0c, 0x40); // bc11, 12 & 13
}

std::vector<MatchPair>  Ripper::GetICMCandidates(HANDLE hFile) {        
    const char patternPFM[8] = { ' ', ' ', ' ', ' ', 'M', ' ', 'P', 'F' };
    const char patternICM[8] = { ' ', ' ', ' ', ' ', 'M', ' ', 'I', 'C' };
    std::vector<MatchPair> results;
    results.reserve(4);

    LARGE_INTEGER fileSize;
    GetFileSizeEx(hFile, &fileSize);
	p_eOutput->SetString("");
	p_eOutput->Enable(FALSE);

    HANDLE hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (!hMap)
    {
        CloseHandle(hFile);
        return results;
    }

    BYTE* data = (BYTE*)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
    if (!data)
    {
        CloseHandle(hMap);
        CloseHandle(hFile);
        return results;
    }

    uint64_t size = fileSize.QuadPart;
    uint64_t activeA = UINT64_MAX;

    for (uint64_t i = 0; i + 8 <= size; ++i)
    {
        // Fast 8-byte compare (7 + 1)
        if (*(uint64_t*)(data + i) == *(uint64_t*)patternPFM &&
            data[i + 7] == patternPFM[7])
        {
            activeA = i;
            continue;
        }

        if (activeA != UINT64_MAX &&
            *(uint64_t*)(data + i) == *(uint64_t*)patternICM &&
            data[i + 7] == patternICM[7])
        {
            results.push_back({ activeA, i });
            activeA = UINT64_MAX;
        }
    }

    UnmapViewOfFile(data);
    CloseHandle(hMap);

    return results;
}


void Ripper::LoadICMFile(HANDLE hf, uint64_t start, uint64_t end)
{
	char TempFileName[MAX_PATH];
	char  temppath[MAX_PATH];
	HANDLE tf = INVALID_HANDLE_VALUE;
	DWORD errorword;
	WORD  buff{};
	BYTE  loop{};

	// get the Temp file path name - default to the current directory
	if (GetTempPath(MAX_PATH, temppath))
		GetTempFileName(temppath, "~lv", 0, TempFileName);
	else
		GetTempFileName(".", "~lv", 0, TempFileName);
	if ((errorword = GetLastError()) != NO_ERROR)
	{
		throw (ErrorClass(SYSTEM_ERROR, "Temp File Name Error", errorword));
	}
	// make the temporary file
	tf = CreateFile(TempFileName,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE,
		NULL);
	if (tf == INVALID_HANDLE_VALUE)
	{
		errorword = GetLastError();
		throw (ErrorClass(SYSTEM_ERROR, "Create Temp File Error", errorword));
	}
	///////////////

	OVERLAPPED bytesread{}, byteswrite{};
	uint64_t currentpoint;

	bytesread.OffsetHigh = 0;
	byteswrite.OffsetHigh = 0xFFFFFFFF;
	byteswrite.Offset = 0xFFFFFFFF;

	for (currentpoint = start - 2; currentpoint > end; currentpoint -= 2)
	{
		bytesread.Offset = currentpoint;
		// read 2 bytes from the end of the file
		if (!ReadFileEx(hf, &buff, 2, &bytesread, NULL))
		{
			errorword = GetLastError();
			throw (ErrorClass(SYSTEM_ERROR, "Read File Error 100", errorword));
		}
		// check for 0000 or ffff. these are compressed in the file by preceding
		// them with the number of consecutive 0000 or ffff.
		if (buff == 0 || buff == 0xffff)
		{
			currentpoint--; // spool back one more byte for the 0000 and ffff count
			bytesread.Offset = currentpoint;

			if (!ReadFileEx(hf, &loop, 1, &bytesread, NULL))
			{
				errorword = GetLastError();
				throw (ErrorClass(SYSTEM_ERROR, "Read File Error 101", errorword));
			}
			for (; loop; loop--)
			{
				// Write to temp file
				if (!WriteFileEx(tf, &buff, 2, &byteswrite, NULL))
				{
					errorword = GetLastError();
					CloseHandle(tf);
					throw (ErrorClass(SYSTEM_ERROR, "Write File Error 101", errorword));
				}
			}
		}
		else
		{
			if (!WriteFileEx(tf, &buff, 2, &byteswrite, NULL))
			{
				errorword = GetLastError();
				CloseHandle(tf);
				throw (ErrorClass(SYSTEM_ERROR, "Write File Error 102", errorword));
			}
		}
	}
	GetPasswordData(tf);
	CloseHandle(tf);
}

void Ripper::GetPasswordData(HANDLE hf)
{
	DWORD errorword;
	OVERLAPPED bytesread{};
	if (!ReadFileEx(hf, &header[0], 40, &bytesread, ReadCb))
	{
		errorword = GetLastError();
		CloseHandle(hf);
		throw (ErrorClass(SYSTEM_ERROR, "Read File Error 200", errorword));
	}
	header[40] = '\0'; // make string
	string banner = "BC8 - BC9";
	// set default to BC8
	versionDATA version = releases[BC8];
	if (string(&header[0]).find(releases[BC8].unitID) != string::npos)
	{
		// BC8 & BC9
		version = releases[BC8];
		banner = "BC8 - BC9";
	}
	else if (string(&header[0]).find(releases[BC10].unitID) != string::npos)
	{
		// BC10
		version = releases[BC10];
		banner = "BC10";
	}
	else if (string(&header[0]).find(releases[BC11].unitID) != string::npos)
	{
		// BC11, BC12 & BC13
		version = releases[BC11];
		banner = "BC11 - BC13";
	}
	BYTE Ccryptvalue = 0;
	char Level7Crypt[(PASSWORDLENGTH + 1)]{};
	char Level7Plain[(PASSWORDLENGTH + 1)]{};

	bytesread.OffsetHigh = 0;
	bytesread.Offset = version.addrPASSWORDREC + (7 * PASSWORDLENGTH);

	if (!ReadFileEx(hf, &Level7Crypt[0], PASSWORDLENGTH, &bytesread, ReadCb))
	{
		errorword = GetLastError();
		CloseHandle(hf);
		throw (ErrorClass(SYSTEM_ERROR, "Read File Error 200", errorword));
	}

	bytesread.Offset = version.addrCCRYPTVALUE;

	if (!ReadFileEx(hf, &Ccryptvalue, 1, &bytesread, ReadCb))
	{
		errorword = GetLastError();
		CloseHandle(hf);
		throw (ErrorClass(SYSTEM_ERROR, "Read File Error 201", errorword));
	}

	BYTE loop;
	loop = Level7Crypt[0];
	if (loop < PASSWORDLENGTH)
	{
		BYTE i;
		for (i = 1; i <= loop; i++)
		{
			Level7Plain[i] = Level7Crypt[i] ^ Ccryptvalue;
		}
		Level7Plain[i] = '\0';
	}
	// Load up the edit control String
	stringstream sstr;
	sstr << "LIM data for " << banner << " found." << "\r\n";
	sstr << version.unitID << "\r\n\r\n";

	sstr << "IPU Level 7 Password-" << "\r\n" << &Level7Plain[1] << "\r\n";
	p_eOutput->AddString(const_cast<char*>(sstr.str().c_str()));
	p_eOutput->Enable(TRUE);
	sstr << "\r\n";
	sstr.str("");    // Clear the content buffer
	sstr.clear();  // Clear error flags (like eofbit, failbit)

	// if BC9 then no account information
	if (version.addrACCOUNTREC != 0)
	{
		sstr << "Accounts-" << "\r\n" << left << setw(24) << "Username:" << setw(24) << "Password:" << "\r\n";

		char Account0User[(ACCOUNTUSERNAMELENGTH + 1)]{};
		char Account0CryptPass[(ACCOUNTPASSWORDLENGTH + 1)]{};
		char Account0PlainPass[(ACCOUNTPASSWORDLENGTH + 1)]{};

		DWORD adrAcount0User = version.addrACCOUNTREC + version.offsetACCOUNTUSER;
		DWORD adrAccount0CryptPass = version.addrACCOUNTREC + version.offsetACCOUNTPASSWORD;
		for (int j = 0; j < 65; j++)
		{
			bytesread.Offset = adrAcount0User + j * version.sizeACCOUNTREC;
			if (!ReadFileEx(hf, &Account0User[0], ACCOUNTUSERNAMELENGTH, &bytesread, ReadCb))
			{
				errorword = GetLastError();
				CloseHandle(hf);
				throw (ErrorClass(SYSTEM_ERROR, "Read File Error 202", errorword));
			}
			if (Account0User[0] == 0)
				break;
			bytesread.Offset = adrAccount0CryptPass + j * version.sizeACCOUNTREC;
			if (!ReadFileEx(hf, &Account0CryptPass[0], ACCOUNTPASSWORDLENGTH, &bytesread, ReadCb))
			{
				errorword = GetLastError();
				CloseHandle(hf);
				throw (ErrorClass(SYSTEM_ERROR, "Read File Error 203", errorword));
			}
			loop = Account0CryptPass[0];
			if (loop < ACCOUNTPASSWORDLENGTH)
			{
				BYTE i;
				for (i = 1; i <= loop; i++)
				{
					Account0PlainPass[i] = Account0CryptPass[i] ^ Ccryptvalue;
				}
				Account0PlainPass[i] = '\0';
			}
			sstr << setw(24) << &Account0User[1] << setw(24) << &Account0PlainPass[1] << "\r\n";
		}
	}
	else
	{
		sstr << "BC8 or BC9; no User Account information exists." << "\r\n";
	}
	// Load up the edit control String
	sstr << "-----------------------------------------------------------" << "\r\n";
	p_eOutput->AddString(const_cast<char*>(sstr.str().c_str()));

}
