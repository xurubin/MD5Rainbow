#include "IndexFile.h"
#include "UIManager.h"
#include <sys/stat.h>
/*
CTableFile::CTableFile(void)
{
}

CTableFile::~CTableFile(void)
{
}

CPartialTableFile::CPartialTableFile(void)
{
}

CPartialTableFile::~CPartialTableFile(void)
{
}
*/
CIndexeFile::CIndexeFile(void)
{
}

CIndexeFile::~CIndexeFile(void)
{
}


bool CIndexeFile::LoadFile(string filename)
{
	char buf[1024];
	struct stat stats;
	stat(filename.c_str(), &stats);

	FILE* indexfile = fopen(filename.c_str(), "wb");
	fread(buf, sizeof(int), 3, indexfile);
	startindex_bits = ((int*)buf)[0] ;
	finishhash_bits = ((int*)buf)[1] ;
	fileoffset_bits = ((int*)buf)[2] ;
	int entry_bytes = BIT2BYTES(finishhash_bits+fileoffset_bits);

	int num_entries = (int)((stats.st_size-3*sizeof(int)) / entry_bytes);
	if ((stats.st_size-3*sizeof(int)) % entry_bytes != 0)
		CUIManager::getSingleton().PrintLn(-1, "Index file size incorrect!");

	IndexFile_Entry entry, prev_entry; 
	bool first = true;
	entries.clear();
	for(int i=0;i<num_entries;i++)
	{
		fread(buf, entry_bytes,1, indexfile);
		UnpackIndexEntry(entry, buf);
		if(!first)
		{
			entry.hash += prev_entry.hash;
			entry.offset += prev_entry.offset;
		}
		first = false;
		prev_entry = entry;
		entries.push_back(entry);
	}

	fclose(indexfile);
	return true;
}


bool CIndexeFile::SaveFile(string filename)
{
	Index_Type previous_hash = 0;
	bool first = true;
	char txtbuf[256];
	char buf[32];
	int TotalOffset=0;

	int FinishHash_BitLength = 0;
	int FileOffset_BitLength = 0;
	int bits;
	//CUIManager::getSingleton().RegisterIntVariable("_FinishHash_BitLength", &FinishHash_BitLength, gid);
	//CUIManager::getSingleton().RegisterIntVariable("_FileOffset_BitLength", &FileOffset_BitLength, gid);

	IndexFile_Entry previous_entry = entries.front();
	FinishHash_BitLength=getbits(previous_entry.hash);
	FileOffset_BitLength=getbits(previous_entry.offset);
	for(EntryList::iterator it = ++entries.begin(); it != entries.end(); it++)
	{
		IndexFile_Entry t = *it;
		it->hash -= previous_entry.hash;
		it->offset -= previous_entry.offset;

		bits = getbits(it->hash);
		if (bits > FinishHash_BitLength) FinishHash_BitLength = bits;
		bits = getbits(it->offset);
		if (bits > FileOffset_BitLength) FileOffset_BitLength = bits;

		previous_entry = t;
	}

	finishhash_bits = FinishHash_BitLength;
	fileoffset_bits = FileOffset_BitLength;

	//CUIManager::getSingleton().PrintLn(gid, "Writing out index file");

	FILE* indexfile = fopen(filename.c_str(), "wb");
	((int*)buf)[0] = startindex_bits;
	((int*)buf)[1] = finishhash_bits;
	((int*)buf)[2] = fileoffset_bits;
	fwrite(buf, sizeof(int), 3, indexfile);

	int entry_bytes = BIT2BYTES(finishhash_bits+fileoffset_bits);
	for(EntryList::iterator it = entries.begin(); it != entries.end(); it++)
	{
		PackIndexEntry(*it, buf);
		fwrite(buf, entry_bytes, 1, indexfile);
	}
	fclose(indexfile);

	//CUIManager::getSingleton().PrintLn(-1, "Create Index file done.");
	//sprintf(txtbuf, "StartIndex:%d FinishHash:%d FileOffset:%d", StartIndex_BitLength, FinishHash_BitLength, FileOffset_BitLength);
	//CUIManager::getSingleton().PrintLn(-1, txtbuf);
	//CUIManager::getSingleton().RemoveGroup(gid);

	entries.clear();
	return true;
}


int CIndexeFile::AddEntries(IndexFile_Entry* entries, int count)
{
	for(int i=0;i<count;i++)
		this->entries.push_back(entries[i]);
	return count;
}


int CIndexeFile::LookupEntries(Index_Type hash, int& offset)
{
	return 0;
}


int CIndexeFile::Clear(void)
{
	entries.clear();
	startindex_bits=finishhash_bits=fileoffset_bits=0;
	return 0;
}


int CIndexeFile::PackIndexEntry(IndexFile_Entry& entry, char* buf)
{
	unsigned long long d = entry.hash;
	d <<= fileoffset_bits;
	d |= entry.offset;
	int r = 0;
	while(d>0)
	{
		buf[0] = (char)(d & 0xFF);
		d >>= 8;
		buf++;
		r++;
	}
	return r;
}
int CIndexeFile::UnpackIndexEntry(IndexFile_Entry& entry, char* buf)
{
	int entry_bytes = BIT2BYTES(finishhash_bits+fileoffset_bits);
	unsigned long long d = 0;
	for(int i = entry_bytes-1; i>=0; i--)
	{
		d <<= 8;
		d |= (buf[i] & 0xFF);
	}
	entry.offset = (d & ((1L << fileoffset_bits)-1));
	entry.hash = ((d >> fileoffset_bits) & ((1L << finishhash_bits)-1));
	return entry_bytes;
}
