/**
 *  @file    CE_Screen_Output.h
 *
 *  $Id: CE_Screen_Output.h 85385 2009-05-19 10:12:29Z johnnyw $
 *
 *  @author Si Mong Park  <spark@ociweb.com>
 */
//=============================================================================

#ifndef CE_Screen_Output_h
#define CE_Screen_Output_h

#include <windows.h>
#include <stdio.h>

const wchar_t endl[] = L"\r\n";
const wchar_t tab[]  = L"\t";

/**
 * @class CE_Screen_Output
 *
 * @brief Replacement of text output for Windows CE.
 *
 * This class allows standard text output to be displayed on
 * text window for Windows CE.  Generally, all ACE output will
 * go through under CE if and only if user uses WindozeCE
 * implementation by using main_ce instead of main.
 * Also, for the easier debugging purpose, object pointer of
 * this class can be gotten from ACE_Log_Msg::msg_callback()
 * and then can be used directly by user just like cout stream.
 */
class CE_Screen_Output
{
public:
    /**
     * Default Ctor
     */
    CE_Screen_Output();

    /**
     * Default Dtor
     */
    virtual ~CE_Screen_Output();

    /**
     * Interface to specify active window handle.
     */
    void SetOutputWindow(HWND hWnd);

    /**
     * Clears text screen.
     */
    void clear();

    /**
     * << operator that performs actual print out.
     *
     * Note: This is the only one operator that performs
     *       output.  All other perators convert the type and
     *       use this operator underneath.
     */
    CE_Screen_Output& operator << (wchar_t*);
    CE_Screen_Output& operator << (const wchar_t*);

    CE_Screen_Output& operator << (char* output);
    CE_Screen_Output& operator << (const char* output);

    CE_Screen_Output& operator << (char output);
    CE_Screen_Output& operator << (unsigned char output);

    CE_Screen_Output& operator << (unsigned short output);

    CE_Screen_Output& operator << (int output);
    CE_Screen_Output& operator << (unsigned int output);

    CE_Screen_Output& operator << (float output);

    CE_Screen_Output& operator << (long output);
    CE_Screen_Output& operator << (unsigned long output);

    CE_Screen_Output& operator << (FILE* pFile);

private:
    /**
     * Copy Ctor
     */
    CE_Screen_Output(CE_Screen_Output&);

    static HWND handler_;

    /**
     * File pointer that used to save output to file.
     * This class does not own the file handler pointer.
     */
    FILE* pFile_;
};

#endif  // CE_Screen_Output_h
