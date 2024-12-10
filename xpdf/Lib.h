#pragma once

#include <comdef.h>

struct SELECTION {
	int firstPage = -1;
	int lastPage = -1;
	int sliceX = -1;
	int sliceY = -1;
	int sliceW = -1;
	int sliceH = -1;
};

extern "C"

{   
    int __stdcall extractText(char* fileName, BSTR *lpTextOutput, int firstPage, int lastPage, const char* textOutEncoding, const char* layout, void ( __stdcall *logCallback) (const BSTR), const char* ownerPassword, const char* userPassword);
    int __stdcall extractTextSlice(char* fileName, BSTR *lpTextOutput, int page, int sliceX, int sliceY, int sliceW, int sliceH, const char* textOutEncoding, const char* layout, void ( __stdcall *logCallback) (const BSTR), const char* ownerPassword, const char* userPassword);
    int __stdcall getNumPages(char* fileName, void ( __stdcall *logCallback) (const BSTR), const char* ownerPassword, const char* userPassword);
}
//int extractText(char* fileName, int firstPage, int lastPage, const char* textOutEnc, const char* layout, char** textOutput, void (*logCallback)(const char*), const char* ownerPassword, const char* userPassword);
//int getNumPages(char* fileName, void (*logCallback)(const char*), const char* ownerPassword, const char* userPassword);

int __stdcall _extractText(char* fileName, BSTR *lpTextOutput, SELECTION sel, const char* textOutEncoding, const char* layout, void ( __stdcall *logCallback) (const BSTR), const char* ownerPassword, const char* userPassword);
