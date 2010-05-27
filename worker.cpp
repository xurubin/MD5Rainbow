#define _CRT_SECURE_NO_WARNINGS
#include "worker.h"
#include "pthread.h"
#include "HashReduce.h"
#ifdef WIN32
#include "windows.h"
#endif
#include "UIManager.h"
#include <string.h>
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
				for(unsigned int j=1;j<task->table->ChainLength;j++)
				{
					index = CMD5HashReduce::Reduce(hash_digest, j) % hash_range;
					*(int *)(hash_buf+0) = 0; //Clear only the first 16 bytes, should be enough
					*(int *)(hash_buf+4) = 0;
					*(int *)(hash_buf+8) = 0;
					*(int *)(hash_buf+12) = 0;
					str_len = task->table->alphabet.Lookup(index, hash_buf);
					hash_buf[str_len] = (char)0x80;
					*(int *)(hash_buf+56) = str_len*8;
					CMD5HashReduce::Hash_One_Block(hash_buf, hash_digest);
/*DBGPRINT*/
				}
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
	for(int i=0;i<sizeof(hash_buf);i++) hash_buf[i] = 0;


#ifdef WIN32
	SetThreadPriority(GetCurrentThread(), THREAD_BASE_PRIORITY_IDLE);
#endif
	int gid = task->UIGroup;
	int progress = 0;
	char varname[20]; sprintf(varname, "Progress%d", task->WorkerID);
	IntVariable* visualiser = CUIManager::getSingleton().RegisterIntVariable(varname, &progress, gid);
	visualiser->min = 0;
	visualiser->max = (task->EndChainLen+task->StartChainLen)*(task->EndChainLen-task->StartChainLen)/2;
	visualiser->style = Progress;

	for(chain_position=task->StartChainLen; chain_position<=task->EndChainLen; chain_position++)
	{
		progress+= chain_position;
		if (*(task->terminating))
			break;
		for(int j=0;j<sizeof(hash_digest);j++) hash_digest[j] = task->hash[j];
		for(int j=0;j<chain_position;j++)
		{
			index = CMD5HashReduce::Reduce(hash_digest, task->table->ChainLength-chain_position+j) % hash_range;
			*(int *)(hash_buf+0) = 0; //Clear only the first 16 bytes, should be enough
			*(int *)(hash_buf+4) = 0;
			*(int *)(hash_buf+8) = 0;
			*(int *)(hash_buf+12) = 0;
			str_len = task->table->alphabet.Lookup(index, hash_buf);
			hash_buf[str_len] = (char)0x80;
			*(int *)(hash_buf+56) = str_len*8;
			CMD5HashReduce::Hash_One_Block(hash_buf, hash_digest);
		}
		
		bool found_match = false;
		//Check if there is a possible match
		int matches = task->table->datafile.Lookup(*(Index_Type*)hash_digest, heads);
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
			for(unsigned int j=1;j<task->table->ChainLength-chain_position;j++)
			{
				index = CMD5HashReduce::Reduce(hash_digest, j) % hash_range;
				*(int *)(hash_buf+0) = 0; //Clear only the first 16 bytes, should be enough
				*(int *)(hash_buf+4) = 0;
				*(int *)(hash_buf+8) = 0;
				*(int *)(hash_buf+12) = 0;
				str_len = task->table->alphabet.Lookup(index, hash_buf);
				hash_buf[str_len] = (char)0x80;
				*(int *)(hash_buf+56) = str_len*8;
				CMD5HashReduce::Hash_One_Block(hash_buf, hash_digest);
			}


			//Check if it's a false positive
			bool falsepositive = false;
			for(int j=0;j<16;j++)
				if (hash_digest[j] != task->hash[j])
				{
					falsepositive = true;
					break;
				}
			if (!falsepositive)
			{
				hash_buf[str_len] = 0;
				char found_text[1024];
				found_match = true;
				*(task->terminating) = true;
				sprintf(found_text, "Worker %d - Found: %s\n", task->WorkerID, hash_buf);
				CUIManager::getSingleton().PrintLn(-1, string(found_text), false);
				CUIManager::getSingleton().PrintLn(gid, string(found_text), false);
				break; //Found the real one so no need to check other possible matches
			}

		}
		if (found_match) break;
	}
	CUIManager::getSingleton().UnregisterVariable(gid, varname);
}
