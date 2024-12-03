#pragma once

#include <comdef.h>

extern "C"

{   
    int __stdcall extractText(char* fileName, BSTR *textOutput, int firstPage, int lastPage, const char* textOutEncoding, const char* layout, void ( __stdcall *logCallback) (const BSTR), const char* ownerPassword, const char* userPassword);
    int __stdcall getNumPages(char* fileName, void ( __stdcall *logCallback) (const BSTR), const char* ownerPassword, const char* userPassword);
}
//int extractText(char* fileName, int firstPage, int lastPage, const char* textOutEnc, const char* layout, char** textOutput, void (*logCallback)(const char*), const char* ownerPassword, const char* userPassword);
//int getNumPages(char* fileName, void (*logCallback)(const char*), const char* ownerPassword, const char* userPassword);