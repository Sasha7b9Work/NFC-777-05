#pragma once


struct FILE
{

};

extern FILE *stdout;


int printf(const char *format, ...);
int fflush(FILE *);
