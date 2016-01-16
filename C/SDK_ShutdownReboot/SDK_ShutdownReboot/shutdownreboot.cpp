#include <tchar.h>
#include <Windows.h>

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
	return 0;
}