#pragma once
#include "common.h"
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string>
#include "pthread.h"
#include "IndexFile.h"

using namespace std;

enum PartFileType
{
	PartialFile,
	IndexedFile
};
struct PartFile
{
	string FileName;
	FILE* handle;
	bool Sorted;
	bool ReadOnly;
	int NumEntries;
	PartFileType type;
	pthread_mutex_t mutex;
	CIndexeFile indexfile;
};


class CDiskFile
{
public:
	CDiskFile(void);
	~CDiskFile(void);
	bool LoadTable(string TableName);
	bool IsCompletedSorted(void);
	int AddContent(ChainData* data, int count);
	int GetEntriesCount(void);
	int Lookup(Index_Type key, Index_Type* startindex);
private:
	static int BytesPerEntry;
	static int BitsPerIndex;
	static Index_Type index_mask;
	static Index_Type hash_mask;
	string TableName;
	PartFile files[500];
	int filecount;
	int perfileentry_limit;
	int perfilesize_limit;
	bool IsFileSorted(int index);
	int UnpackChainData(char* buf , ChainData* chaindata);
	int PackChainData(ChainData* chaindata, char* buf);
public:
	bool LoadPartFile(int index, const char* filename);
	int GetBytesPerEntry( void );
private:
	bool WriteToFile(int index, ChainData* data, int count);
public:
	void SetAlphabetSize(Index_Type size);
	void MergeSortAll(void);
	bool IsMergedFilesValid(void);
	static int ComparePackedChainData(char* c1, char* c2);
private:
	void SortFile(int index);
public:
	void Test(void);
	void CreateIndexedDatabase(void);
	int Lookup_PartFile(int i, Index_Type key, Index_Type* startindex);
	int Lookup_IndexedFile(int i, Index_Type key, Index_Type* startindex);
};
