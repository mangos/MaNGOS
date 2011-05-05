// $Id: CE_ARGV.cpp 85395 2009-05-19 10:22:42Z johnnyw $

#include "CE_ARGV.h"

CE_ARGV::CE_ARGV(wchar_t* cmdLine)
: ce_argv_(0)
, ce_argc_(0)
{
    const wchar_t* dummyArgv = L"root";  // dummy for the first argv
    const wchar_t* separator = L" ";     // blank space is a separator

    int formattedCmdLineLength = wcslen(dummyArgv) +
                                 wcslen(separator) +
                                 1;  // 1 is for the NULL at the end

    if (wcslen(cmdLine) > 0) {
        formattedCmdLineLength += wcslen(cmdLine);
        formattedCmdLineLength += wcslen(separator);
    }

    // formattedCmdLine will have dummyArgv and a separator at the beginning of cmdLine
    // and a separator at the end to generalize format and reduce the amount of code
    wchar_t* formattedCmdLine = 0;
    formattedCmdLine = new wchar_t[formattedCmdLineLength];

    wcscpy(formattedCmdLine, dummyArgv);
    wcscat(formattedCmdLine, separator);

    int max_possible_argc = 1;  // start with 1 because of the dummyArgv at the beginning

    if (wcslen(cmdLine) > 0) {
        int formattedPos  = wcslen(formattedCmdLine);
        int cmdLineLength = wcslen(cmdLine);

        // Inside of this for loop, it does same thing as strcat except it
        // checks and puts only one single white space between two argv entries.
        for (int i = 0; i < cmdLineLength; ++i) {
            if (iswspace(cmdLine[i]) != 0) {
                ++max_possible_argc;  // counting the number of white spaces
            }

            formattedCmdLine[formattedPos++] = cmdLine[i];

            if (iswspace(cmdLine[i]) != 0) {
                // make sure there is only one white space between two argv entries.
                while ((i < cmdLineLength) && (iswspace(cmdLine[i + 1]) != 0)) {
                    ++i;
                }
            }
        }

        formattedCmdLine[formattedPos] = 0;
        wcscat(formattedCmdLine, separator);  // make sure formattedCmdLine ends with a blank
    }

    int formattedCmdLength = wcslen(formattedCmdLine);

    bool insideQuotation = false;
    int* argv_strlen = 0;
    int entry_size = 0;
    argv_strlen = new int[max_possible_argc];

    // determine argc
    for (int i = 0; i < formattedCmdLength; ++i) {
        if (formattedCmdLine[i] == '\\') {
            ++i; // ignore the following character
            ++entry_size;
        }
        else if (formattedCmdLine[i] == '"') {
            insideQuotation = !insideQuotation;
        }
        else if ((!insideQuotation) && (iswspace(formattedCmdLine[i]) != 0)) {
            // new argv entry end found
            argv_strlen[ce_argc_++] = entry_size;  // cache the size of this entry
            entry_size = 0;
        }
        else {
            ++entry_size;
        }
    }

    ce_argv_ = new wchar_t*[ce_argc_ + 1];
    ce_argv_[ce_argc_] = 0;  // Last command line entry is a NULL.

    for (int j = 0, cmdLinePos = 0; j < ce_argc_; ++j, ++cmdLinePos) {
        int length = argv_strlen[j];

        ce_argv_[j] = new wchar_t[length + 1];
        ce_argv_[j][length] = 0;  // string termination null

        if (iswspace(formattedCmdLine[cmdLinePos]) != 0) {
            // This is where prior argv has trailing '"' at the end.
            ++cmdLinePos;
        }

        for (int n = 0; n < length; ++n, ++cmdLinePos) {
            if ((formattedCmdLine[cmdLinePos] == '\\') || (formattedCmdLine[cmdLinePos] == '"')) {
                ++cmdLinePos;
            }

            ce_argv_[j][n] = formattedCmdLine[cmdLinePos];
        }
    }

    delete argv_strlen;
    delete formattedCmdLine;
}


CE_ARGV::~CE_ARGV(void)
{
    for (int i = 0; i < ce_argc_; ++i) {
        delete [] ce_argv_[i];
    }

    delete [] ce_argv_;
}
