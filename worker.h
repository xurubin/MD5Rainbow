#pragma once
#include "common.h"
#include "TableManager.h"


struct GeneratorTaskInfo
{
	int WorkerID;
	int TotalThreads;
	CTableManager* table;
	int UIGroup;
	//ChainData *storage;
};
struct LookupTaskInfo
{
	int WorkerID;
	CTableManager* table;
	char *hash;
	int StartChainLen;
	int EndChainLen;
	bool* terminating;
	int UIGroup;
};

void rainbow_generate(GeneratorTaskInfo* task);
void rainbow_lookup(LookupTaskInfo* task);
