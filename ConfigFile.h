#pragma once
#include <string>
#include <map>
using namespace std;
class CConfigFile
{
public:
	CConfigFile(void);
	~CConfigFile(void);
	bool LoadFile(string filename);
	bool SaveFile(string filename);
	void Clear();
	void SetStr(string KeyName, string Value);
	void SetInt(string KeyName, int Value);
	string GetStr(string Key);
	int GetInt(string Key);
private:
	map<string, string> data;
};
