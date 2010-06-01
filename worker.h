#pragma once
#include "common.h"
#include "TableManager.h"
#include "pthread.h"

class CLookupJobPool
{
public:
	CLookupJobPool();
	~CLookupJobPool();
	void SetJobs(int min, int max);
	int GetNextJob();
private:
	pthread_mutex_t mutex;
	int v, min, max;
	int gid;
};

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
	CLookupJobPool* jobpool;
	bool* terminating;
	int UIGroup;
};

void rainbow_generate(GeneratorTaskInfo* task);
void rainbow_lookup(LookupTaskInfo* task);

