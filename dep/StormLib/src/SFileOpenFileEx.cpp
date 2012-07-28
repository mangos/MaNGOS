/*****************************************************************************/
/* SFileOpenFileEx.cpp                    Copyright (c) Ladislav Zezula 2003 */
/*---------------------------------------------------------------------------*/
/* Description :                                                             */
/*---------------------------------------------------------------------------*/
/*   Date    Ver   Who  Comment                                              */
/* --------  ----  ---  -------                                              */
/* xx.xx.99  1.00  Lad  The first version of SFileOpenFileEx.cpp             */
/*****************************************************************************/

#define __STORMLIB_SELF__
#include "StormLib.h"
#include "StormCommon.h"

/*****************************************************************************/
/* Local functions                                                           */
/*****************************************************************************/

static bool OpenLocalFile(const char * szFileName, HANDLE * phFile)
{
    TFileStream * pStream;
    TMPQFile * hf = NULL;
    
    pStream = FileStream_OpenFile(szFileName, false);
    if(pStream != NULL)
    {
        // Allocate and initialize file handle
        hf = CreateMpqFile(NULL);
        if(hf != NULL)
        {
            hf->pStream = pStream;
            *phFile = hf;
            return true;
        }
        else
        {
            FileStream_Close(pStream);
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        }
    }
    *phFile = NULL;
    return false;
}

bool OpenPatchedFile(HANDLE hMpq, const char * szFileName, DWORD dwReserved, HANDLE * phFile)
{
    TMPQArchive * ha = (TMPQArchive *)hMpq;
    TMPQFile * hfPatch;                     // Pointer to patch file
    TMPQFile * hfBase = NULL;               // Pointer to base open file
    TMPQFile * hfLast;                      // The highest file in the chain that is not patch file
    TMPQFile * hf;
    HANDLE hPatchFile;
    char szPatchFileName[MAX_PATH];

    // Keep this flag here for future updates
    dwReserved = dwReserved;

    // First of all, try to open the original version of the file in any of the patch chain
    while(ha != NULL)
    {
        // Construct the name of the patch file
        strcpy(szPatchFileName, ha->szPatchPrefix);
        strcat(szPatchFileName, szFileName);
        if(SFileOpenFileEx((HANDLE)ha, szPatchFileName, SFILE_OPEN_FROM_MPQ, (HANDLE *)&hfBase))
        {
            //
            // The following scenario is possible:
            // 1) Base MPQ file doesn't contain the desired file
            // 2) First patch MPQ contains the file with MPQ_FILE_PATCH_FILE
            // 3) Second patch contains full version of the file   
            //

            break;
        }

        // Move to the next file in the patch chain
        ha = ha->haPatch;
    }

    // If we couldn't find the file in any of the patches, it doesn't exist
    hf = hfLast = hfBase;
    if(hf == NULL)
    {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return false;
    }

    // At this point, we require that the open file is not a patch
    if(hf->pFileEntry->dwFlags & MPQ_FILE_PATCH_FILE)
    {
        FreeMPQFile(hf);
        SetLastError(ERROR_BASE_FILE_MISSING);
        return false;
    }
    
    // Move to the patch MPQ
    ha = ha->haPatch;

    // Now keep going in the patch chain and open every patch file that is there
    while(ha != NULL)
    {
        // Construct patch file name
        strcpy(szPatchFileName, ha->szPatchPrefix);
        strcat(szPatchFileName, szFileName);
        if(SFileOpenFileEx((HANDLE)ha, szPatchFileName, SFILE_OPEN_FROM_MPQ, &hPatchFile))
        {
            // Remember the new version
            hfPatch = (TMPQFile *)hPatchFile;

            // If we encountered a full replacement of the file, 
            // we have to remember the highest full file
            if((hfPatch->pFileEntry->dwFlags & MPQ_FILE_PATCH_FILE) == 0)
                hfLast = hfPatch;

            // Set current patch to base file and move on
            hf->hfPatchFile = hfPatch;
            hf = hfPatch;
        }

        // Move to the next patch in the chain
        ha = ha->haPatch;
    }

    // Now we need to free all files that are below the highest unpatched version
    while(hfBase != hfLast)
    {
        TMPQFile * hfNext = hfBase->hfPatchFile;

        // Free the file below
        hfBase->hfPatchFile = NULL;
        FreeMPQFile(hfBase);

        // Move the base to the next file
        hfBase = hfNext;
    }

    // Give the updated base MPQ
    if(phFile != NULL)
        *phFile = (HANDLE)hfBase;
    return true;
}

/*****************************************************************************/
/* Public functions                                                          */
/*****************************************************************************/

//-----------------------------------------------------------------------------
// SFileEnumLocales enums all locale versions within MPQ. 
// Functions fills all available language identifiers on a file into the buffer
// pointed by plcLocales. There must be enough entries to copy the localed,
// otherwise the function returns ERROR_INSUFFICIENT_BUFFER.

int WINAPI SFileEnumLocales(
    HANDLE hMpq,
    const char * szFileName,
    LCID * plcLocales,
    LPDWORD pdwMaxLocales,
    DWORD dwSearchScope)
{
    TMPQArchive * ha = (TMPQArchive *)hMpq;
    TMPQHash * pFirstHash;
    TMPQHash * pHash;
    DWORD dwLocales = 0;

    // Test the parameters
    if(!IsValidMpqHandle(ha))
        return ERROR_INVALID_HANDLE;
    if(pdwMaxLocales == NULL)
        return ERROR_INVALID_PARAMETER;
    if(dwSearchScope == SFILE_OPEN_BY_INDEX && (DWORD_PTR)szFileName > ha->pHeader->dwBlockTableSize)
        return ERROR_INVALID_PARAMETER;
    if(dwSearchScope != SFILE_OPEN_BY_INDEX && *szFileName == 0)
        return ERROR_INVALID_PARAMETER;

    // Parse hash table entries for all locales
    if(dwSearchScope == SFILE_OPEN_FROM_MPQ)
    {
        pFirstHash = pHash = GetFirstHashEntry(ha, szFileName);
        while(pHash != NULL)
        {
            dwLocales++;
            pHash = GetNextHashEntry(ha, pFirstHash, pHash);
        }
    }
    else
    {
        // For nameless access, always return 1 locale
//      pHash = GetFileEntryByIndex(ha, (DWORD)(DWORD_PTR)szFileName);
//      if(pHash != NULL)
//          dwLocales++;
    }

    // Test if there is enough space to copy the locales
    if(*pdwMaxLocales < dwLocales)
    {
        *pdwMaxLocales = dwLocales;
        return ERROR_INSUFFICIENT_BUFFER;
    }

    // Now find all locales
    if(dwSearchScope == SFILE_OPEN_FROM_MPQ)
    {
        pFirstHash = pHash = GetFirstHashEntry(ha, szFileName);
        while(pHash != NULL)
        {
            *plcLocales++ = pHash->lcLocale;
            pHash = GetNextHashEntry(ha, pFirstHash, pHash);
        }
    }
    else
    {
        // For nameless access, always return 1 locale
//      pHash = GetFileEntryByIndex(ha, (DWORD)(DWORD_PTR)szFileName);
//      if(pHash != NULL)
//          *plcLocales++ = pHash->lcLocale;
    }

    // Give the caller the total number of found locales
    *pdwMaxLocales = dwLocales;
    return ERROR_SUCCESS;
}

//-----------------------------------------------------------------------------
// SFileHasFile
//
//   hMpq          - Handle of opened MPQ archive
//   szFileName    - Name of file to look for

bool WINAPI SFileHasFile(HANDLE hMpq, const char * szFileName)
{
    TMPQArchive * ha = (TMPQArchive *)hMpq;
    int nError = ERROR_SUCCESS;

    if(!IsValidMpqHandle(ha))
        nError = ERROR_INVALID_HANDLE;
    if(*szFileName == 0)
        nError = ERROR_INVALID_PARAMETER;

    // Prepare the file opening
    if(nError == ERROR_SUCCESS)
    {
        if(GetFileEntryLocale(ha, szFileName, lcFileLocale) == NULL)
        {
            nError = ERROR_FILE_NOT_FOUND;
        }
    }

    // Cleanup
    if(nError != ERROR_SUCCESS)
        SetLastError(nError);
    return (nError == ERROR_SUCCESS);
}


//-----------------------------------------------------------------------------
// SFileOpenFileEx
//
//   hMpq          - Handle of opened MPQ archive
//   szFileName    - Name of file to open
//   dwSearchScope - Where to search
//   phFile        - Pointer to store opened file handle

bool WINAPI SFileOpenFileEx(HANDLE hMpq, const char * szFileName, DWORD dwSearchScope, HANDLE * phFile)
{
    TMPQArchive * ha = (TMPQArchive *)hMpq;
    TFileEntry  * pFileEntry = NULL;
    TMPQFile    * hf = NULL;
    DWORD dwBlockIndex = 0;             // Found table index
    int nError = ERROR_SUCCESS;

    // Don't accept NULL pointer to file handle
    if(phFile == NULL)
        nError = ERROR_INVALID_PARAMETER;

    // Prepare the file opening
    if(nError == ERROR_SUCCESS)
    {
        switch(dwSearchScope)
        {
            case SFILE_OPEN_PATCHED_FILE:

                // We want to open the updated version of the file
                return OpenPatchedFile(hMpq, szFileName, 0, phFile);

            case SFILE_OPEN_FROM_MPQ:
                
                if(!IsValidMpqHandle(ha))
                {
                    nError = ERROR_INVALID_HANDLE;
                    break;
                }

                if(szFileName == NULL || *szFileName == 0)
                {
                    nError = ERROR_INVALID_PARAMETER;
                    break;
                }

                // First of all, check the name as-is
                pFileEntry = GetFileEntryLocale(ha, szFileName, lcFileLocale);
                if(pFileEntry != NULL)
                    break;

                // If the file doesn't exist in the MPQ, check file pseudo-name ("FileXXXXXXXX.ext")
                if(!IsPseudoFileName(szFileName, &dwBlockIndex))
                {
                    nError = ERROR_FILE_NOT_FOUND;
                    break;
                }

                // Set the file name to the file index and fall through
                szFileName = (const char *)(DWORD_PTR)dwBlockIndex;
                dwSearchScope = SFILE_OPEN_BY_INDEX;
                // No break here, fall through.

            case SFILE_OPEN_BY_INDEX:

                if(!IsValidMpqHandle(ha))
                {
                    nError = ERROR_INVALID_HANDLE;
                    break;
                }

                // Set handle size to be sizeof(TMPQFile) + length of FileXXXXXXXX.xxx
                pFileEntry = GetFileEntryByIndex(ha, (DWORD)(DWORD_PTR)szFileName);
                if(pFileEntry == NULL)
                    nError = ERROR_FILE_NOT_FOUND;
                break;

            case SFILE_OPEN_ANY_LOCALE:

                // This open option is reserved for opening MPQ internal listfile.
                // No argument validation. Tries to open file with neutral locale first,
                // then any other available.
                dwSearchScope = SFILE_OPEN_FROM_MPQ;
                pFileEntry = GetFileEntryAny(ha, szFileName);
                if(pFileEntry == NULL)
                    nError = ERROR_FILE_NOT_FOUND;
                break;

            case SFILE_OPEN_LOCAL_FILE:

                if(szFileName == NULL || *szFileName == 0)
                {
                    nError = ERROR_INVALID_PARAMETER;
                    break;
                }

                return OpenLocalFile(szFileName, phFile); 

            default:

                // Don't accept any other value
                nError = ERROR_INVALID_PARAMETER;
                break;
        }

        // Quick return if something failed
        if(nError != ERROR_SUCCESS)
        {
            SetLastError(nError);
            return false;
        }
    }

    // Test if the file was not already deleted.
    if(nError == ERROR_SUCCESS)
    {
        if((pFileEntry->dwFlags & MPQ_FILE_EXISTS) == 0)
            nError = ERROR_FILE_NOT_FOUND;
        if(pFileEntry->dwFlags & ~MPQ_FILE_VALID_FLAGS)
            nError = ERROR_NOT_SUPPORTED;
    }

    // Allocate file handle
    if(nError == ERROR_SUCCESS)
    {
        if((hf = ALLOCMEM(TMPQFile, 1)) == NULL)
            nError = ERROR_NOT_ENOUGH_MEMORY;
    }

    // Initialize file handle
    if(nError == ERROR_SUCCESS)
    {
        memset(hf, 0, sizeof(TMPQFile));
        hf->pFileEntry = pFileEntry;
        hf->dwMagic = ID_MPQ_FILE;
        hf->ha = ha;

        hf->MpqFilePos   = pFileEntry->ByteOffset;
        hf->RawFilePos   = ha->MpqPos + hf->MpqFilePos;
        hf->dwDataSize   = pFileEntry->dwFileSize;

        // If the MPQ has sector CRC enabled, enable if for the file
        if(ha->dwFlags & MPQ_FLAG_CHECK_SECTOR_CRC)
            hf->bCheckSectorCRCs = true;

        // Decrypt file key. Cannot be used if the file is given by index
        if(dwSearchScope == SFILE_OPEN_FROM_MPQ)
        {
            if(pFileEntry->dwFlags & MPQ_FILE_ENCRYPTED)
            {
                hf->dwFileKey = DecryptFileKey(szFileName,
                                               pFileEntry->ByteOffset,
                                               pFileEntry->dwFileSize,
                                               pFileEntry->dwFlags);
            }
        }
        else
        {
            // If the file is encrypted and not compressed, we cannot detect the file key
            if(!SFileGetFileName(hf, NULL))
                nError = GetLastError();
        }
    }

    // If the file is actually a patch file, we have to load the patch file header
    if(nError == ERROR_SUCCESS && pFileEntry->dwFlags & MPQ_FILE_PATCH_FILE)
    {
        assert(hf->pPatchInfo == NULL);
        nError = AllocatePatchInfo(hf, true);
    }

    // Cleanup
    if(nError != ERROR_SUCCESS)
    {
        SetLastError(nError);
        FreeMPQFile(hf);
    }

    *phFile = hf;
    return (nError == ERROR_SUCCESS);
}

//-----------------------------------------------------------------------------
// bool WINAPI SFileCloseFile(HANDLE hFile);

bool WINAPI SFileCloseFile(HANDLE hFile)
{
    TMPQFile * hf = (TMPQFile *)hFile;
    
    if(!IsValidFileHandle(hf))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return false;
    }

    // Free the structure
    FreeMPQFile(hf);
    return true;
}
