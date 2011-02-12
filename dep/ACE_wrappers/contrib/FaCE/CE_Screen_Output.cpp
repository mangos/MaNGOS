// $Id: CE_Screen_Output.cpp 80826 2008-03-04 14:51:23Z wotte $

#include "CE_Screen_Output.h"
#include <string.h>

// This utility does not use ACE, and shouldn't.
//FUZZ: disable check_for_lack_ACE_OS

HWND CE_Screen_Output::handler_ = 0;


CE_Screen_Output::CE_Screen_Output()
: pFile_(0)
{
}


CE_Screen_Output::~CE_Screen_Output()
{
    if (pFile_ != 0) {
        fclose(pFile_);
    }
}


void CE_Screen_Output::SetOutputWindow(HWND hEdit)
{
    handler_ = hEdit;
}


void CE_Screen_Output::clear()
{
    SetWindowText(handler_, 0);
}


CE_Screen_Output& CE_Screen_Output::operator << (wchar_t* output)
{
    int length = GetWindowTextLength(handler_);
    SendMessage(handler_, EM_SETSEL, length, length);
    SendMessage(handler_, EM_REPLACESEL, 0, (LPARAM)output);

    if (pFile_ != 0)
    {
        fwprintf(pFile_, L"%s", output);
    }

    return *this;
}


CE_Screen_Output& CE_Screen_Output::operator << (const wchar_t* output)
{
    wchar_t* buffer = _wcsdup(output);
    if (buffer != 0)
    {
        *this << buffer;
        delete buffer;
    }
    return *this;
}


CE_Screen_Output& CE_Screen_Output::operator << (char* output)
{
    int len = MultiByteToWideChar(CP_OEMCP, MB_PRECOMPOSED, output, -1, 0, 0);
    wchar_t* w_output = new wchar_t[len];

    MultiByteToWideChar(CP_OEMCP, MB_PRECOMPOSED, output, -1, w_output, len);
    *this << w_output;

    delete w_output;

    return *this;
}


CE_Screen_Output& CE_Screen_Output::operator << (const char* output)
{
    int len = MultiByteToWideChar(CP_OEMCP, MB_PRECOMPOSED, output, -1, 0, 0);
    wchar_t* w_output = new wchar_t[len];

    MultiByteToWideChar(CP_OEMCP, MB_PRECOMPOSED, output, -1, w_output, len);
    *this << w_output;

    delete w_output;
    return *this;
}


CE_Screen_Output& CE_Screen_Output::operator << (char output)
{
    *this << (int)output;
    return *this;
}


CE_Screen_Output& CE_Screen_Output::operator << (unsigned char output)
{
    *this << (int)output;
    return *this;
}


CE_Screen_Output& CE_Screen_Output::operator << (unsigned short output)
{
  wchar_t buffer[20];
  wsprintf(buffer, L"%u", output);
  *this << buffer;
  return *this;
}


CE_Screen_Output& CE_Screen_Output::operator << (int output)
{
    wchar_t buffer[20];
    wsprintf(buffer, L"%d", output);
    *this << buffer;
    return *this;
}


CE_Screen_Output& CE_Screen_Output::operator << (unsigned int output)
{
    wchar_t buffer[20];
    wsprintf(buffer, L"%du", output);
    *this << buffer;
    return *this;
}


CE_Screen_Output& CE_Screen_Output::operator << (float output)
{
    wchar_t buffer[20];
    swprintf(buffer, L"%f", output);
    *this << buffer;
    return *this;
}


CE_Screen_Output& CE_Screen_Output::operator << (long output)
{
    wchar_t buffer[20];
    wsprintf(buffer, L"%l", output);
    *this << buffer;
    return *this;
}


CE_Screen_Output& CE_Screen_Output::operator << (unsigned long output)
{
    wchar_t buffer[20];
    wsprintf(buffer, L"%lu", output);
    *this << buffer;
    return *this;
}


CE_Screen_Output& CE_Screen_Output::operator << (FILE* pFile)
{
    pFile_ = pFile;
    return *this;
}

//FUZZ: enable check_for_lack_ACE_OS
