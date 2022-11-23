#pragma once

#include <windows.h>
#include "resource.h"

class SimpleControl
{
public:
	SimpleControl(HWND hwndParent, int id, BOOL initialState = TRUE)
		: hwnd(GetDlgItem(hwndParent, id))
	{
		Enable(initialState);
	}
	void SetFocus()
	{
		::SetFocus(hwnd);
	}
	void Show(BOOL state)
	{
		int show = state ? SW_SHOW : SW_HIDE;
		::ShowWindow(hwnd, show);
	}
	void Enable(BOOL state)
	{
		::EnableWindow(hwnd, state);
	}
	BOOL IsVisible()
	{
		return (::IsWindowVisible(hwnd));
	}
	HWND Hwnd() const
	{
		return hwnd;
	}
protected:
	HWND hwnd;
};

class Button : public SimpleControl
{
public:
	Button(HWND hwndParent, int id, BOOL initialState = TRUE)
		: SimpleControl(hwndParent, id, initialState)
	{}
	void SetName(char const* newName)
	{
		SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)newName);
	}
};

class CheckBox : public Button
{
public:
	CheckBox(HWND hwndParent, int id, BOOL initialState = TRUE)
		: Button(hwndParent, id, initialState)
	{}
	BOOL IsChecked()
	{
		return (SendMessage(hwnd, BM_GETCHECK, 0, 0) == BST_CHECKED);
	}
	void Check()
	{
		SendMessage(hwnd, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
	}
	void UnCheck()
	{
		SendMessage(hwnd, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
	}
};

class RadioButton : public Button
{
public:
	RadioButton(HWND hwndParent, int id, BOOL initialState = TRUE)
		: Button(hwndParent, id, initialState)
	{}
	BOOL IsSelected()
	{
		return (SendMessage(hwnd, BM_GETCHECK, 0, 0) == BST_CHECKED);
	}
	void Select()
	{
		SendMessage(hwnd, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
	}
};

class Edit : public SimpleControl
{
public:
	Edit(HWND hwndParent, int id, BOOL initialState = TRUE)
		: SimpleControl(hwndParent, id, initialState)
	{}
	void SetFont(HGDIOBJ hf)
	{
		SendMessage(hwnd, WM_SETFONT, (WPARAM)hf, MAKELPARAM(FALSE, 0));
	}
	void SetString(char* buf)
	{
		SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)buf);
	}
	void AddString(char* buf)
	{
		int ndx = GetLength();
		SetFocus();
		SendMessage(hwnd, EM_SETSEL, (WPARAM)ndx, (LPARAM)ndx);
		SendMessage(hwnd, EM_REPLACESEL, 0, (LPARAM)((LPSTR)buf));
	}
	// code is the HIWORD(wParam)
	static BOOL IsChanged(int code)
	{
		return code == EN_CHANGE;
	}
	int GetLength()
	{
		return (int)(SendMessage(hwnd, WM_GETTEXTLENGTH, 0, 0));
	}
	void GetString(char* buf, int len)
	{
		SendMessage(hwnd, WM_GETTEXT, (WPARAM)len, (LPARAM)buf);
	}
	void Select()
	{
		SendMessage(hwnd, EM_SETSEL, (WPARAM)0, (LPARAM)-1);
	}
	void ClearSelection()
	{
		SendMessage(hwnd, EM_SETSEL, (WPARAM)-1, (LPARAM)0);
	}
	void ScrollEnd()
	{
		SendMessage(hwnd, EM_LINESCROLL, (WPARAM)0, (LPARAM)100);
	}

};
