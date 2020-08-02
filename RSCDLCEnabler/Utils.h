#pragma once
#include "windows.h"
#include <cstdlib>
#include <stdio.h>
#include <fstream>
#include <stdlib.h>
#include <stdarg.h>
#include <iostream>
#include <random>

char dlldir[320];
char* GetDirFile(char* name)
{
	static char pldir[320];
	strcpy_s(pldir, dlldir);
	strcat_s(pldir, name);
	return pldir;
}

void Log(const char* fmt, ...)
{
	using namespace std;
	if (!fmt)	return;

	char		text[4096];
	va_list		ap;
	va_start(ap, fmt);
	vsprintf_s(text, fmt, ap);
	va_end(ap);

	ofstream logfile;
	logfile.open(GetDirFile((PCHAR)"log.txt"), ios::app);
	if (logfile.is_open() && text)	logfile << text << endl;
	logfile.close();
}