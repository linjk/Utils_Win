
// LANChat.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CLANChatApp: 
// �йش����ʵ�֣������ LANChat.cpp
//

class CLANChatApp : public CWinApp
{
public:
	CLANChatApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CLANChatApp theApp;