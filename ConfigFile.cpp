#define _CRT_SECURE_NO_WARNINGS
#include "ConfigFile.h"
#include "stdio.h"
#include "stdlib.h"
CConfigFile::CConfigFile(void) : data()
{
}

CConfigFile::~CConfigFile(void)
{
}

bool CConfigFile::LoadFile(string filename)
{
	FILE* f;
	f = fopen(filename.c_str(), "rt");
	if (f == NULL)
		return false;
	char buf[4096];
	data.clear();
	while(!feof(f))
	{
		fgets(buf, sizeof(buf), f); 
		string line(buf);
		if (line.length() < 2) continue;
		if ((line[line.length()-1] == '\r') || (line[line.length()-1] == '\n')  )
			line = line.substr(0, line.length() - 1);
		int split = line.find_first_of('=');
		data[line.substr(0, split)] = line.substr(split+1, line.length() - split - 1);
	}
	fclose(f);
	return true;
}

bool CConfigFile::SaveFile(string filename)
{
	map<string,string>::iterator iter;
	FILE* f = fopen(filename.c_str(), "wt");
	if (f == NULL)
		return false;
	for(iter = data.begin(); iter != data.end(); iter++)
	{
		fprintf(f, "%s=%s\n", iter->first.c_str(), iter->second.c_str());
	}
	fclose(f);
	return true;
}

void CConfigFile::Clear()
{
	data.clear();
}
void CConfigFile::SetStr(string KeyName, string Value)
{
	data[KeyName] = Value;
}

string CConfigFile::GetStr(string Key)
{
	map<string,string>::iterator iter = data.find(Key);
	if (iter == data.end())
		return string("");
	else
		return string(iter->second);
}

void CConfigFile::SetInt(string KeyName, int Value)
{
	char buf[127];
	sprintf(buf,"%d", Value);
	SetStr(KeyName, buf);
}
int CConfigFile::GetInt(string Key)
{
	return atoi(GetStr(Key).c_str());
}
