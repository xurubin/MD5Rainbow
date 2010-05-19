#pragma once

#define CONFIGEXT ".ini"
#define TABLEEXT ".rnb"
#define MAXSIZE_TMPTABLE_GUIDELINE (64*1024*1024)
#define MERGESORT_MEMORY 256*1024*1024

#define PERTHREAD_MEMORY 16*1024*1024
#define PERTHREAD_BATCHSIZE 100

#define CHECKSORT_ITERATIONS 25

#define LINE_WIDTH 75
typedef unsigned long long int Index_Type;
enum TableStatus
{
	Invalid = 11,
	Partial = 21,
	Sorting = 31,
	Indexing = 41,
	Completed = 99
};

struct ChainData
{
	Index_Type StartIndex;
	Index_Type FinishHash;
};

