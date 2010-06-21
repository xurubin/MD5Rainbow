#pragma once
#include "common.h"
#include "TableManager.h"
#include "pthread.h"
#include <vector>

typedef vector<pair<int, Index_Type>> ChainData_List;
bool ChainData_List_Pred(pair<int, Index_Type> first, pair<int, Index_Type> second);

class CLookupJobPool
{
public:
	CLookupJobPool();
	~CLookupJobPool();
	void SetLookupJobs(int min, int max);
	int GetNextChain();
	void SubmitChainData(int chain_offset, Index_Type finish_hash);
	int GetNextChainHeads(CDiskFile& datafile, int* start_offset, Index_Type* start_indexes);
private:
	pthread_mutex_t mutex;
	volatile int prelookup;
	ChainData_List chaindata;
	unsigned int chaindata_pointer;
	int chain_v, chain_min, chain_max;
	volatile int progress;
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

