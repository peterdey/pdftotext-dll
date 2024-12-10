#include "Lib.h"
#include <sstream>
#include <iostream>

#include <aconf.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include <windows.h>
#include <comdef.h>

#include "gmem.h"
#include "gmempp.h"
#include "parseargs.h"
#include "GString.h"
#include "GList.h"
#include "GlobalParams.h"
#include "Object.h"
#include "Stream.h"
#include "Array.h"
#include "Dict.h"
#include "XRef.h"
#include "Catalog.h"
#include "Page.h"
#include "PDFDoc.h"
#include "TextOutputDev.h"
#include "CharTypes.h"
#include "UnicodeMap.h"
#include "TextString.h"
#include "Error.h"
#include "config.h"

//for test only
#include <thread>
#include <chrono>
using namespace std::chrono_literals;

/*static int firstPage = 1;
static int lastPage = 0;*/
static GBool physLayout = gFalse;
static GBool simpleLayout = gFalse;
static GBool simple2Layout = gFalse;
static GBool tableLayout = gFalse;
static GBool linePrinter = gFalse;
static GBool rawOrder = gFalse;
static double fixedPitch = 0;
static double fixedLineSpacing = 0;
static GBool clipText = gFalse;
static GBool discardDiag = gFalse;
static char textEncName[128] = "UTF-8";
static char textEOL[16] = "";
static GBool noPageBreaks = gFalse;
static GBool insertBOM = gFalse;
static double marginLeft = 0;
static double marginRight = 0;
static double marginTop = 0;
static double marginBottom = 0;
static char ownerPassword[33] = "\001";
static char userPassword[33] = "\001";
static GBool quiet = gFalse;
static char cfgFileName[256] = "";
static GBool listEncodings = gFalse;
static GBool printVersion = gFalse;
static GBool printHelp = gFalse;

int __stdcall extractText(char* fileName, BSTR *lpTextOutput, int firstPage, int lastPage, const char* textOutEncoding, const char* layout, void ( __stdcall *logCallback) (const BSTR), const char* ownerPassword, const char* userPassword) {
	SELECTION sel;
	sel.firstPage = firstPage;
	sel.lastPage = lastPage;

	return _extractText(fileName, lpTextOutput, sel, textOutEncoding, layout, logCallback, ownerPassword, userPassword);
}

int __stdcall extractTextSlice(char* fileName, BSTR *lpTextOutput, int page, int sliceX, int sliceY, int sliceW, int sliceH, const char* textOutEncoding, const char* layout, void ( __stdcall *logCallback) (const BSTR), const char* ownerPassword, const char* userPassword) {
	SELECTION sel;
	sel.firstPage = page;
	sel.sliceX = sliceX;
	sel.sliceY = sliceY;
	sel.sliceW = sliceW;
	sel.sliceH = sliceH;

	return _extractText(fileName, lpTextOutput, sel, textOutEncoding, layout, logCallback, ownerPassword, userPassword);
}

int __stdcall _extractText(char* fileName, BSTR *lpTextOutput, SELECTION sel, const char* textOutEncoding, const char* layout, void ( __stdcall *logCallback) (const BSTR), const char* ownerPassword, const char* userPassword) {
	PDFDoc* doc;
	GString* ownerPW, * userPW;
	TextOutputControl *textOutControl;
	TextOutputDev* textOut;
	UnicodeMap* uMap;

	// read config file
	globalParams = new GlobalParams(cfgFileName);
	if (textOutEncoding) {
		globalParams->setTextEncoding(textOutEncoding);
	} else {
		globalParams->setTextEncoding(textEncName);
	}

	//globalParams->setTextEOL(textEOL))  
	if (noPageBreaks) {
		globalParams->setTextPageBreaks(gFalse);
	}
	if (quiet) {
		globalParams->setErrQuiet(quiet);
	}

	// get mapping to output encoding
	if (!(uMap = globalParams->getTextEncoding())) {
		error(errConfig, -1, "Couldn't get text encoding (extractText)");
		if (logCallback != NULL) {
			logCallback(SysAllocString(L"Couldn't get text encoding (extractText)"));
		}
		delete globalParams;
		return 99;
	}

	// open PDF file
	if (ownerPassword != NULL) {
		ownerPW = new GString(ownerPassword);
	} else {
		ownerPW = NULL;
	}
	if (userPassword != NULL) {
		userPW = new GString(userPassword);
	} else {
		userPW = NULL;
	}

	doc = new PDFDoc(fileName, ownerPW, userPW);
	if (userPW) {
		delete userPW;
	}
	if (ownerPW) {
		delete ownerPW;
	}

	if (!doc->isOk()) {
		if (logCallback != NULL) {
			logCallback(SysAllocString(L"doc is not Ok (extractText)"));
		}
		delete doc;
		uMap->decRefCnt();
		return 1;
	}

	// check for copy permission
	if (!doc->okToCopy()) {
		error(errNotAllowed, -1,
			"Copying of text from this document is not allowed (extractText).");
		if (logCallback != NULL) {
			logCallback(SysAllocString(L"Copying of text from this document is not allowed (extractText)."));
		}
		delete doc;
		uMap->decRefCnt();
		return 1;
	}

	// construct text file name  
	//textFileName = new GString(outFileName); 

	// get page range
	if (sel.firstPage < 1) {
		sel.firstPage = 1;
	}
	if (sel.lastPage < 1 || sel.lastPage > doc->getNumPages()) {
		sel.lastPage = doc->getNumPages();
	}

	// write text file
	if (layout == NULL) {
		layout = "NULL";
	}
	textOutControl = new TextOutputControl();

	if (strcmp(layout, "table") == 0) {
		textOutControl->mode = textOutTableLayout;
		textOutControl->fixedPitch = fixedPitch;
	}
	else if (strcmp(layout, "phys") == 0) {
		textOutControl->mode = textOutPhysLayout;
		textOutControl->fixedPitch = fixedPitch;
	}
	else if (strcmp(layout, "simple") == 0) {
		textOutControl->mode = textOutSimpleLayout;
	}
	else if (strcmp(layout, "simple2") == 0) {
		textOutControl->mode = textOutSimple2Layout;
	}
	else if (strcmp(layout, "linePrinter") == 0) {
		textOutControl->mode = textOutLinePrinter;
		textOutControl->fixedPitch = fixedPitch;
		textOutControl->fixedLineSpacing = fixedLineSpacing;
	}
	else if (strcmp(layout, "rawOrder") == 0) {
		textOutControl->mode = textOutRawOrder;
	}
	else {
		textOutControl->mode = textOutReadingOrder;
	}

	textOutControl->clipText = clipText;
	textOutControl->discardDiagonalText = discardDiag;
	textOutControl->insertBOM = insertBOM;
	textOutControl->marginLeft = marginLeft;
	textOutControl->marginRight = marginRight;
	textOutControl->marginTop = marginTop;
	textOutControl->marginBottom = marginBottom;

	//textOut = new TextOutputDev(textFileName->getCString(), &textOutControl, gFalse, gTrue);  
	//stream = open_memstream(&bp, &size);
	std::stringstream* stream = new std::stringstream(std::stringstream::out | std::stringstream::in);	
	
	textOut = new TextOutputDev([](void* str, const char* text, int len) {
		std::stringstream* stream = (std::stringstream*)str;
		stream->write(text, len);
		//printf("%s", text);
		}, stream, textOutControl);

	if (textOut->isOk()) {
		if (sel.sliceX >= 0) {
			doc->displayPageSlice(textOut, sel.firstPage, 72, 72, 0, gFalse, gTrue, gFalse, sel.sliceX, sel.sliceY, sel.sliceW, sel.sliceH);
		} else {
			doc->displayPages(textOut, sel.firstPage, sel.lastPage, 72, 72, 0, gFalse, gTrue, gFalse);
		}
	}
	else {
		if (logCallback != NULL) {
			logCallback(SysAllocString(L"text Out is not Ok (extractText)"));
		}
		delete textOut;
		return 2;
	}
	//free
	delete globalParams;	
	delete textOut;
	delete textOutControl;
	delete doc;

	//copy stringstream data to textOutput
	std::string str = stream->str();
	char* cstr = new char[str.length() + 1];
	strcpy(cstr, str.c_str());
	delete stream;
	str = "";

	// Check if the input BSTR pointer is valid
    if (lpTextOutput == nullptr) {
        return -1; // Null pointer supplied
    }
    // If the BSTR was previously allocated, free it before allocating new memory
    if (*lpTextOutput != nullptr) {
        SysFreeString(*lpTextOutput);  // Free the previously allocated BSTR
    }

    // Determine how long the UTF16 string will be
    int len = MultiByteToWideChar(CP_UTF8, 0, cstr, -1, nullptr, 0);
    if (len == 0) {
        return -2; // Conversion failure
    }

    // Allocate a new BSTR to assign the converted wide string
    *lpTextOutput = SysAllocStringLen(nullptr, len - 1);  // Allocate space for the string, excluding the null terminator
    if (*lpTextOutput == nullptr) {
        return -3; // Error code for memory allocation failure
    }

    // Convert the UTF-8 string to the UTF16 BSTR (wide string)
    MultiByteToWideChar(CP_UTF8, 0, cstr, -1, *lpTextOutput, len);

	return 0;
}

int __stdcall getNumPages(char* fileName, void ( __stdcall *logCallback) (const BSTR), const char* ownerPassword, const char* userPassword) {
	PDFDoc* doc;
	GString* ownerPW, * userPW;
	
	UnicodeMap* uMap;

#ifdef DEBUG_FP_LINUX
	// enable exceptions on floating point div-by-zero
	feenableexcept(FE_DIVBYZERO);
	// force 64-bit rounding: this avoids changes in output when minor
	// code changes result in spills of x87 registers; it also avoids
	// differences in output with valgrind's 64-bit floating point
	// emulation (yes, this is a kludge; but it's pretty much
	// unavoidable given the x87 instruction set; see gcc bug 323 for
	// more info)
	fpu_control_t cw;
	_FPU_GETCW(cw);
	cw = (fpu_control_t)((cw & ~_FPU_EXTENDED) | _FPU_DOUBLE);
	_FPU_SETCW(cw);
#endif

	// read config file
	globalParams = new GlobalParams(cfgFileName);	
	globalParams->setTextEncoding(textEncName);
	//globalParams->setTextEOL(textEOL))  
	if (noPageBreaks) {
		globalParams->setTextPageBreaks(gFalse);
	}
	if (quiet) {
		globalParams->setErrQuiet(quiet);
	}

	// get mapping to output encoding
	if (!(uMap = globalParams->getTextEncoding())) {
		error(errConfig, -1, "Couldn't get text encoding (getNumPages)");
		if (logCallback != NULL) {
			logCallback(SysAllocString(L"Couldn't get text encoding (getNumPages)"));
		}
		delete globalParams;
		return 99;
	}

	// open PDF file
	if (ownerPassword != NULL) {
		ownerPW = new GString(ownerPassword);
	}
	else {
		ownerPW = NULL;
	}
	if (userPassword != NULL) {
		userPW = new GString(userPassword);
	}
	else {
		userPW = NULL;
	}

	doc = new PDFDoc(fileName, ownerPW, userPW);
	if (userPW) {
		delete userPW;
	}
	if (ownerPW) {
		delete ownerPW;
	}

	if (!doc->isOk()) {
		if (logCallback != NULL) {
			logCallback(SysAllocString(L"doc is not Ok (getNumPages)"));
		}
		delete doc;
		uMap->decRefCnt();
		return -1;
	}

	// check for copy permission
	if (!doc->okToCopy()) {
		error(errNotAllowed, -1,
			"Copying of text from this document is not allowed (getNumPages).");
		if (logCallback != NULL) {
			logCallback(SysAllocString(L"Copying of text from this document is not allowed (getNumPages)."));
		}
		delete doc;
		uMap->decRefCnt();
		return -1;
	}
		
	// get page range
	int pages= doc->getNumPages();	
	//free
	delete globalParams;	
	delete doc;

	return pages;
}