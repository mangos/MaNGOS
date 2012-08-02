#define _CRT_SECURE_NO_DEPRECATE

#include "loadlib.h"

// list of mpq files for lookup most recent file version
ArchiveSet gOpenArchives;

ArchiveSetBounds GetArchivesBounds()
{
    return ArchiveSetBounds(gOpenArchives.begin(), gOpenArchives.end());
}

bool OpenArchive(char const* mpqFileName, HANDLE* mpqHandlePtr /*= NULL*/)
{
    HANDLE mpqHandle;

    if (!SFileOpenArchive(mpqFileName, 0, MPQ_OPEN_READ_ONLY, &mpqHandle))
        return false;

    gOpenArchives.push_back(mpqHandle);

    if (mpqHandlePtr)
        *mpqHandlePtr = mpqHandle;

    return true;
}

bool OpenNewestFile(char const* filename, HANDLE* fileHandlerPtr)
{
    for(ArchiveSet::const_reverse_iterator i=gOpenArchives.rbegin(); i!=gOpenArchives.rend();++i)
    {
        // always prefer get updated file version
        if (SFileOpenFileEx(*i, filename, SFILE_OPEN_PATCHED_FILE, fileHandlerPtr))
            return true;
    }

    return false;
}

bool ExtractFile( char const* mpq_name, std::string const& filename )
{
    for(ArchiveSet::const_reverse_iterator i=gOpenArchives.rbegin(); i!=gOpenArchives.rend();++i)
    {
        HANDLE fileHandle;
        if (!SFileOpenFileEx(*i, mpq_name, SFILE_OPEN_PATCHED_FILE, &fileHandle))
            continue;

        if (SFileGetFileSize(fileHandle, NULL) == 0)              // some files removed in next updates and its reported  size 0
        {
            SFileCloseFile(fileHandle);
            return true;
        }

        SFileCloseFile(fileHandle);

        if (!SFileExtractFile(*i, mpq_name, filename.c_str(), SFILE_OPEN_PATCHED_FILE))
        {
            printf("Can't extract file: %s\n", mpq_name);
            return false;
        }

        return true;
    }

    printf("Extracting file not found: %s\n", filename.c_str());
    return false;
}


void CloseArchives()
{
    for(ArchiveSet::const_iterator i = gOpenArchives.begin(); i != gOpenArchives.end();++i)
        SFileCloseArchive(*i);
    gOpenArchives.clear();
}

FileLoader::FileLoader()
{
    data = 0;
    data_size = 0;
    version = 0;
}

FileLoader::~FileLoader()
{
    free();
}

bool FileLoader::loadFile(char *filename, bool log)
{
    free();

    HANDLE fileHandle = 0;

    if (!OpenNewestFile(filename, &fileHandle))
    {
        if (log)
            printf("No such file %s\n", filename);
        return false;
    }

    data_size = SFileGetFileSize(fileHandle, NULL);

    data = new uint8 [data_size];
    if (!data)
    {
        SFileCloseFile(fileHandle);
        return false;
    }

    if (!SFileReadFile(fileHandle, data, data_size, NULL, NULL))
    {
        if (log)
            printf("Can't read file %s\n", filename);
        SFileCloseFile(fileHandle);
        return false;
    }

    SFileCloseFile(fileHandle);

    // ToDo: Fix WDT errors...
    if (!prepareLoadedData())
    {
        //printf("Error loading %s\n\n", filename);
        //free();
        return true;
    }

    return true;
}

bool FileLoader::prepareLoadedData()
{
    // Check version
    version = (file_MVER *) data;

    if (version->fcc != 'MVER')
        return false;

    if (version->ver != FILE_FORMAT_VERSION)
        return false;
    return true;
}

void FileLoader::free()
{
    if (data) delete[] data;
    data = 0;
    data_size = 0;
    version = 0;
}