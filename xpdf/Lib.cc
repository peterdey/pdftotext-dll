#include "Lib.h"
#include <sstream>
#include <iostream>

#include <aconf.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#ifdef DEBUG_FP_LINUX
#  include <fenv.h>
#  include <fpu_control.h>
#endif
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

//static std::stringstream* stream = new std::stringstream(std::stringstream::out | std::stringstream::in );

//void (*TextOutputFunc)(void* stream, const char* text, int len)

/*void textOutputFun(void* str, const char* text, int len) {
	//printf("%s", text);
	std::stringstream* st = (std::stringstream*)str;
	st->write("123456",6);
	printf("textOutputFun \r\n");
}*/

//void (*textOutputFunc)(void* stream, const char* text, int len)

int extractText(char* fileName, int firstPage, int lastPage, const char* textOutEnc, const char* layout, char** textOutput, void (*logCallback)(const char*)) {
	PDFDoc* doc;
	GString* ownerPW, * userPW;
	TextOutputControl textOutControl;
	TextOutputDev* textOut;
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
	if (textOutEnc) {
		globalParams->setTextEncoding(textOutEnc);
	}
	else
	{
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
		error(errConfig, -1, "Couldn't get text encoding");
		if (logCallback != NULL) {
			logCallback("Couldn't get text encoding");
		}
		delete globalParams;
		return 99;
	}

	// open PDF file
	if (ownerPassword[0] != '\001') {
		ownerPW = new GString(ownerPassword);
	}
	else {
		ownerPW = NULL;
	}
	if (userPassword[0] != '\001') {
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
			logCallback("doc is not Ok");
		}
		delete doc;
		uMap->decRefCnt();
		return 1;
	}

	// check for copy permission
	if (!doc->okToCopy()) {
		error(errNotAllowed, -1,
			"Copying of text from this document is not allowed.");
		if (logCallback != NULL) {
			logCallback("Copying of text from this document is not allowed.");
		}
		delete doc;
		uMap->decRefCnt();
		return 1;
	}

	// construct text file name  
	//textFileName = new GString(outFileName); 

	// get page range
	if (firstPage < 1) {
		firstPage = 1;
	}
	if (lastPage < 1 || lastPage > doc->getNumPages()) {
		lastPage = doc->getNumPages();
	}

	// write text file
	if (layout == NULL) {
		layout = "NULL";
	}
	if (strcmp(layout, "table") == 0) {
		textOutControl.mode = textOutTableLayout;
		textOutControl.fixedPitch = fixedPitch;
	}
	else if (strcmp(layout, "phys") == 0) {
		textOutControl.mode = textOutPhysLayout;
		textOutControl.fixedPitch = fixedPitch;
	}
	else if (strcmp(layout, "simple") == 0) {
		textOutControl.mode = textOutSimpleLayout;
	}
	else if (strcmp(layout, "simple2") == 0) {
		textOutControl.mode = textOutSimple2Layout;
	}
	else if (strcmp(layout, "linePrinter") == 0) {
		textOutControl.mode = textOutLinePrinter;
		textOutControl.fixedPitch = fixedPitch;
		textOutControl.fixedLineSpacing = fixedLineSpacing;
	}
	else if (strcmp(layout, "rawOrder") == 0) {
		textOutControl.mode = textOutRawOrder;
	}
	else {
		textOutControl.mode = textOutReadingOrder;
	}

	textOutControl.clipText = clipText;
	textOutControl.discardDiagonalText = discardDiag;
	textOutControl.insertBOM = insertBOM;
	textOutControl.marginLeft = marginLeft;
	textOutControl.marginRight = marginRight;
	textOutControl.marginTop = marginTop;
	textOutControl.marginBottom = marginBottom;

	//textOut = new TextOutputDev(textFileName->getCString(), &textOutControl, gFalse, gTrue);  
	//stream = open_memstream(&bp, &size);
	 std::stringstream* stream = new std::stringstream(std::stringstream::out | std::stringstream::in);
	//textOut = new TextOutputDev(textOutputFunc, stream, &textOutControl);
	
	textOut = new TextOutputDev([](void* str, const char* text, int len) {
		std::stringstream* stream = (std::stringstream*)str;
		stream->write(text, len);
		//printf("%s", text);
		}, stream, &textOutControl);

	if (textOut->isOk()) {
		doc->displayPages(textOut, firstPage, lastPage, 72, 72, 0,
			gFalse, gTrue, gFalse);
	}
	else {
		if (logCallback != NULL) {
			logCallback("text Out is not Ok");
		}
		delete textOut;
		return 2;
	}
	delete textOut;

	// check for memory leaks
	Object::memCheck(stderr);
	gMemReport(stderr);
	//copy stringstream data to textOutput
	std::string str = stream->str();
	char* cstr = new char[str.length() + 1];
	strcpy(cstr, str.c_str());
	stream->clear();
	*textOutput = cstr;	

	return 0;
}


/*int main(int argc, char* argv[]) {
	printf("inicio \r\n");
	char* fileName = "C:/MyDartProjects/riodasostras/pdf_text_extraction/downloads/a980ff28-9e4b-496d-84a5-ba28f10c7751.pdf";
	// char* outFileName = "C:/MyDartProjects/riodasostras/pdf_text_extraction/out.txt";
	char* textOutput = NULL;
	int re = extractText(fileName, 1, 1, "UTF-8", NULL, &textOutput, NULL);
	printf("result %s \r\n", textOutput);

	delete[] textOutput;
}*/