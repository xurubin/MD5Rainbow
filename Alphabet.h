#pragma once
#include "common.h"
#include <string>
using namespace std;

struct InvididualAlphabet
{
	Index_Type wordcount;
	char charset[40];
	int charset_len;
	int min_length;
	int max_length;
	Index_Type wordcount_table[16]; 
};

class CAlphabet
{
public:
	CAlphabet(void);
	~CAlphabet(void);
	bool Initialise(string Description);
	int Lookup(Index_Type index, char* result);
	Index_Type GetAlphabetSize(void);
	string GetDescription(void);
private:
	string description;
	InvididualAlphabet alphabet[20];
	int alphabet_len;

public:
	bool Parse(string desc);
private:
	const char* LookupAbbreviation(char c);
public:
	int GetSizeBits(void);
};
