#define _CRT_SECURE_NO_WARNINGS
#include "worker.h"
#include "pthread.h"
#include "HashReduce.h"
#ifdef WIN32
#include "windows.h"
#endif
#include "UIManager.h"
#include <string.h>
#include <algorithm>
#define DBGPRINT hash_buf[str_len] = 0;  \
printf("Lookup(\"%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x\", 4); // %s\n", \
(unsigned char)hash_digest[0], (unsigned char)hash_digest[1], \
(unsigned char)hash_digest[2], (unsigned char)hash_digest[3], \
(unsigned char)hash_digest[4], (unsigned char)hash_digest[5], \
(unsigned char)hash_digest[6], (unsigned char)hash_digest[7], \
(unsigned char)hash_digest[8], (unsigned char)hash_digest[9], \
(unsigned char)hash_digest[10], (unsigned char)hash_digest[11], \
(unsigned char)hash_digest[12], (unsigned char)hash_digest[13], \
(unsigned char)hash_digest[14], (unsigned char)hash_digest[15], \
hash_buf); 

Index_Type hash_mask;
bool ChainData_List_Pred(pair<int, Index_Type> first, pair<int, Index_Type> second) { 
	return (first.second&hash_mask) < (second.second&hash_mask); 
}

void ComputeHashChain(CAlphabet& alphabet, Index_Type hash_range, int start_offset, unsigned int chain_length, const char * start_hash, char* final_hash, char* last_str = 0)
{
	char hash_buf[64];
	char hash_digest[16];
	Index_Type index;
	int str_len =0;
	memset(hash_buf, 0, sizeof(hash_buf));
	for(int i=0;i<16;i++) hash_digest[i] = start_hash[i];

	for(unsigned int i=0;i < chain_length;i++)
	{
		index = CMD5HashReduce::Reduce(hash_digest, start_offset++) % hash_range;
		*(int *)(hash_buf+0) = 0; //Clear only the first 16 bytes, should be enough
		*(int *)(hash_buf+4) = 0;
		*(int *)(hash_buf+8) = 0;
		*(int *)(hash_buf+12) = 0;
		str_len = alphabet.Lookup(index, hash_buf);
		hash_buf[str_len] = (char)0x80;
		*(int *)(hash_buf+56) = str_len*8;
		CMD5HashReduce::Hash_One_Block(hash_buf, hash_digest);
		/*DBGPRINT*/
	}
	for(int i=0;i<16;i++) final_hash[i] = hash_digest[i];
	if(last_str)
	{
		strncpy(last_str, hash_buf, str_len);
		last_str[str_len] = '\0';
	}
}

void rainbow_generate(GeneratorTaskInfo* task)
{
	ChainData *chain_buf = new ChainData[PERTHREAD_MEMORY/sizeof(ChainData)];
	int chainbuf_len = PERTHREAD_MEMORY/sizeof(ChainData);
	char hash_buf[64];
	char hash_digest[16];
	int str_len;
	Index_Type index;
	Index_Type hash_range = task->table->alphabet.GetAlphabetSize();//((Index_Type)1 << task->table->alphabet.GetSizeBits()) - 1;
	Index_Type rndseed = task->table->datafile.GetEntriesCount();
	int worklen;
	int progress_counter = 0;
#ifdef WIN32
	SetThreadPriority(GetCurrentThread(), THREAD_BASE_PRIORITY_IDLE);
#endif
	int gid = task->UIGroup;
	char varname[20]; sprintf(varname, "Worker %d Speed", task->WorkerID);
	CUIManager::getSingleton().RegisterIntVariable(varname, &progress_counter, gid)->style = Gradient;

	for(int i=0;i<sizeof(hash_buf);i++) hash_buf[i] = 0;
	while ((worklen = task->table->RequestWork()) > 0)
	{ //Loop for more work
		do
		{ //Divide worklen into smaller batches that fits in chain_buf

			int batch_len = (worklen > chainbuf_len) ? chainbuf_len : worklen;
			for(int i =0; i<batch_len; i++)
			{
				//Compute chain_buf[i];
				index = ((rndseed+=task->TotalThreads) + task->WorkerID) % hash_range;
				chain_buf[i].StartIndex = index;

				*(int *)(hash_buf+0) = 0; //Clear only the first 16 bytes, should be enough
				*(int *)(hash_buf+4) = 0;
				*(int *)(hash_buf+8) = 0;
				*(int *)(hash_buf+12) = 0;
				str_len = task->table->alphabet.Lookup(index, hash_buf);
				hash_buf[str_len] = (char)0x80;
				*(int *)(hash_buf+56) = str_len*8;
				CMD5HashReduce::Hash_One_Block(hash_buf, hash_digest);
/*DBGPRINT*/
//long t0 = GetTickCount();
				ComputeHashChain(task->table->alphabet, hash_range, 1, task->table->ChainLength-1, hash_digest, hash_digest);

				chain_buf[i].FinishHash = *(Index_Type*)hash_digest;
				progress_counter++;
			}

			task->table->SignalWorkDone(task->WorkerID, batch_len, chain_buf);
			worklen -= batch_len;
		}while (worklen > 0);
	}
	CUIManager::getSingleton().UnregisterVariable(gid, varname);
	delete[] chain_buf;
#ifdef WIN32
	//CUIManager::getSingleton().UnregisterInt(&speed);
#endif

}

void rainbow_lookup(LookupTaskInfo* task)
{
	char hash_buf[64];
	char hash_digest[16];
	int str_len;
	Index_Type index;
	Index_Type heads[1024];
	Index_Type hash_range = task->table->alphabet.GetAlphabetSize();//((Index_Type)1 << task->table->alphabet.GetSizeBits()) - 1;
	int chain_position;
	hash_mask = task->table->datafile.hash_mask;
	for(int i=0;i<sizeof(hash_buf);i++) hash_buf[i] = 0;


#ifdef WIN32
	SetThreadPriority(GetCurrentThread(), THREAD_BASE_PRIORITY_IDLE);
#endif
	int gid = task->UIGroup;
	//int progress = 0;
	//char progressname[20]; 
	//sprintf(progressname, "Progress%d", task->WorkerID);
	//IntVariable* visualiser = CUIManager::getSingleton().RegisterIntVariable(progressname, &progress, gid);
	//visualiser->min = 0;
	//visualiser->max = (task->EndChainLen+task->StartChainLen)*(task->EndChainLen-task->StartChainLen)/2;
	//visualiser->style = Progress;

	int falsealarms = 0;
	char falsealarmname[20];
	sprintf(falsealarmname, "False Alarm %d", task->WorkerID);
	CUIManager::getSingleton().RegisterIntVariable(falsealarmname, &falsealarms, gid);

	//for(chain_position=task->StartChainLen; chain_position<=task->EndChainLen; chain_position++)
	while((chain_position=task->jobpool->GetNextChain()) >= 0)
	{
		// Check if the requested hash is at position chain_position of some hash chain.
		//progress+= chain_position;
		for(int j=0;j<sizeof(hash_digest);j++) hash_digest[j] = task->hash[j];

		// Assume it's at chain_position, first find the hypothetical finish hash
		ComputeHashChain(task->table->alphabet, hash_range, task->table->ChainLength-chain_position, chain_position, hash_digest, hash_digest);

		task->jobpool->SubmitChainData(chain_position, *(Index_Type*)hash_digest);
	}

	int matches = 0;
	while((matches = task->jobpool->GetNextChainHeads(task->table->datafile, &chain_position, heads)) >= 0)
	{
		if (*(task->terminating))
			break;
		if(matches == 0) continue;
		bool found_match = false;
		//Check if there is a possible match
		for(int match_i=0;match_i<matches;match_i++)
		{
			index = heads[match_i];
			*(int *)(hash_buf+0) = 0; //Clear only the first 16 bytes, should be enough
			*(int *)(hash_buf+4) = 0;
			*(int *)(hash_buf+8) = 0;
			*(int *)(hash_buf+12) = 0;
			str_len = task->table->alphabet.Lookup(index, hash_buf);
			hash_buf[str_len] = (char)0x80;
			*(int *)(hash_buf+56) = str_len*8;
			CMD5HashReduce::Hash_One_Block(hash_buf, hash_digest);
			ComputeHashChain(task->table->alphabet, hash_range, 1, task->table->ChainLength-chain_position-1, hash_digest, hash_digest, hash_buf);

			//Check if it's a false positive
			bool falsepositive = false;
			for(int j=0;j<16;j++)
				if (hash_digest[j] != task->hash[j])
				{
					falsepositive = true;
					falsealarms++;
					break;
				}
			if (!falsepositive)
			{
				char found_text[1024];
				found_match = true;
				*(task->terminating) = true;
				sprintf(found_text, "Worker %d - Found: %s", task->WorkerID, hash_buf);
				CUIManager::getSingleton().PrintLn(-1, string(found_text), false);
				CUIManager::getSingleton().PrintLn(gid, string(found_text), false);
				break; //Found the real one so no need to check other possible matches
			}

		}
		if (found_match) break;
	}
	//CUIManager::getSingleton().UnregisterVariable(gid, progressname);
	CUIManager::getSingleton().UnregisterVariable(gid, falsealarmname);
}

CLookupJobPool::CLookupJobPool(): chain_min(0), chain_v(0), chain_max(0), chaindata_pointer(0)
{
	pthread_mutex_init(&mutex, NULL);
	prelookup = 1;
}

CLookupJobPool::~CLookupJobPool()
{
	CUIManager::getSingleton().UnregisterVariable(gid, "Progress");
	pthread_mutex_destroy(&mutex);
}

void CLookupJobPool::SetLookupJobs( int min, int max )
{
	this->chain_min = min;
	this->chain_max = max;
	chain_v =min;

	chaindata_pointer = 0;
	prelookup = 1;
	chaindata.clear();

	progress = 0;
	gid = CUIManager::getSingleton().CreateGroup("Lookup");
	CUIManager::getSingleton().UnregisterVariable(gid, "Progress");
	IntVariable* var = CUIManager::getSingleton().RegisterIntVariable("Progress", &progress, gid);
	var->max = (max-min)*(max+min)/2;
	var->min = 0;
	var->style = Progress;
}

int CLookupJobPool::GetNextChain()
{
	int r;
	pthread_mutex_lock(&mutex);
	if (chain_v>chain_max)
		r =  -1;
	else
		r =  chain_v++;
	progress += r;
	pthread_mutex_unlock(&mutex);
	return r;
}

int CLookupJobPool::GetNextChainHeads(CDiskFile& datafile, int* start_offset, Index_Type* start_indexes)
{
	if (prelookup)
		return 0;
	pair<int,Index_Type> *d = NULL;
	pthread_mutex_lock(&mutex);
	if (chaindata_pointer < chaindata.size())
	{
		d = &(chaindata[chaindata_pointer++]);
		progress +=  d->first;
	}
	pthread_mutex_unlock(&mutex);
	if (d == NULL)
	{
		return -1;
	}
	else
	{
		*start_offset = d->first;
		return datafile.Lookup(d->second, start_indexes);
	}
}

void CLookupJobPool::SubmitChainData( int chain_offset, Index_Type finish_hash )
{
	pthread_mutex_lock(&mutex);
	chaindata.push_back(pair<int,Index_Type>(chain_offset, finish_hash&hash_mask));
	if (chaindata.size() == chain_max - chain_min + 1)
	{
		sort(chaindata.begin(), chaindata.end(), ChainData_List_Pred);
		progress = 0;
		chaindata_pointer = 0;
		prelookup = 0;
	}
	pthread_mutex_unlock(&mutex);

}
