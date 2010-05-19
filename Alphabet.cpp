#define _CRT_SECURE_NO_WARNINGS
#include "Alphabet.h"
#include <string.h>

CAlphabet::CAlphabet(void)
{
}

CAlphabet::~CAlphabet(void)
{
}

bool CAlphabet::Initialise(string Description)
{
	description = Description;
	return Parse(Description);
}

int CAlphabet::Lookup(Index_Type index, char* result)
{
	int ind = 0;
	while ( (ind < alphabet_len) && (index >= alphabet[ind].wordcount) )
		index -= alphabet[ind++].wordcount;

	if (ind == alphabet_len) 
		return 0;

	int len = alphabet[ind].min_length;
	while (index >= alphabet[ind].wordcount_table[len])
		index -= alphabet[ind].wordcount_table[len++];

	unsigned int radix = alphabet[ind].charset_len;
	char* radixtable = alphabet[ind].charset;
	//Pin index to alphabet[ind] with length len
	for(int i=0;i<len;i++)
	{
		unsigned int x = index % radix;
		index = index / radix;
		result[i] = radixtable[x];
	}
	result[len] = '\0';
	return len;
}

Index_Type CAlphabet::GetAlphabetSize(void)
{
	Index_Type r = 0;
	for(int i=0;i<alphabet_len;i++)
		r += alphabet[i].wordcount;
	return r;
}

string CAlphabet::GetDescription(void)
{
	return string(description);
}

bool CAlphabet::Parse(string desc)
{
	string substr;
	
	alphabet_len = 0;
	while (desc.length() > 0)
	{
		int split = desc.find_first_of("|");
		if (split != string::npos)
		{
			substr = desc.substr(0, split);
			desc = desc.substr(split+1, desc.length() - split - 1);
		}
		else
		{
			substr = desc;
			desc = "";
		}
		const char* c = substr.c_str();

		if ((c[0] != '[') || (c[substr.length()-1] != '}')) return false;
		split = substr.find_first_of("]{");
		if (split == string::npos)return false;
		if (sscanf(substr.substr(split+2, substr.length()-split-2).c_str(), "%d,%d",
			&alphabet[alphabet_len].min_length, &alphabet[alphabet_len].max_length) != 2)
			return false;
		alphabet[alphabet_len].charset[0] = '\0';
		for(int i=1;i<split;i++)
			strcat(alphabet[alphabet_len].charset, LookupAbbreviation(substr[i]));
		
		//Filling missing gaps
		alphabet[alphabet_len].charset_len = strlen(alphabet[alphabet_len].charset);
		alphabet[alphabet_len].wordcount_table[0] = 1;
		for(int i=1;i<=alphabet[alphabet_len].max_length;i++)
			alphabet[alphabet_len].wordcount_table[i] = alphabet[alphabet_len].wordcount_table[i-1] * 
				(Index_Type) alphabet[alphabet_len].charset_len;

		alphabet[alphabet_len].wordcount = 0;
		for(int i=alphabet[alphabet_len].min_length; i<= alphabet[alphabet_len].max_length;i++)
			alphabet[alphabet_len].wordcount += alphabet[alphabet_len].wordcount_table[i];

		alphabet_len++;
	}

	InvididualAlphabet t;

	for(int i=0;i<alphabet_len;i++)
		for(int j=i+1;j<alphabet_len;j++)
			if (alphabet[i].wordcount < alphabet[j].wordcount)
			{
				t = alphabet[i];
				alphabet[i] = alphabet[j];
				alphabet[j] = t;
			}
	return true;
}

const char* CAlphabet::LookupAbbreviation(char c)
{
	switch (c)
	{
	case 'N': return "0123456789";
	case 'L': return "abcdefghijklmnopqrstuvwxyz";
	default: return "";
	}
}

int CAlphabet::GetSizeBits(void)
{
	int bits = 0;
	Index_Type size = GetAlphabetSize() - 1;
	while (size > 0 )
	{
		bits++;
		size /= 2;
	}
	return bits;
}

