#include <tchar.h>
#include <Windows.h>
#include "resource.h"

//////////////////提升当前用户权限//////////////////
BOOL EnableShutDownPriv(){
	HANDLE hToken = NULL;
	TOKEN_PRIVILEGES tkp = { 0 };

	if (!OpenProcessToken(GetCurrentProcess(),
		TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
		&hToken)) {
		return FALSE;
	}
	if (!LookupPrivilegeValue(NULL,
		SE_SHUTDOWN_NAME,             //取得关机权限
		&tkp.
		Privileges[0].Luid)) {
		CloseHandle(hToken);
		return FALSE;
	}
	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if (!AdjustTokenPrivileges(hToken,
		FALSE,
		&tkp,
		sizeof(TOKEN_PRIVILEGES),
		NULL,
		NULL)) {
		CloseHandle(hToken);
		return FALSE;
	}
	return TRUE;
}
BOOL ResetWindows(DWORD dwFlags, BOOL bForce){
	//Check the params
	if (dwFlags != EWX_LOGOFF && dwFlags != EWX_REBOOT && dwFlags != EWX_SHUTDOWN){
		return FALSE;
	}

	//Get the os version
	OSVERSIONINFO osvi = { 0 };
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (!GetVersionEx(&osvi)){
		return FALSE;
	}
	if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT){
		EnableShutDownPriv();//NT架构以上系统需要提升当前用户权限才可以关机重启
	}

	dwFlags |= (bForce != FALSE) ? EWX_FORCE : EWX_FORCEIFHUNG;

	return ExitWindowsEx(dwFlags, 0);
}

INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam){
	switch (uMsg){
	case WM_INITDIALOG://对话框初始化时发送的初始化消息
	{
		HWND hCombo = GetDlgItem(hwndDlg, IDC_COMBO);
		//初始化combo box
		SendMessage(hCombo, CB_INSERTSTRING, 0, (LPARAM)_T("Logout"));
		SendMessage(hCombo, CB_INSERTSTRING, 1, (LPARAM)_T("Restart"));
		SendMessage(hCombo, CB_INSERTSTRING, 2, (LPARAM)_T("Shutdown"));
		//
		SetWindowPos(hwndDlg, HWND_TOP, 200, 200, 400, 300, SWP_SHOWWINDOW);
	}
	break;
	case WM_COMMAND:
	{
		switch (wParam)
		{
		case IDOK:
		{
			TCHAR comboText[20] = { 0 };
			GetDlgItemText(hwndDlg, IDC_COMBO, comboText, 20);
			if (0 == _tcscmp(comboText, _T("Logout"))){
				ResetWindows(EWX_LOGOFF, FALSE);
			}
			else if (0 == _tcscmp(comboText, _T("Restart"))){
				ResetWindows(EWX_REBOOT, FALSE);
			}
			else if (0 == _tcscmp(comboText, _T("Shutdown"))){
				int ret = MessageBox(hwndDlg, _T("Sure to shutdown ?"), _T("Info"), MB_OKCANCEL);

				if (IDOK == ret){
					EndDialog(hwndDlg, IDOK);
					ResetWindows(EWX_SHUTDOWN, FALSE);
				}
			}
		}
			break;
		case IDCANCEL:
		{
			int ret = MessageBox(hwndDlg, _T("Quit ?"), _T("Info"), MB_OKCANCEL);

			if (IDOK == ret){
				EndDialog(hwndDlg, IDOK);
			}
		}
		break;
		default:
			break;
		}
	}
	break;

	default:
		break;
	}
	return 0;
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPTSTR lpCmdLine,
	int nCmdShow){
	if (_tcscmp(lpCmdLine, _T("/r")) == 0) {
		ResetWindows(EWX_REBOOT, FALSE);
	}
	else if (_tcscmp(lpCmdLine, _T("/s")) == 0) {
		ResetWindows(EWX_SHUTDOWN, FALSE);
	}
	else if (_tcscmp(lpCmdLine, _T("/l")) == 0) {
		ResetWindows(EWX_LOGOFF, FALSE);
	}
	else{
		DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN_DIALOG), NULL, DialogProc);
	}
	return 0;
}
