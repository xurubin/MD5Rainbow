#pragma once
#include "common.h"
#include "ConfigFile.h"
#include "DiskFile.h"
#include "Alphabet.h"
#include <pthread.h>
#include <string.h>



class CTableManager
{
public:
	CTableManager(void);
	~CTableManager(void);
	bool CreateNewTable(string TableName);
	bool LoadTable(string TableName);
	bool Flush(void);
	void Generate(int NumThreads);
	bool Lookup(string hash_str, int NumThreads);

	int RequestWork(void);
	void SignalWorkDone(int WorkerID, int NumEntries, ChainData* Entries);

	double GetSuccessProbability(void);
	void DisplayDescription(void);
public:
	CConfigFile configfile;
	CDiskFile datafile;
	CAlphabet alphabet;

	string Tablename;
	//Internal status
	TableStatus Status;
	unsigned int ChainLength;
	unsigned int TotalTableEntries;
	unsigned int CompletedEntries;
	pthread_mutex_t mutex;
	int  ui;
private:
	bool SynchronizeConfigFile(bool toDisk);
public:
};
