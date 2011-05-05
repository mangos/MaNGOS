// $Id: FaCE.cpp 85504 2009-06-04 09:41:32Z johnnyw $

#include "FaCE.h"

#ifdef NO_ACE

#include "CE_ARGV.h"

#else

#include <ace/ACE.h>
#include <ace/ARGV.h>
#include <ace/Log_Msg.h>

#endif  // NO_ACE

#include <commctrl.h>
#include <aygshell.h>
#include <sipapi.h>

// This utility does not use ACE, and shouldn't.
//FUZZ: disable check_for_lack_ACE_OS

const ACE_TCHAR* g_ParameterFileName = ACE_TEXT("Parameters.txt");

/**
 * This simple and small class manages user-input command line
 * parameters and parameter history file.
 *
 * @author Si Mong Park (spark@ociweb.com)
 * @version $Revision: 85504 $ $Date: 2009-06-04 11:41:32 +0200 (Thu, 04 Jun 2009) $
 */
class ParameterList
{
public:
    /**
     * Default Ctor.
     */
    ParameterList() : next_(0), param_(0) {};

    /**
     * Dtor: deletes all sub-PameterList objects as well as
     *       memory block allocated for the param_ by _wcsdup().
     */
    ~ParameterList() { free(param_); delete next_; };

    /**
     * Add a new parameter to the list.
     */
    void addParameter(char*);

    /**
     * Add a new parameter to the list.
     */
    void addParameter(ACE_TCHAR*);

    /**
     * Save all parameters stored in the list to the
     * file.
     * Note that 'outputFile' is only for the internal use
     * and user must call this function without any parameter.
     */
    void saveParameter(FILE* outputFile = 0);

    /**
     * Send out windows message to load/update parameters.
     */
    void sendParameterMSG(HWND, UINT);

private:
    /**
     * A pointer to the next ParameterList object.
     * This attribute is totally hidden from user.
     */
    ParameterList* next_;

    /**
     * User-specified command line parameter.
     * This attribute is totally hidden from user.
     */
    ACE_TCHAR* param_;
};


void ParameterList::addParameter(char* newParameter)
{
#ifdef NO_ACE
    int len = MultiByteToWideChar(CP_OEMCP, MB_PRECOMPOSED, newParameter, -1, 0, 0);
    wchar_t* w_output = new wchar_t[len];

    MultiByteToWideChar(CP_OEMCP, MB_PRECOMPOSED, newParameter, -1, w_output, len);
    this->addParameter(w_output);

    delete w_output;
#else
    this->addParameter(ACE_TEXT_CHAR_TO_TCHAR(newParameter));
#endif  // NO_ACE
}


void ParameterList::addParameter(ACE_TCHAR* newParameter)
{
    if (this->param_ == 0) {
        this->param_ = _wcsdup(newParameter);
        this->next_ = new ParameterList();  // create and add a new ParameterList object
    }
    else {
        if (wcscmp(this->param_, newParameter) != 0) {
            this->next_->addParameter(newParameter);
        }
    }
}


void ParameterList::saveParameter(FILE* outputFile)
{
    if ( (outputFile == 0) && (this->param_ != 0) ) {
        outputFile = _wfopen(g_ParameterFileName, ACE_TEXT("w+"));
    }

    if (outputFile != 0) {
        if (this->param_ != 0) {
            fwprintf(outputFile, ACE_TEXT("%s\n"), this->param_);
            this->next_->saveParameter(outputFile);
        }
        else {
            fclose(outputFile);
        }
    }
}


void ParameterList::sendParameterMSG(HWND hDlg, UINT message)
{
    if (param_ != 0) {
        SendDlgItemMessage(hDlg, IDC_CMDEDIT, message, 0, (LPARAM)this->param_);
        this->next_->sendParameterMSG(hDlg, message);
    }
}


// Global Variables:
HINSTANCE g_hInst;          // The current instance
HWND      g_hwndCB;         // The command bar handle
HWND      hWndEdit;         // Read only edit box for output display
FILE*     g_OutputFile;     // File handler for output save

ParameterList g_Parameter;  // command line parameter list

ACE_CE_Screen_Output cout;  // Replacement of std::cout

ACE_TCHAR g_CommandLine[MAX_COMMAND_LINE]; // User-specified command line parameter
ACE_TCHAR g_SaveFileName[MAX_LOADSTRING];      // Name of the output file

static SHACTIVATEINFO s_sai;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass (HINSTANCE, ACE_TCHAR*);
BOOL                InitInstance    (HINSTANCE, int);
LRESULT CALLBACK    WndProc         (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    About           (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    CommandLine     (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    SaveFileName    (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    FileError       (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    FileExist       (HWND, UINT, WPARAM, LPARAM);
HWND                CreateRpCommandBar(HWND);


void InitSetup()
{
    g_OutputFile    = 0;
    memset(g_CommandLine,  0, MAX_COMMAND_LINE * sizeof(ACE_TCHAR));
    memset(g_SaveFileName, 0, MAX_LOADSTRING   * sizeof(ACE_TCHAR));
}


void LoadParameterHistory()
{
    FILE* parameterFile = _wfopen(g_ParameterFileName, ACE_TEXT("r"));

    if (parameterFile != 0) {
        while (feof(parameterFile) == 0) {
            // Note: Remember that fwprintf takes wide-character format specifier but
            // save string as ASCII.  Thus, history must be read as ASCII then converted
            // to wide-character (Unicode on WinCE).
            char singleParameter[MAX_COMMAND_LINE];
            int size = 0;
            fread(&singleParameter[size], sizeof(char), 1, parameterFile);

            // WinCE does not have function that reads upto the end of line.
            while (singleParameter[size] != '\n') {
                fread(&singleParameter[++size], sizeof(char), 1, parameterFile);
            }

            if (size > 0) {
                singleParameter[size] = 0;  // NULL terminator
                g_Parameter.addParameter(singleParameter);
            }
        }
        fclose(parameterFile);
    }
}


//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    It is important to call this function so that the application
//    will get 'well formed' small icons associated with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance, ACE_TCHAR* szWindowClass)
{
    WNDCLASS    wc;

    wc.style            = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc      = (WNDPROC) WndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = hInstance;
    wc.hIcon            = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_FACE));
    wc.hCursor          = 0;
    wc.hbrBackground    = (HBRUSH) GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName     = 0;
    wc.lpszClassName    = szWindowClass;

    return RegisterClass(&wc);
}

//
//  FUNCTION: InitInstance(HANDLE, int)
//
//  PURPOSE: Saves instance handle and creates main window
//
//  COMMENTS:
//
//    In this function, we save the instance handle in a global variable and
//    create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    HWND hWnd = 0;

    ACE_TCHAR szTitle[MAX_LOADSTRING];            // The title bar text
    ACE_TCHAR szWindowClass[MAX_LOADSTRING];      // The window class name

    g_hInst = hInstance;        // Store instance handle in our global variable
    // Initialize global strings
    LoadString(hInstance, IDC_FACE, szWindowClass, MAX_LOADSTRING);
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);

    //If it is already running, then focus on the window
    hWnd = FindWindow(szWindowClass, szTitle);
    if (hWnd)
    {
        // set focus to foremost child window
        // The "| 0x01" is used to bring any owned windows to the foreground and
        // activate them.
        SetForegroundWindow((HWND)((ULONG) hWnd | 0x00000001));
        return 0;
    }

    MyRegisterClass(hInstance, szWindowClass);

    RECT    rect;
    GetClientRect(hWnd, &rect);

    hWnd = CreateWindow(szWindowClass, szTitle, WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, hInstance, 0);

    if (!hWnd)
    {
        int error = 0;
        error = GetLastError();
        return FALSE;
    }
    //When the main window is created using CW_USEDEFAULT the height of the menubar (if one
    // is created is not taken into account). So we resize the window after creating it
    // if a menubar is present
    {
        RECT rc;
        GetWindowRect(hWnd, &rc);
        rc.bottom -= MENU_HEIGHT;
        if (g_hwndCB)
            MoveWindow(hWnd, rc.left, rc.top, rc.right, rc.bottom, FALSE);
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    int wmId, wmEvent, nCmdHt;
    PAINTSTRUCT ps;
    RECT textRect;

    switch (message)
    {
        case WM_COMMAND:
            wmId    = LOWORD(wParam);
            wmEvent = HIWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
                case IDM_HELP_ABOUT:
                    DialogBox(g_hInst, (const ACE_TCHAR*)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
                    break;

                case IDOK:
                    SendMessage(hWnd, WM_ACTIVATE, MAKEWPARAM(WA_INACTIVE, 0), (LPARAM)hWnd);
                    SendMessage(hWnd, WM_CLOSE, 0, 0);
                    break;

                case ID_SETTING_RUN:
                    {
#ifdef NO_ACE
                        cout << ACE_TEXT("START with command line: ") << g_CommandLine << endl;
                        CE_ARGV ce_argv(g_CommandLine);
                        main_i(ce_argv.argc(), ce_argv.argv());
                        cout << ACE_TEXT("END") << endl << endl;
#else
                        cout << ACE_TEXT("START with command line: ") << g_CommandLine << endl;
                        ACE_ARGV ce_argv(g_CommandLine);
                        ACE::init();
                        ACE_LOG_MSG->msg_callback(&cout);  // register call back
                        ACE_LOG_MSG->set_flags(ACE_Log_Msg::MSG_CALLBACK);  // set call back flag
                        ace_main_i(ce_argv.argc(), ce_argv.argv());
                        ACE::fini();
                        cout << ACE_TEXT("END") << endl << endl;
#endif  // NO_ACE
                    }
                    break;

                case ID_SETTING_EXIT:
                    SendMessage(hWnd, WM_DESTROY, 0, 0);
                    break;

                case ID_TOOLS_SAVETOFILE:
                    // create a dialog box to get the file name
                    DialogBox(g_hInst, (const ACE_TCHAR*)IDD_OUTFILE, hWnd, (DLGPROC)SaveFileName);
                    break;

                case ID_SETTING_COMMANDLINE:
                    // create a dialog box to get the command line
                    DialogBox(g_hInst, (const ACE_TCHAR*)IDD_CMDLINE, hWnd, (DLGPROC)CommandLine);
                    break;

                default:
                   return DefWindowProc(hWnd, message, wParam, lParam);
            }
            break;

        case WM_CREATE:
            SHMENUBARINFO mbi;

            memset(&mbi, 0, sizeof(SHMENUBARINFO));
            mbi.cbSize     = sizeof(SHMENUBARINFO);
            mbi.hwndParent = hWnd;
            mbi.nToolBarId = IDM_MENU;
            mbi.hInstRes   = g_hInst;
            mbi.nBmpId     = 0;
            mbi.cBmpImages = 0;

            if (!SHCreateMenuBar(&mbi))
                return 0;

            g_hwndCB = mbi.hwndMB;

            // Initialize the shell activate info structure
            memset (&s_sai, 0, sizeof (s_sai));
            s_sai.cbSize = sizeof (s_sai);

            GetClientRect(hWnd, &textRect);
            nCmdHt = CommandBar_Height(mbi.hwndMB);

            hWndEdit = CreateWindow(ACE_TEXT("EDIT"),
                                    0,
                                    WS_CHILD | WS_VISIBLE | ES_READONLY | ES_MULTILINE | WS_VSCROLL | WS_HSCROLL,
                                    0,
                                    0,
                                    textRect.right,
                                    textRect.bottom - MENU_HEIGHT,
                                    hWnd,
                                    0,
                                    g_hInst,
                                    0);
            cout.SetOutputWindow(hWndEdit);
            LoadParameterHistory();
            break;

        case WM_PAINT:
            RECT rt;
            hdc = BeginPaint(hWnd, &ps);
            GetClientRect(hWnd, &rt);
            EndPaint(hWnd, &ps);
            break;

        case WM_ACTIVATE:
            // Notify shell of our activate message
            SHHandleWMActivate(hWnd, wParam, lParam, &s_sai, FALSE);
            break;

        case WM_SETTINGCHANGE:
            SHHandleWMSettingChange(hWnd, wParam, lParam, &s_sai);
            break;

        case WM_HIBERNATE:  // low power
        case WM_CLOSE:
        case WM_DESTROY:
            g_Parameter.saveParameter();  // save parameters to history file
            CommandBar_Destroy(g_hwndCB);
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}


HWND CreateRpCommandBar(HWND hwnd)
{
    SHMENUBARINFO mbi;

    memset(&mbi, 0, sizeof(SHMENUBARINFO));
    mbi.cbSize     = sizeof(SHMENUBARINFO);
    mbi.hwndParent = hwnd;
    mbi.nToolBarId = IDM_MENU;
    mbi.hInstRes   = g_hInst;
    mbi.nBmpId     = 0;
    mbi.cBmpImages = 0;

    if (!SHCreateMenuBar(&mbi))
        return 0;

    return mbi.hwndMB;
}

// Mesage handler for the About box.
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM)
{
    SHINITDLGINFO shidi;

    const ACE_TCHAR* copyrightNote =
ACE_TEXT("ACE and TAO are copyrighted by Dr. Douglas C. Schmidt and Center for Distributed Object") \
ACE_TEXT("Computing at Washington University, 1993-2002, all rights reserved.")  \
ACE_TEXT("FaCE is copyrighted by Object Computing, Inc., 2002,\n all rights reserved.\n") \
ACE_TEXT("See License.txt for more information.");

    switch (message)
    {
        case WM_INITDIALOG:
            // Create a Done button and size it.
            shidi.dwMask = SHIDIM_FLAGS;
            shidi.dwFlags = SHIDIF_DONEBUTTON | SHIDIF_SIPDOWN | SHIDIF_SIZEDLGFULLSCREEN;
            shidi.hDlg = hDlg;
            SHInitDialog(&shidi);
            SetDlgItemText(hDlg, IDC_COPYRIGHT, copyrightNote);
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK)
            {
                EndDialog(hDlg, LOWORD(wParam));
                return TRUE;
            }
            break;
    }
    return FALSE;
}


LRESULT CALLBACK CommandLine(HWND hDlg, UINT message, WPARAM wParam, LPARAM)
{
    int wmId;
    int wmEvent;

    switch (message)
    {
    case WM_INITDIALOG:
        g_Parameter.sendParameterMSG(hDlg, CB_INSERTSTRING);
        SetDlgItemText(hDlg, IDC_CMDEDIT, g_CommandLine);  // pass existing command line for display
        return TRUE;

    case WM_COMMAND:
        wmId    = LOWORD(wParam);
        wmEvent = HIWORD(wParam);
        // Parse the menu selections:
        switch (wmId)
        {
        case IDOK:
            // new command line accepted
            GetDlgItemText(hDlg, IDC_CMDEDIT, g_CommandLine, MAX_COMMAND_LINE - 1);
            EndDialog(hDlg, wmId);
            g_Parameter.addParameter(g_CommandLine);
            return TRUE;

        case IDCANCEL:
            EndDialog(hDlg, wmId);
            return TRUE;

        default:
            return FALSE;
        }
        break;
        default:
            return FALSE;
    }

    return FALSE;
}


LRESULT CALLBACK SaveFileName(HWND hDlg, UINT message, WPARAM wParam, LPARAM)
{
    int wmId;
    int wmEvent;

    ACE_TCHAR tempBuffer[MAX_LOADSTRING];
    ACE_TCHAR fileMode[3] = { 0, '+', 0 };  // mode will either be "a+" or "w+"
    FILE* tempFile;

    switch (message)
    {
    case WM_INITDIALOG:
        SetDlgItemText(hDlg, IDC_SAVEFILE, g_SaveFileName);
        return TRUE;

    case WM_COMMAND:
        wmId    = LOWORD(wParam);
        wmEvent = HIWORD(wParam);
        // Parse the menu selections:
        switch (wmId)
        {
        case IDOK:
            GetDlgItemText(hDlg, IDC_SAVEFILE, tempBuffer, MAX_LOADSTRING - 1);
            EndDialog(hDlg, wmId);

            tempFile = _wfopen(tempBuffer, ACE_TEXT("r"));

            if (tempFile != 0)  // if file exists
            {
                fclose(tempFile);  // close temp handler
                int choice = DialogBox(g_hInst, (const ACE_TCHAR*)IDD_FILEEXIST, hDlg, (DLGPROC)FileExist);
                switch (choice)
                {
                case IDOVERWRITE:  // overwrite existing file
                    fileMode[0] = 'w';
                    break;

                case IDC_APPEND:  // append to existing file
                    fileMode[0] = 'a';
                    break;

                case IDCANCEL:  // cancel operation without changing g_OutputFile
                    return TRUE;
                }
            }
            else  // if file does not exist
            {
                fileMode[0] = 'w';
            }

            tempFile = _wfopen(tempBuffer, fileMode);

            if (tempFile == 0)
            {
                DialogBox(g_hInst, (const ACE_TCHAR*)IDD_ERRFILE, hDlg, (DLGPROC)FileError);
            }
            else
            {
                wcscpy(g_SaveFileName, tempBuffer);

                if (g_OutputFile != 0)
                {
                    fclose(g_OutputFile);  // close any open file
                }

                g_OutputFile = tempFile;

                cout << g_OutputFile;  // update FILE* for the CE_Screen_Output class object.
            }

            return TRUE;

        case IDCANCEL:
            EndDialog(hDlg, wmId);
            return TRUE;

        default:
            return FALSE;
        }
        break;
        default:
            return FALSE;
    }

    return FALSE;
}


LRESULT CALLBACK FileError(HWND hDlg, UINT message, WPARAM wParam, LPARAM)
{
    switch (message)
    {
        case WM_INITDIALOG:
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK)
            {
                EndDialog(hDlg, LOWORD(wParam));
                return TRUE;
            }
            break;
    }

    return FALSE;
}


LRESULT CALLBACK FileExist(HWND hDlg, UINT message, WPARAM wParam, LPARAM)
{
    switch (message)
    {
    case WM_INITDIALOG:
        return TRUE;
    case WM_COMMAND:
        EndDialog(hDlg, LOWORD(wParam));
        return TRUE;
    default:
        return FALSE;
    }

    return FALSE;
}

//FUZZ: enable check_for_lack_ACE_OS
