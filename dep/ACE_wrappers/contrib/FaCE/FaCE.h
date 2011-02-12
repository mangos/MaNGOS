// -*- C++ -*-
//=============================================================================
/**
 *  @file    FaCE.h
 *
 *  $Id: FaCE.h 91730 2010-09-13 09:31:11Z johnnyw $
 *
 *  @author Si Mong Park <spark@ociweb.com>
 */
//=============================================================================

#if !defined(AFX_FACE_H__1043241E_A6A9_4246_A9E4_7A774E19EE73__INCLUDED_)
#define AFX_FACE_H__1043241E_A6A9_4246_A9E4_7A774E19EE73__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#if (_WIN32_WCE <= 211)
#error This project can not be built for H/PC Pro 2.11 or earlier platforms.
#endif

#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers

//////
// BEGIN FaCE specific preprocessor
#ifdef NO_ACE

#include <windows.h>
#include "CE_Screen_Output.h"

#define ACE_TCHAR wchar_t
#define ACE_TEXT(STRING) L##STRING
#define ACE_CE_Screen_Output CE_Screen_Output

int main_i(int, wchar_t**);

#else

#include <ace/CE_Screen_Output.h>

int ace_main_i(int, ACE_TCHAR**);

#endif  // NO_ACE
// END FaCE specific
//////

#include "resource.h"

#define MENU_HEIGHT 26
#define MAX_LOADSTRING    101
#define MAX_COMMAND_LINE 1001  // Max number of characters + 1 (null at the end) for user-input argv

extern ACE_CE_Screen_Output cout;  // Replacement of std::cout

#endif // !defined(AFX_FACE_H__1043241E_A6A9_4246_A9E4_7A774E19EE73__INCLUDED_)
