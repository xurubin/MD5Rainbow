#define _CRT_SECURE_NO_WARNINGS
#include "TableManager.h"
#include "worker.h"
#include "math.h"
#include "UIManager.h"
#include <stdlib.h>
#define SAVEINT(x) configfile.SetInt(#x, x)
#define SAVESTR(x) configfile.SetStr(#x, x)
#define LOADINT(x) x = configfile.GetInt(#x)
#define LOADSTR(x) x = configfile.GetStr(#x)

CTableManager::CTableManager(void):Tablename(""), ChainLength(0), TotalTableEntries(0), CompletedEntries(0)
{
	pthread_mutex_init(&mutex, NULL);
	ui = CUIManager::getSingleton().CreateGroup("Table Information");
	CUIManager::getSingleton().RegisterIntVariable("Completed Entries", (int*)&CompletedEntries, ui);
}

CTableManager::~CTableManager(void)
{
	pthread_mutex_destroy(&mutex);
}

bool CTableManager::CreateNewTable(string TableName)
{
	Tablename = TableName;
	ChainLength = 10000;
	TotalTableEntries = 100000000;
	CompletedEntries = 0;
	alphabet.Initialise(string("[LN]{1,7}|[L]{8,8}|[N]{9,11}"));
	Status = Partial;

	SynchronizeConfigFile(true);
	datafile.SetAlphabetSize(alphabet.GetAlphabetSize());
	return datafile.LoadTable(Tablename);
}

bool CTableManager::LoadTable(string TableName)
{
	Tablename = TableName;
	if (!SynchronizeConfigFile(false))
		return false;
	datafile.SetAlphabetSize(alphabet.GetAlphabetSize());
	if (datafile.LoadTable(Tablename))
	{
		CompletedEntries = datafile.GetEntriesCount();

		return true;
	}else
		return false;
}

bool CTableManager::Flush(void)
{
	return false;
}

void CTableManager::Generate(int NumThreads)
{
	int gid = -1;
	while(Status != Completed)
	{
		switch(Status)
		{
		case (Partial):
			gid = CUIManager::getSingleton().CreateGroup("Generate");
			GeneratorTaskInfo tasks[32];
			pthread_t pids[32];
			CUIManager::getSingleton().PrintLn(-1, "Start generating...", false);
			for(int i=0;i<NumThreads;i++)
			{
				tasks[i].WorkerID = i;
				tasks[i].table = this;
				tasks[i].TotalThreads = NumThreads;
				tasks[i].UIGroup = gid;
				pthread_create(&pids[i], 0, (void *(*)(void *))rainbow_generate, &tasks[i]);
			}
			for(int i=0;i<NumThreads; i++)
				pthread_join(pids[i], NULL);
			if (CompletedEntries >= TotalTableEntries)
			{
				Status = Sorting;
				CUIManager::getSingleton().RemoveGroup(gid);
			}
			break;
		case(Sorting):
			CUIManager::getSingleton().PrintLn(-1, "Start sorting...", false);
			if (!(datafile.IsCompletedSorted() && datafile.IsMergedFilesValid()))
			{
				datafile.MergeSortAll();
			}
			Status = Indexing;
			break;
		case(Indexing):
			CUIManager::getSingleton().PrintLn(-1, "Start indexing...", false);
			datafile.CreateIndexedDatabase();
			Status = Completed;
			break;
		case(Completed):
			CUIManager::getSingleton().PrintLn(-1, "Table completed.", false);
			break;
		default:;
		}
		SynchronizeConfigFile(true);
	}
}

int CTableManager::RequestWork(void)
{
	int result = 0;
	pthread_mutex_lock(&mutex);
	if(CompletedEntries < TotalTableEntries)
	{
		result = TotalTableEntries - CompletedEntries;
		if (result > PERTHREAD_BATCHSIZE)
			result = PERTHREAD_BATCHSIZE;
	}else
		result = 0;
	pthread_mutex_unlock(&mutex);
	return result;
}

void CTableManager::SignalWorkDone(int WorkerID, int NumEntries, ChainData* Entries)
{
	pthread_mutex_lock(&mutex);
	int added  = datafile.AddContent(Entries, NumEntries);
	if (added != NumEntries)
		CUIManager::getSingleton().PrintLn(ui, "CTableManager::SignalWorkDone: AddContent() returns inconsistent value!", false);
	CompletedEntries += NumEntries;
	pthread_mutex_unlock(&mutex);
	//CUIManager::getSingleton().PrintLn(ui, "Writing to disk...", false);

}

bool CTableManager::SynchronizeConfigFile(bool toDisk)
{
	if(toDisk)
	{
		configfile.Clear();
		SAVEINT(ChainLength);
		SAVEINT(TotalTableEntries);
		switch(Status)
		{
		case Invalid: 
			configfile.SetStr("Status", "Invalid");
			break;
		case Partial: 
			configfile.SetStr("Status", "Partial");
			break;
		case Sorting: 
			configfile.SetStr("Status", "Sorting");
			break;
		case Indexing:
			configfile.SetStr("Status", "Indexing");
			break;
		case Completed: 
			configfile.SetStr("Status", "Completed");
			break;
		}
		configfile.SetStr("Alphabet", alphabet.GetDescription());

		return configfile.SaveFile(Tablename + CONFIGEXT);
	}
	else
	{
		if (!configfile.LoadFile(Tablename + CONFIGEXT))
			return false;

		LOADINT(ChainLength);
		LOADINT(TotalTableEntries);
		string status = configfile.GetStr("Status");
		if (atoi(status.c_str())> 0)
			Status = (TableStatus) atoi(status.c_str());
		else if (status == "Partial")
			Status = Partial;
		else if (status == "Sorting")
			Status = Sorting;
		else if (status == "Completed")
			Status = Completed;
		else if (status == "Indexing")
			Status = Indexing;
		else
			Status = Invalid;

		return alphabet.Initialise(configfile.GetStr("Alphabet"));
	}
}

double CTableManager::GetSuccessProbability(void)
{
	int t = ChainLength;
	int m = TotalTableEntries;
	double  N = (double)alphabet.GetAlphabetSize();
	double mi_N = m / N;
	double p = 1;
	for(int i=1;i<=t;i++)
	{
		p *= (1 - mi_N);
		mi_N =  1 - exp(- mi_N);
	}
	return 1-p;
}

void CTableManager::DisplayDescription(void)
{
	char /*str[4096],*/ str2[128];
	double filesize = datafile.GetBytesPerEntry() * TotalTableEntries;
	string unit;
	if (filesize >= 1024*1024)
	{
		filesize /= 1024*1024;
		unit = "MB";
	}else if (filesize >= 1024)
	{
		filesize /= 1024;
		unit = "KB";
	}else
	{
		unit = "Byte";

	}
	long long totalhashes = ChainLength; totalhashes *= TotalTableEntries;
	long long alphabetsize = alphabet.GetAlphabetSize();
	str2[0] = '\0';
	sprintf(str2, "Rainbow table: %s", Tablename.c_str());
	CUIManager::getSingleton().PrintLn(ui,str2);
	sprintf(str2, "Chain Length: %d", ChainLength);
	CUIManager::getSingleton().PrintLn(ui,str2);
	sprintf(str2, "Total Entries: %d", TotalTableEntries);
	CUIManager::getSingleton().PrintLn(ui,str2);
	sprintf(str2, "Covered Hashes: %lld", totalhashes);
	CUIManager::getSingleton().PrintLn(ui,str2);
	sprintf(str2, "Alphabet size:  %lld", alphabetsize);
	CUIManager::getSingleton().PrintLn(ui,str2);
	sprintf(str2, "Coverage ratio: %.3f", (double)totalhashes/(double)alphabetsize);
	CUIManager::getSingleton().PrintLn(ui,str2);
	sprintf(str2, "Success Probability: %.3f", GetSuccessProbability());
	CUIManager::getSingleton().PrintLn(ui,str2);
	sprintf(str2, "Disk Size: %.3f %s", filesize, unit.c_str());
	CUIManager::getSingleton().PrintLn(ui,str2);
	
	sprintf(str2, "Disk File: %s", datafile.IsCompletedSorted()? "sorted":"unsorted");
	CUIManager::getSingleton().PrintLn(ui,str2);
	CUIManager::getSingleton().PrintLn(-1,"-----------------------------------------------------------------------------------", false);

/*
	str[0] = '\0';
	sprintf(str2, "Rainbow table: %s\n", Tablename.c_str());
	strcat(str, str2);
	sprintf(str2, "Chain Length:%d\n", ChainLength);
	strcat(str, str2);
	sprintf(str2, "Entries:%d\n", TotalTableEntries);
	strcat(str, str2);
	sprintf(str2, "Covered Hashes: %lld\n", totalhashes);
	strcat(str, str2);
	sprintf(str2, "Alphabet size:  %lld\n", alphabetsize);
	strcat(str, str2);
	sprintf(str2, "Coverage ratio: %.3f\n", (double)totalhashes/(double)alphabetsize);
	strcat(str, str2);
	sprintf(str2, "Success Probability: %.3f\n", GetSuccessProbability());
	strcat(str, str2);
	sprintf(str2, "Disk Size:%.3f %s\n", filesize, unit.c_str());
	strcat(str, str2);
	return string(str);
*/
}

bool CTableManager::Lookup(string hash_str, int NumThreads)
{
	LookupTaskInfo tasks[32];
	pthread_t pids[32];
	char hash[32];
	string hextable("0123456789ABCDEF");
	CUIManager::getSingleton().PrintLn(-1, string("Searching hash "+hash_str+"..."), false);
	if (hash_str.length() != 32) return false;
	for(unsigned int i=0;i<hash_str.length()/2;i++)
	{	
		char c = hash_str[2*i];
		if ((c>='a')&&(c<='z')) c = c - 'a' + 'A';
		int n1 = hextable.find_first_of(c);
		if ( n1 == string::npos)
			return false;
		c = hash_str[2*i+1];
		if ((c>='a')&&(c<='z')) c = c - 'a' + 'A';
		int n2 = hextable.find_first_of(c);
		if ( n2 == string::npos)
			return false;		
		hash[i] =((n1 & 0xF)<<4 ) | (n2 & 0xF);
	}
	int gid = CUIManager::getSingleton().CreateGroup("Lookup");
	CUIManager::getSingleton().ClearGroup(gid);
	bool terminating = false;
	CLookupJobPool lookupjob;
	lookupjob.SetJobs(0, ChainLength-1);
	for(int i=0;i<NumThreads;i++)
	{
		tasks[i].WorkerID = i;
		tasks[i].table = this;
		tasks[i].hash = hash;
		tasks[i].UIGroup = gid;
		tasks[i].terminating = &terminating;
		tasks[i].jobpool = &lookupjob;
		if (i==0)
			tasks[i].StartChainLen = 0;
		else
			tasks[i].StartChainLen = tasks[i-1].EndChainLen + 1;
		tasks[i].EndChainLen = (int)(sqrt( ((double)i+1)/(double)NumThreads ) * (ChainLength-1));

		pthread_create(&pids[i], 0, (void *(*)(void *))rainbow_lookup, &tasks[i]);
	}
	for(int i=0;i<NumThreads; i++)
		pthread_join(pids[i], NULL);

	CUIManager::getSingleton().PrintLn(-1, string("Searching hash done."), false);
	return true;
}
