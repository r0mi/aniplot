// Elmo Trolla, 2019
// Licence: pick one - public domain / UNLICENCE (https://www.unlicense.org) / MIT (https://opensource.org/licenses/MIT).

//
// memory-mapped file interface for windows.
//
//     MemFile f("data.bin");
//     printf("last byte: %i", (int)f.buf[f.file_size-1]);
//

#ifndef MEMFILE_H
#define MEMFILE_H


#include "windows.h"

// memorymapped file read
struct MemFile
{
	char* buf;
	uint64_t file_size;

	MemFile(char* filename)
	{
		buf = NULL;
		fileHandle = NULL;
		mapFile = NULL;
		file_size = 0;

		fileHandle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
		if (!fileHandle)	return;

		if (!GetFileSizeEx(fileHandle, (PLARGE_INTEGER)&file_size)) return;

    		//mapFile = OpenFileMapping(FILE_MAP_READ, FALSE, filename);
		mapFile = CreateFileMapping(fileHandle, NULL, PAGE_READONLY, 0, 0, NULL);
		//mapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READONLY, 0, 0, filename);

		if (!mapFile) return;
		buf = (char*)MapViewOfFile(mapFile, FILE_MAP_READ, 0, 0, 0);
		//buf = (LPTSTR) MapViewOfFile(mapFile, FILE_MAP_READ, 0, 0, BUF_SIZE);
		if (!buf) CloseHandle(mapFile);
	}

	~MemFile() {
		if (buf) UnmapViewOfFile(buf);
		if (mapFile) CloseHandle(mapFile);
		if (fileHandle) CloseHandle(fileHandle);
	}

private:

	HANDLE fileHandle;
	HANDLE mapFile;
};


#endif // MEMFILE_H
