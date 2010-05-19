#pragma once
#include "pthread.h"
#include <vector>
#include <deque>
#include <map>
//#include <pair>
#include <string>
using namespace std;


class Variable
{
public:
	Variable(void* variable);
	virtual int Draw(double interval, int maxlen, char * Out);
protected:
	void* var;
};

enum VariableDisplayStyle { Raw, Progress, Gradient};

class IntVariable : public Variable
{
public:
	IntVariable(void* variable);
	virtual int Draw(double interval, int maxlen,  char * Out);
	VariableDisplayStyle style;
	int min,max;
private:
	int oldvalue;
};
class DoubleVariable : public Variable
{
public:
	DoubleVariable(void* variable);
	virtual int Draw(double interval, int maxlen,  char * Out);
};

typedef deque<string> StringList;
typedef map<string, Variable*> VariableCollection;
typedef struct {
	VariableCollection variables;
	string name;
	int height;
	StringList logs;
} VariableGroup;
typedef map<int, VariableGroup> GroupCollection;
class CUIManager
{
public:
	CUIManager(void);
	~CUIManager(void);
	static CUIManager& getSingleton(void);
private:
	static CUIManager* instance;
	GroupCollection groups;
	FILE* console_out;
	int max_handle;
	pthread_mutex_t mutex;
	int group_height;
	double refresh_interval;
	int allocate_handle(void);
	void Internal_Handler(void);
	void PrintVariableLine(VariableCollection::iterator var, double interval);
	void PrintLogLine( StringList::iterator log );
public:
	static void Handler(void);
	
	int CreateGroup(string name);
	void ClearGroup(int id);
	int RemoveGroup(int id);
	Variable* RegisterVariable(string name, int group, Variable* v);
	IntVariable* RegisterIntVariable(string name, int* var, int group);
	DoubleVariable* RegisterDoubleVariable(string name, double* var, int group);
	void UnregisterVariable(int group, string name);

	void PrintLn(int group, string line, bool raw = true);
	void ClearLogLines(int group);
};
