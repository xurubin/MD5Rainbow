#include "DiskFile.h"
#include <assert.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "UIManager.h"
#include <string.h>
#include <list>


CDiskFile::CDiskFile(void)
{
}

CDiskFile::~CDiskFile(void)
{
	for(int i=0;i<filecount;i++)
	{
		fclose(files[i].handle);
		pthread_mutex_destroy(&files[i].mutex);
	}
}

bool CDiskFile::LoadTable(string TableName)
{

	this->TableName = TableName;
	FILE* f = fopen(string(TableName+TABLEEXT).c_str(), "rb");
	if (f) //Entire Table is Ready
	{
		fclose(f);
		filecount = 1;
		files[0].type = IndexedFile;
		return LoadPartFile(0, string(TableName+TABLEEXT).c_str());
	} else //Enumerating all part files. 
	{
		filecount = 0;
		char tmpfile[1024];
		sprintf(tmpfile, "%s.part%d",string(TableName+TABLEEXT).c_str(), filecount);
		while ((f = fopen( tmpfile, "rb")) != 0)
		{
			fclose(f);
			if (!LoadPartFile(filecount++, tmpfile))
				return false;
			sprintf(tmpfile, "%s.part%d",string(TableName+TABLEEXT).c_str(), filecount);
		}
	}


	return true;
}

bool CDiskFile::IsCompletedSorted(void)
{
	for(int i=0;i<filecount;i++)
		if (!files[i].Sorted)
			return false;
	return true;
}

int CDiskFile::AddContent(ChainData* data, int count)
{
	int i = 0;
	int result = 0;
	while( (count > 0) )
	{
		if (i < filecount)
		{
			if (files[i].NumEntries < perfileentry_limit)
			{
				int add_count = perfileentry_limit - files[i].NumEntries;
				if (add_count > count ) add_count = count;
				if (WriteToFile(i, data, add_count))
				{
					data += add_count;
					result += add_count;
					count -= add_count;
				}
			}
			i++;
		}else  // Create a new part file
		{
			char tmpfile[1024];
			FILE* f;
			sprintf(tmpfile, "%s.part%d",string(TableName+TABLEEXT).c_str(), i);
			f = fopen(tmpfile, "a+b");
			files[i].FileName = string(tmpfile);
			files[i].handle = f;
			files[i].ReadOnly = false;
			files[i].Sorted = false;
			files[i].NumEntries = 0;
			pthread_mutex_init(&files[i].mutex, 0);
			int add_count = (count > perfileentry_limit) ? perfileentry_limit : count;
			if (WriteToFile(i, data, add_count))
			{
				result += add_count;
				data += add_count; 
				count -= add_count;
			}
			i++;
			filecount++;
		}
	}
	return result;
}

int CDiskFile::GetEntriesCount(void)
{
	int count = 0;
	for(int i=0;i<filecount;i++)
		count += files[i].NumEntries;
	return count;
}

int CDiskFile::Lookup(Index_Type key, Index_Type* startindex)
{
	struct timespec deltatime;
	deltatime.tv_sec = 0;
	deltatime.tv_nsec = 50*1000*1000;
	int result = 0;

	key = key & hash_mask;
	for(int i=0;i<filecount;i++)
	{
		//Acquire mutex
		while (pthread_mutex_timedlock(&files[i].mutex, &deltatime) != 0) ;

		ChainData chain;
		char buf[sizeof(ChainData)*2];
		int st, ed;
		if (files[i].Sorted) //Binary Search
		{
			st = 0;
			ed = files[i].NumEntries - 1;
			while(st <= ed)
			{
				int cur = (st+ed)/2;
				fseek(files[i].handle, cur*BytesPerEntry, SEEK_SET);
				fread(buf, BytesPerEntry, 1, files[i].handle);
				UnpackChainData(buf, &chain);
				if (key < chain.FinishHash)
					ed = cur-1;
				else if (key > chain.FinishHash)
					st =cur+1;
				else
				{
					//Store found chain
					startindex[result++] = chain.StartIndex;
					//Search backwards
					st = cur-1;
					while(st>=0)
					{
						fseek(files[i].handle, st*BytesPerEntry, SEEK_SET);
						fread(buf, BytesPerEntry, 1, files[i].handle);
						UnpackChainData(buf, &chain);
						if (key == chain.FinishHash)
						{
							startindex[result++] = chain.StartIndex;
							st--;
						}
						else
							break;
					}
					//Search forwards
					ed = cur+1;
					while(ed<=files[i].NumEntries - 1)
					{
						fseek(files[i].handle, ed*BytesPerEntry, SEEK_SET);
						fread(buf, BytesPerEntry, 1, files[i].handle);
						UnpackChainData(buf, &chain);
						if (key == chain.FinishHash)
						{
							startindex[result++] = chain.StartIndex;
							ed++;
						}
						else
							break;
					}
					break; //Done with this file, break out binary search loop
				}
			}
		}
		else //Sequential Search
		{
			fseek(files[i].handle, 0, SEEK_SET);
			for(int j=0;j<files[i].NumEntries;j++)
			{
				fread(buf, BytesPerEntry, 1, files[i].handle);
				UnpackChainData(buf, &chain);
				if (chain.FinishHash == key)
				{	
					startindex[result++] = chain.StartIndex;
				}
			}
		}

		//Release mutex
		pthread_mutex_unlock(&files[i].mutex);
	}
	return result;
}

bool CDiskFile::IsFileSorted(int index)
{
	int n = files[index].NumEntries;
	fseek(files[index].handle, 0, SEEK_SET);
	char p0[sizeof(Index_Type)*2], p1[sizeof(Index_Type)*2];

	fread(p0, 1, BytesPerEntry, files[index].handle);
	n--;
	int m;
	for(int i=0; i<CHECKSORT_ITERATIONS; i++)
	{
		m = (int) ((double)rand()  / (double)RAND_MAX * (double)n / CHECKSORT_ITERATIONS);
		fseek(files[index].handle, BytesPerEntry*m, SEEK_CUR);
		n -= m;
		if (n <= 0) break;
		fread(p1, 1, BytesPerEntry, files[index].handle);
		if (ComparePackedChainData(p0, p1) >= 0)
			return false;
		for(int j=0;j<sizeof(p0);j++) p0[j] = p1[j];
		n -= 1;
	}
	return true;
}

int CDiskFile::UnpackChainData(char* buf , ChainData* chaindata)
{
	int bytes = BytesPerEntry/2 - 1;
	int extra_bits = BitsPerIndex - bytes * 8;
	chaindata->StartIndex = (*(Index_Type*)buf) & index_mask;
	chaindata->FinishHash = ((*(Index_Type*)(buf+bytes)) >> extra_bits) & hash_mask;
	return BytesPerEntry;
}

int CDiskFile::PackChainData(ChainData* chaindata, char* buf)
{
	*(Index_Type*)buf = chaindata->StartIndex;
	int bytes = BytesPerEntry/2 - 1;
	int extra_bits = BitsPerIndex - bytes * 8;
	*(Index_Type*)(buf+bytes) = (chaindata->StartIndex >> (8*bytes))
		| ( (chaindata->FinishHash&hash_mask)  << extra_bits);
	return BytesPerEntry;
}

bool CDiskFile::LoadPartFile(int index, const char* filename)
{
	FILE* f = fopen(filename, "a+b");
	if (f == NULL) 
		return false;
	files[index].FileName = string(filename);
	files[index].handle = f;
	files[index].ReadOnly = true;
	files[index].type = PartialFile;
/*
	struct __stat64 fstat; 
	_fstat64(_fileno(f), &fstat);
	files[index].NumEntries = (int)(fstat.st_size / BytesPerEntry);
*/
	struct stat stats;
	stat(filename, &stats);
	files[index].NumEntries = (int)(stats.st_size / BytesPerEntry);
	assert(stats.st_size % BytesPerEntry == 0);

	files[index].Sorted = IsFileSorted(index);
	//assert(files[index].Sorted);

	pthread_mutex_init(&files[index].mutex, 0);

	return true;
}

bool CDiskFile::WriteToFile(int index, ChainData* data, int count)
{
	char buf[16384];
	int buf_len;
	int count2 = count;
	while(count>0)
	{
		int size = PackChainData(data, buf);
		buf_len = size;
		count--;
		for(unsigned int i=1;i<=sizeof(buf)/size;i++)
		{
			if (count <= 0) break;
			buf_len += PackChainData(data+i, buf+size*i);
			count--;
		}
		fwrite(buf, 1, buf_len, files[index].handle);

	}
	files[index].NumEntries+=count2;
	fflush(files[index].handle);
	return true;
}

int CDiskFile::GetBytesPerEntry( void )
{
	return BytesPerEntry;
}
void CDiskFile::SetAlphabetSize(Index_Type size)
{
	BitsPerIndex = 0;
	size -= 1;
	while (size > 0 )
	{
		BitsPerIndex++;
		size /= 2;
	}
	BytesPerEntry = (BitsPerIndex*2+7)/8;
	index_mask = ((Index_Type)1 << BitsPerIndex) - 1;
	hash_mask = ((Index_Type)1 << (BytesPerEntry*8 - BitsPerIndex)) - 1;
	
	perfileentry_limit = MAXSIZE_TMPTABLE_GUIDELINE /BytesPerEntry;
	perfilesize_limit = perfileentry_limit * BytesPerEntry;

}

bool CDiskFile::IsMergedFilesValid(void)
{
	return true;
}
void CDiskFile::MergeSortAll(void)
{
	int i;
	char message[256];
	CUIManager::getSingleton().PrintLn(-1, "Sorting individual part files...");
	//Sort individual part files
	for(i=0;i<filecount;i++)
	{
		if (files[i].Sorted)
			continue;
		sprintf(message, "Sorting part file %d/%d..", i+1, filecount);
		CUIManager::getSingleton().PrintLn(-1, message);
		SortFile(i);
		assert(files[i].Sorted == IsFileSorted(i));
	}
	//Merging
	CUIManager::getSingleton().PrintLn(-1, "Merging files...");
	char *inbuf[64];
	int inbuf_entries[64],inbuf_pointers[64];
	char *outbuf;
	int outbuf_entries = 0;
	int buf_maxitems = MERGESORT_MEMORY / (BytesPerEntry*(filecount+1));

	FILE* outfile = NULL;
	int outfile_maxcapacity = MAXSIZE_TMPTABLE_GUIDELINE / BytesPerEntry;
	int outfile_remaincapacity = 0;
	int outfile_i = 0;

	char txtbuf[1024];

	outbuf = new char[buf_maxitems*BytesPerEntry];
	for(i=0;i<filecount;i++)
	{
		inbuf[i] = new char[buf_maxitems*BytesPerEntry];
		inbuf_entries[i] = inbuf_pointers[i] = 0;
		fseek(files[i].handle, 0, SEEK_SET);
	}
	bool notfinished;
	do 
	{
		notfinished = false;
		//First step - fill in empty inbuf
		for(i=0;i<filecount;i++)
			if (inbuf_pointers[i] >= inbuf_entries[i])
			{
				inbuf_pointers[i] = 0;
				inbuf_entries[i] = fread(inbuf[i], BytesPerEntry, buf_maxitems, files[i].handle);
				if (inbuf_entries[i])
					notfinished = true;
			}
		//Second step - write out and empty outbuf
		if(outbuf_entries > 0)
		{
			char* saved_outbuf = outbuf;
			do //Loop: write to outfile
			{
				int tobeoutput = (outbuf_entries < outfile_remaincapacity) ? outbuf_entries : outfile_remaincapacity;
				if (outfile != NULL)
				fwrite(outbuf, BytesPerEntry, tobeoutput, outfile);

				outfile_remaincapacity -= tobeoutput;
				if (outfile_remaincapacity == 0)
				{
					outfile_remaincapacity = outfile_maxcapacity;
					sprintf(txtbuf, "%s.merged%d",string(TableName+TABLEEXT).c_str(), outfile_i++);
					if(outfile != NULL)
						fclose(outfile);
					outfile = fopen(txtbuf, "wb");
					sprintf(message, "Producing %s..", txtbuf);
					CUIManager::getSingleton().PrintLn(-1, message);
				}
				outbuf_entries -= tobeoutput;
				outbuf += tobeoutput*BytesPerEntry;
			}while(outbuf_entries > 0);
			outbuf = saved_outbuf;
		}
		//Third step - merge until some inbuf is empty or outbuf is full
		bool cancontinue=true;
		do //Loop: Merge in_buf into out_buf
		{
			int min_index = -1;
			//bool some_inbuf_empty = false;
			for(int i=0;i<filecount;i++)
				if (inbuf_pointers[i]<inbuf_entries[i])
				{
					if (min_index == -1)
						min_index = i;
					else if (ComparePackedChainData(inbuf[i]+BytesPerEntry*inbuf_pointers[i], inbuf[min_index]+BytesPerEntry*inbuf_pointers[min_index]) < 0)
						min_index = i;
				}/*else //inbuf[i] is empty 
					cancontinue = false;*/
			if( min_index == -1)
				cancontinue = false;
			else
			{
				memmove(outbuf+outbuf_entries*BytesPerEntry, inbuf[min_index]+BytesPerEntry*inbuf_pointers[min_index], BytesPerEntry);
				outbuf_entries++;
				inbuf_pointers[min_index]++;
				if(inbuf_pointers[min_index] >= inbuf_entries[min_index])
					cancontinue = false;
			}
			if (outbuf_entries>= buf_maxitems)
				cancontinue = false;

		} while (cancontinue); //Merge buffers until some inbuf is empty or oub_buf is full.
		//Finally - check if we need to loop again (exist inbuf_file != nil | outbuf != nil)
		notfinished = notfinished || (outbuf_entries>0);
	} while (notfinished);

	if(outfile)
		fclose(outfile);
	delete[] outbuf;
	for(int i=0;i<filecount;i++)
		delete inbuf[i];

	CUIManager::getSingleton().PrintLn(-1, "MergeSort done.");
}

int CDiskFile::ComparePackedChainData(char* c1, char* c2)
{
	int bytes = BytesPerEntry/2 - 1;
	int extra_bits = BitsPerIndex - bytes * 8;
	Index_Type h1 = ((*(Index_Type*)(c1+bytes)) >> extra_bits) & hash_mask;
	Index_Type h2 = ((*(Index_Type*)(c2+bytes)) >> extra_bits) & hash_mask;
	if (h1 > h2)
		return 1;
	else if (h1 < h2)
		return -1;
	else
	{	
		Index_Type i1 = *(Index_Type*)c1;
		Index_Type i2 = *(Index_Type*)c2;
		if (i1 > i2)
			return 1;
		else if (i1 < i2)
			return -1;
		else
			return 0;
	}
}

void CDiskFile::SortFile(int index)
{
	char *buf = new char[files[index].NumEntries * BytesPerEntry];
	
	fseek(files[index].handle, 0, SEEK_SET);
	fread(buf, BytesPerEntry, files[index].NumEntries, files[index].handle);
	fclose(files[index].handle);

	qsort(buf, files[index].NumEntries, BytesPerEntry, (int (*)(const void *,const void *))CDiskFile::ComparePackedChainData);
	
	//Now remove duplicate
	int i=0,j, k, duplicates = 0;
	for(j=1;j<files[index].NumEntries;j++)
	{
		bool duplicated = (*(int*)(buf+i*BytesPerEntry) == *(int*)(buf+j*BytesPerEntry));
		if (duplicated)
			for(k=sizeof(int);k<BytesPerEntry;k++)
				if (buf[i*BytesPerEntry+k] != buf[j*BytesPerEntry+k])
				{
					duplicated = false;
					break;
				}
		if (!duplicated) 
		{
			i++;
			if (i != j)
				for(k=0;k<BytesPerEntry;k++) 
					buf[i*BytesPerEntry+k] = buf[j*BytesPerEntry+k];
		}else
		{
			duplicates++;
		}
	}
	if (files[index].NumEntries == i+1)
		files[index].handle = fopen(files[index].FileName.c_str(), "r+b");
	else
	{
		printf("Remove %d duplicates from %s\n", duplicates, files[index].FileName.c_str());
		files[index].handle = fopen(files[index].FileName.c_str(), "w+b");
		files[index].NumEntries = i+1;
	}
	fseek(files[index].handle, 0, SEEK_SET);
	fwrite(buf, BytesPerEntry, files[index].NumEntries, files[index].handle);
	fflush(files[index].handle);
	
	files[index].Sorted = true;
	delete[] buf;
}

int CDiskFile::BytesPerEntry;
int CDiskFile::BitsPerIndex;
Index_Type CDiskFile::hash_mask;
Index_Type CDiskFile::index_mask;

void CDiskFile::Test(void)
{
	int min_bit_difference = 40;
	int max_bit_difference = -1;
	int different_hashes = 0;
	int dup_distribution[100];
	ChainData r1, r2;
	char buf[32];
	char txtbuf[256];
	int i,j;
	memset(dup_distribution, 0, sizeof(dup_distribution));
	int gid = CUIManager::getSingleton().CreateGroup("Test");
	CUIManager::getSingleton().RegisterIntVariable("Max bit diff", &max_bit_difference, gid);
	CUIManager::getSingleton().RegisterIntVariable("files", &i, gid);
	IntVariable* j_var = CUIManager::getSingleton().RegisterIntVariable("entries", &j, gid);
	r1.FinishHash = 0;
	int dup = 1;
	for(i=0;i<filecount;i++)
	{
		if (!files[i].Sorted) continue;
		j_var->min = 0;
		j_var->max = files[i].NumEntries;
		j_var->style = Progress;
		fseek(files[i].handle, 0, SEEK_SET);
		for(j=0;j<files[i].NumEntries;j++)
		{
			fread(buf, BytesPerEntry, 1, files[i].handle);
			UnpackChainData(buf, &r2);
			if (r1.FinishHash != r2.FinishHash)
			{
				different_hashes++;
				if (dup >= sizeof(dup_distribution))
					CUIManager::getSingleton().PrintLn(-1, "dup overflow");

				dup_distribution[dup]++;
				dup = 1;
				if (r1.FinishHash > r2.FinishHash)
				{
					sprintf(txtbuf, "Invalid sorting: %d/%d %.16lx %.16lx", i, j, r1.FinishHash, r2.FinishHash);
					CUIManager::getSingleton().PrintLn(gid, txtbuf);
				}
				Index_Type dif = r2.FinishHash - r1.FinishHash;
				int bits = 0;
				while (dif > 0) {bits++; dif /= 2;}
				if (bits > max_bit_difference)
				{
					//_ui64toa(r1.FinishHash, buf, 16);
					//sprintf(txtbuf,"New max diff: %s, ", buf);
					//_ui64toa(r2.FinishHash, buf, 16);
					//strcat(txtbuf,buf);
					sprintf(txtbuf, "New max diff: %.16lx %.16lx", r1.FinishHash, r2.FinishHash);
					CUIManager::getSingleton().PrintLn(gid, txtbuf);
					if (max_bit_difference == -1)
						max_bit_difference = 0;
					else
						max_bit_difference = bits;
				}
				
				bits = 0;
				dif = r2.FinishHash ^ r1.FinishHash;
				while (dif > 0) {bits++; dif /= 2;}
				if (bits < min_bit_difference)
				{
					sprintf(txtbuf, "New min diff: %.16lx %.16lx", r1.FinishHash, r2.FinishHash);
					CUIManager::getSingleton().PrintLn(gid, txtbuf);
					min_bit_difference = bits;
				}

			}
			else
				dup++;
			r1 = r2;
		}
	}
	txtbuf[0] = '\0';
	for(i=0;i<42;i++)
	{
		char num[32];
		sprintf(num, "%8d, ", dup_distribution[i]);
		strcat(txtbuf, num);
		if (i % 6 ==5) 
		{
			CUIManager::getSingleton().PrintLn(-1, txtbuf);
			txtbuf[0] = '\0';
		}
	}
	sprintf(txtbuf,"Test:diff_hashes %d, max_bit_diff %d, min_bit_diff %d", different_hashes, max_bit_difference, min_bit_difference);
	CUIManager::getSingleton().PrintLn(-1, txtbuf);
	CUIManager::getSingleton().RemoveGroup(gid);
}


void CDiskFile::CreateIndexedDatabase(void)
{
	Index_Type previous_hash = 0;
	bool first = true;
	ChainData chain;
	char txtbuf[256];
	char buf[32];
	int i, TotalOffset=0;
	unsigned int IntBuf;
	int StartIndex_BitLength = 0;
	list<Index_Type> startindexes;
	IndexFile_Entry new_entry;

	int gid = CUIManager::getSingleton().CreateGroup("Indexing");
	IntVariable* i_var = CUIManager::getSingleton().RegisterIntVariable("Progress", &i, gid);

	// Prepare out files
	sprintf(txtbuf, "%s",string(TableName+TABLEEXT).c_str());
	FILE* tablefile = fopen(txtbuf, "wb");

	CIndexeFile& indexfile = files[0].indexfile;
	indexfile.Clear();

	for(int id=0;id<filecount;id++)
	{
		sprintf(txtbuf, "Indexing:Processing part file %d/%d", id+1,filecount);
		CUIManager::getSingleton().PrintLn(gid, txtbuf);
		i_var->min = 0;
		i_var->max = files[id].NumEntries;
		i_var->style = Progress;

		fseek(files[id].handle, 0, SEEK_SET);

		for(i=0;i<files[id].NumEntries;i++)
		{
			if (fread(buf, BytesPerEntry, 1, files[id].handle) != 1)
				CUIManager::getSingleton().PrintLn(-1, "Indexing: ReadFile failed");
			UnpackChainData(buf, &chain);

			// Write out StartIndex_BitLength of chain.StartIndex
			if(getbits(chain.StartIndex) > 32)
				CUIManager::getSingleton().PrintLn(-1, "Fatal: StartIndex in the partfile is too big.");
			IntBuf = chain.StartIndex & 0xFFFFFFFF;
			fwrite(&IntBuf, 4, 1, tablefile);
			StartIndex_BitLength =32;

			if (first || (chain.FinishHash > previous_hash))
			{
				first = false;
				new_entry.offset = TotalOffset+i;
				new_entry.hash   = chain.FinishHash;
				previous_hash = chain.FinishHash;

				indexfile.AddEntries(&new_entry, 1);
			}else if (chain.FinishHash < previous_hash)
			{
				sprintf(txtbuf, "Indexing: Unsorted input file(part%d) %lx %lx", id, previous_hash, chain.FinishHash);
				CUIManager::getSingleton().PrintLn(gid, txtbuf);
			}
		}
		TotalOffset += files[id].NumEntries;
	}
	sprintf(txtbuf, "Indexing:Loaded %d entries", startindexes.size());
	CUIManager::getSingleton().PrintLn(gid, txtbuf);

	indexfile.SetStartIndexBits(StartIndex_BitLength);
	indexfile.SaveFile(string(TableName+TABLEEXT+".index").c_str());

	fclose(tablefile);
	CUIManager::getSingleton().PrintLn(-1, "Create Indexed file done.");
	//sprintf(txtbuf, "StartIndex:%d FinishHash:%d FileOffset:%d", StartIndex_BitLength, FinishHash_BitLength, FileOffset_BitLength);
	//CUIManager::getSingleton().PrintLn(-1, txtbuf);
	CUIManager::getSingleton().RemoveGroup(gid);
}
