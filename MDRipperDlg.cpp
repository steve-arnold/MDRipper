#include "stdafx.h"

HINSTANCE hInst = 0;
static char appAboutName[] = "About MDRipper";

int NewHandler(size_t size)
{
	UNREFERENCED_PARAMETER(size);
	throw WinException("Out of memory");
}

// Forward declarations of functions included in this code module:
INT_PTR CALLBACK DialogProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);
VOID CALLBACK ReadCb(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);

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
	return msg.wParam;
}

Controller::Controller(HWND hwnd)
	: m_eOutput(hwnd, IDC_EDIT2, FALSE),
	m_bClose(hwnd, IDOK),
	m_bLoad(hwnd, IDC_LOAD),
	m_rRipper(&m_eOutput)
{
	// set fixed font to make output columns neat
	HGDIOBJ hfDefault = GetStockObject(OEM_FIXED_FONT);
	SendMessage(m_eOutput.Hwnd(), WM_SETFONT, reinterpret_cast<WPARAM>(hfDefault), MAKELPARAM(FALSE, 0));
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
		char szFile[260];      // buffer for file name
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
			m_rRipper.FileLoad(hf);
			CloseHandle(hf);
		}
	}
	catch (ErrorClass error)
	{
		LPVOID lpMsgBuf;
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

INT_PTR CALLBACK DialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	static Controller* control = 0;
	switch (message)
	{
	case WM_INITDIALOG:
		try
		{
			control = new Controller(hwnd);
			HMENU hSysMenu;
			hSysMenu = GetSystemMenu(hwnd, FALSE);
			// add a system menu separator
			AppendMenu(hSysMenu, MF_SEPARATOR, NULL, NULL);
			// add a system menu item
			AppendMenu(hSysMenu, MF_STRING, IDS_ABOUTBOX, appAboutName);
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
			control->Command(hwnd, LOWORD(wParam), HIWORD(wParam));
		return TRUE;
	case WM_DESTROY:
		PostQuitMessage(0);
		return TRUE;
	case WM_CLOSE:
		if (control)
			delete control;
		DestroyWindow(hwnd);
		return TRUE;
	case WM_SYSCOMMAND:
	{
		switch (wParam)
		{
		case IDS_ABOUTBOX:
		{
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hwnd, About);
			break;
		}
		}
		break;
	}
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

// ReadFileEx callback function. Does nothing.
VOID CALLBACK ReadCb(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
{
	UNREFERENCED_PARAMETER(dwErrorCode);
	UNREFERENCED_PARAMETER(dwNumberOfBytesTransfered);
	UNREFERENCED_PARAMETER(lpOverlapped);

}


Ripper::Ripper(Edit* pEdit)
{
	// initializations...
	p_eOutput = pEdit;
	releases[BC8] = versionDATA(string("ICM     8/CAA 111 445/6"), 0x153, 0x418, 0, 0, 0, 0); // bc8 & bc9
	releases[BC10] = versionDATA(string("ICM     9/CAA 111 445/8  "), 0x17f, 0x516, 0x59e, 0x51, 0x08, 0x3c); // bc10
	releases[BC11] = versionDATA(string("ICM     10/CAA 111 445/01"), 0x197, 0x549, 0x5d1, 0x55, 0x0c, 0x40); // bc11, 12 & 13
}

void Ripper::FileLoad(HANDLE hf)
{
	// get relevent data from LIM file
	GetFilePointers(hf);
	GetICM(hf);
}

bool Ripper::GetFilePointers(HANDLE hf)
{
	OVERLAPPED bytesread;
	bool flag = false;
	pointDST = 0;
	point3 = 0xFFFFFFFF;
	if (GetFileSizeEx(hf, &pointEOF))
	{
		// last 4 bytes of the file contain the offset to the start of the Data Structure Table
		// step back 4 bytes from end of file to get point2
		bytesread.OffsetHigh = 0;
		bytesread.Offset = pointEOF.LowPart - 4;
		if (ReadFileEx(hf, &pointDST, 4, &bytesread, ReadCb))
		{
			// step back 4 more bytes from end of file to get point3
			bytesread.Offset = pointEOF.LowPart - 8;
			if (ReadFileEx(hf, &point3, 4, &bytesread, ReadCb))
			{
				flag = true;
				pointDST = ChangeEndian(pointDST);
				point3 = ChangeEndian(point3);
			}
		}
	}
	if (flag == false)
	{
		DWORD errorword = GetLastError();
		throw (ErrorClass(SYSTEM_ERROR, "GetFilePointers File Error", errorword));
	}
	// check pointers: might not be LIM data file
	if (pointEOF.LowPart < point3 || point3 < pointDST)
	{
		throw (ErrorClass(APPLICATION_ERROR, "Application Error", 0, "Pointer errors.\nSelected file is not a LIM data file."));
	}
	return flag;
}

bool Ripper::GetICM(HANDLE hf)
{
	OVERLAPPED bytesread;
	//PU 18 = ICM, PU 19 = PFM. PU 18 ends where PU 19 begins
	DWORD temp, PuICMPointer, PuPFMPointer;
	bool flag = false;

	if (GetFileSizeEx(hf, &pointEOF))
	{
		bytesread.OffsetHigh = 0;
		// get start address for ICM PU number 0x12
		bytesread.Offset = pointEOF.LowPart - pointDST + ICM_ADDRESS;

		if (ReadFileEx(hf, &temp, 4, &bytesread, ReadCb))
		{
			PuICMPointer = pointEOF.LowPart - ChangeEndian(temp);
			// get start address for PU number 0x13 (indicates end pointer for ICM data
			bytesread.Offset = pointEOF.LowPart - pointDST + PFM_ADDRESS;

			if (ReadFileEx(hf, &temp, 4, &bytesread, ReadCb))
			{
				PuPFMPointer = pointEOF.LowPart - ChangeEndian(temp);
				bytesread.Offset = PuICMPointer - 40;
				if (ReadFileEx(hf, &header, 40, &bytesread, ReadCb))
				{
					header[40] = '\0'; // make string
					SetPUName(&header[0]);
					LoadICMFile(hf, PuICMPointer, PuPFMPointer);
					flag = true;
				}
			}
		}
	}
	if (flag == false)
	{
		DWORD errorword = GetLastError();
		throw (ErrorClass(SYSTEM_ERROR, "GetICM File Error", errorword));
	}
	return flag;
}

void Ripper::LoadICMFile(HANDLE hf, DWORD start, DWORD end)
{
	char TempFileName[MAX_PATH];
	char  temppath[MAX_PATH];
	HANDLE tf = INVALID_HANDLE_VALUE;
	DWORD errorword;
	WORD  buff;
	BYTE  loop;

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

	OVERLAPPED bytesread, byteswrite;
	DWORD currentpoint;

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
	// set default to BC11, BC12 & BC13
	versionDATA version = releases[BC11];
	if (string(&header[0]).find(releases[BC8].unitID) != string::npos)
	{
		// BC8 & BC9
		version = releases[BC8];
	}
	else if (string(&header[0]).find(releases[BC10].unitID) != string::npos)
	{
		// BC10
		version = releases[BC10];
	}

	BYTE Ccryptvalue;
	char Level7Crypt[(PASSWORDLENGTH + 1)];
	char Level7Plain[(PASSWORDLENGTH + 1)];

	OVERLAPPED bytesread;
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
	sstr << "IPU Level 7 Password-" << "\r\n" << &Level7Plain[1] << "\r\n";
	p_eOutput->SetString(const_cast<char*>(sstr.str().c_str()));
	p_eOutput->Enable(TRUE);
	sstr << "\r\n";

	// if BC9 then no account information
	if (version.addrACCOUNTREC != 0)
	{
		sstr << "Accounts-" << "\r\n" << left << setw(24) << "Username:" << setw(24) << "Password:" << "\r\n";

		char Account0User[(ACCOUNTUSERNAMELENGTH + 1)];
		char Account0CryptPass[(ACCOUNTPASSWORDLENGTH + 1)];
		char Account0PlainPass[(ACCOUNTPASSWORDLENGTH + 1)];

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
		sstr << "BC8 or BC9; no User Account information available." << "\r\n";
	}
	// Load up the edit control String
	p_eOutput->SetString(const_cast<char*>(sstr.str().c_str()));

}

DWORD Ripper::ChangeEndian(DWORD source)
{
	DWORD dest;

	dest = LOBYTE(LOWORD(source));
	dest = dest << 8;
	dest = dest + HIBYTE(LOWORD(source));
	dest = dest << 8;
	dest = dest + LOBYTE(HIWORD(source));
	dest = dest << 8;
	dest = dest + HIBYTE(HIWORD(source));
	return dest;
}

// straighten up the scrambled program unit name and identifier
void Ripper::SetPUName(char* name)
{
	_strrev(name);  // reverse the string
	int count;
	char temp;
	for (count = 0; count < 40; count += 2)
	{
		temp = name[count];
		name[count] = name[count + 1];
		name[count + 1] = temp;
	}
}
