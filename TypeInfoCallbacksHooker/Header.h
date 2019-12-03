#pragma once
#include <wdm.h>


typedef struct _OBJECT_DUMP_CONTROL
{
	/* 0x0000 */ void* Stream;
	/* 0x0008 */ unsigned long Detail;
	/* 0x000c */ long __PADDING__[1];
} OBJECT_DUMP_CONTROL, * POBJECT_DUMP_CONTROL; /* size: 0x0010 */

typedef enum _OB_OPEN_REASON
{
	ObCreateHandle = 0,
	ObOpenHandle = 1,
	ObDuplicateHandle = 2,
	ObInheritHandle = 3,
	ObMaxOpenReason = 4,
} OB_OPEN_REASON, * POB_OPEN_REASON;


typedef struct _OB_EXTENDED_PARSE_PARAMETERS
{
	/* 0x0000 */ unsigned short Length;
	/* 0x0002 */ char Padding_0[2];
	/* 0x0004 */ unsigned long RestrictedAccessMask;
	/* 0x0008 */ struct _EJOB* Silo;
} OB_EXTENDED_PARSE_PARAMETERS, * POB_EXTENDED_PARSE_PARAMETERS; /* size: 0x0010 */

