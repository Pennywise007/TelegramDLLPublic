// TelegramDLL.h : main header file for the TelegramDLL DLL
//

#pragma once

#ifndef __AFXWIN_H__
    #error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CTelegramDLLApp
// See TelegramDLL.cpp for the implementation of this class
//

class CTelegramDLLApp : public CWinApp
{
public:
    CTelegramDLLApp();

// Overrides
public:
    virtual BOOL InitInstance();

    DECLARE_MESSAGE_MAP()
};
