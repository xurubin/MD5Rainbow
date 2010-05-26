#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <string>
#include "common.h"
#include <vector>
using namespace std;

struct IndexFile_Entry 
{
	unsigned long int offset;
	Index_Type hash;
};

typedef vector<IndexFile_Entry> EntryList;
class CIndexeFile
{
public:
	CIndexeFile(void);
	~CIndexeFile(void);
	bool LoadFile(string filename);
	bool SaveFile(string filename);
	int Clear(void);
	int AddEntries(IndexFile_Entry* entries, int count);
	int LookupEntries(Index_Type hash, int& offset);

	void SetStartIndexBits(int bits) { startindex_bits = bits;}
	int GetStartIndexBits() { return startindex_bits;}
	int GetNumEntries() { return entries.size(); }
private:
	int PackIndexEntry(IndexFile_Entry& entry, char* buf);
	int UnpackIndexEntry(IndexFile_Entry& entry, char* buf);
	EntryList entries;
	int finishhash_bits;
	int fileoffset_bits;
	int startindex_bits;
};
