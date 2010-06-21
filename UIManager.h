#pragma once
#include "pthread.h"
#include <vector>
#include <deque>
#include <set>
#include <map>
//#include <pair>
#include <string>
using namespace std;
#ifdef WIN32
#include <Windows.h>
#endif

class Variable
{
public:
	Variable(volatile void* variable);
	virtual int DrawConsole(double interval, int maxlen, char * Out)=0;
#ifdef WIN32
	virtual int DrawWindow(double interval, int x, int y, int width, HDC dc)=0;
#endif
protected:
	volatile void* var;
};

enum VariableDisplayStyle { Raw, Progress, Gradient};

class IntVariable : public Variable
{
public:
	IntVariable(volatile void* variable);
	virtual int DrawConsole(double interval, int maxlen, char * Out);
#ifdef WIN32
	virtual int DrawWindow(double interval, int x, int y, int width, HDC dc);
#endif
	VariableDisplayStyle style;
	int min,max;
private:
	int oldvalue;
};
class DoubleVariable : public Variable
{
public:
	DoubleVariable(volatile void* variable);
	virtual int DrawConsole(double interval, int maxlen, char * Out);
#ifdef WIN32
	virtual int DrawWindow(double interval, int x, int y, int width, HDC dc);
#endif
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

class CacheVisualiser
{
public:
	int range_min, range_max;
	int* data;
	int segments;
	int numaccesses;
	int numhits;
	set<int> cacheset;
	deque<int> cachequeue;
	long timetag;
	CacheVisualiser() : segments(5000), range_min(0), range_max(0), data(0), numhits(0), numaccesses(0) { 
		data = new int[segments];for(int i=0;i<segments;i++) data[i] = 0;
	}
	~CacheVisualiser() { delete[] data;}
	void Hit(int position);
	void Damp(long now);
};
class CUIManager
{
public:
	CUIManager(void);
	~CUIManager(void);
	static CUIManager& getSingleton(void);
	static CacheVisualiser cache;
protected:
	static CUIManager* instance;
	GroupCollection groups;
	FILE* console_out;
	int max_handle;
	pthread_mutex_t mutex;
	int group_height;
	double refresh_interval;
	int allocate_handle(void);
	virtual void Internal_Handler(void) = 0;
public:
	static void Handler(void);
	
	int CreateGroup(string name);
	void ClearGroup(int id);
	int RemoveGroup(int id);
	Variable* RegisterVariable(string name, int group, Variable* v);
	IntVariable* RegisterIntVariable(string name, volatile int* var, int group);
	DoubleVariable* RegisterDoubleVariable(string name, volatile double* var, int group);
	void UnregisterVariable(int group, string name);

	void PrintLn(int group, string line, bool raw = true);
	void ClearLogLines(int group);

	virtual string InputString(string prompt) = 0;
};

class CConsoleUIManager : public CUIManager
{
protected:
	void PrintVariableLine(VariableCollection::iterator var, double interval);
	void PrintLogLine( StringList::iterator log );
	virtual void Internal_Handler(void);
public:
	virtual string InputString(string prompt);
};

class CWindowUIManager : public CUIManager
{
public:
	CWindowUIManager();
	void RefreshUI( long delta );
	static int DrawTextOnWindow(int x, int y, string caption, int* width = 0, int* height = 0);
	virtual string InputString(string prompt);
protected:
	int DrawGroupCaption(int x, int y, string caption);
	int PrintVariableLine(int x, int y, VariableCollection::iterator var, double interval);
	int PrintLogLine(int x, int y, StringList::iterator log );
	virtual void Internal_Handler(void);
public:
	bool IsInputMode;
	string InputBuffer;
private:
	string InputModeCaption;

};
