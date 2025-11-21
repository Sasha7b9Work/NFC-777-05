#pragma once
#include "stdint.h"

/*
/* These types MUST be 16-bit or 32-bit */
typedef int                 INT; //-V677
typedef unsigned int        UINT; //-V677

/* This type MUST be 8-bit */
typedef unsigned char       BYTE; //-V677

/* These types MUST be 16-bit */
typedef short               SHORT; //-V677
typedef unsigned short      WORD; //-V677
typedef unsigned short      WCHAR; //-V677

/* These types MUST be 32-bit */
typedef long                LONG; //-V677
typedef unsigned long       DWORD; //-V677

/* This type MUST be 64-bit (Remove this for ANSI C (C89) compatibility) */
typedef unsigned long long  QWORD; //-V677

typedef unsigned int        uint32_t;
