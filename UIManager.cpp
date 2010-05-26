#define _CRT_SECURE_NO_WARNINGS
#include "UIManager.h"
#ifdef WIN32
#include "windows.h"
#endif
#include "common.h"
#include <string.h> 
#include <memory.h> 
CUIManager::CUIManager(void):max_handle(0)
{
	pthread_mutex_init(&mutex, NULL);
	group_height = 10;
	console_out = stdout;
	refresh_interval = 1.000;
	groups[-1].name = "Global logs:";
}

CUIManager::~CUIManager(void)
{
	pthread_mutex_destroy(&mutex);
	for(GroupCollection::iterator g = groups.begin(); g!= groups.end(); g++)
		for(VariableCollection::iterator v = g->second.variables.begin(); v != g->second.variables.end(); v++)
			delete v->second;
}

CUIManager& CUIManager::getSingleton(void)
{
	if (instance == 0)
		instance = new CUIManager();
	return *instance;
}

CUIManager* CUIManager::instance = 0;

void CUIManager::Handler(void)
{
	CUIManager::getSingleton().Internal_Handler();
}

void CUIManager::PrintVariableLine( VariableCollection::iterator var, double interval )
{
	char line[LINE_WIDTH+1];
	sprintf(line, "|*%s: ", var->first.c_str());
	var->second->Draw(interval, LINE_WIDTH-strlen(line), line+strlen(line));
	for(int j=strlen(line); j<LINE_WIDTH-1;j++) line[j] = ' ';
	line[LINE_WIDTH-1] = '|';
	line[LINE_WIDTH] = '\0';
	printf("%s\n", line);
}

void CUIManager::PrintLogLine( StringList::iterator logiter )
{
	char line[LINE_WIDTH+1];
	string log = *logiter;
	while( (log[log.size()-1] == '\r') || (log[log.size()-1] == '\n')) log = log.substr(0, log.size()-1);
	if (log.size() > LINE_WIDTH - 4)
		sprintf(line, "| %s...", log.substr(0, LINE_WIDTH-10).c_str());
	else
		sprintf(line, "| %s", log.c_str());
	
	for(int j=strlen(line); j<LINE_WIDTH-1;j++) line[j] = ' ';
	line[LINE_WIDTH-1] = '|';
	line[LINE_WIDTH] = '\0';
	printf("%s\n",line);
}

void CUIManager::Internal_Handler(void)
{
	int lines = 0;
	char line[LINE_WIDTH+1];
	double interval = 1;
	int max_loglines = 10;
	while(true)
	{
		pthread_mutex_lock(&mutex);
		lines = 0;
		for(GroupCollection::iterator gi = groups.begin(); gi != groups.end();gi++)
		{
			VariableCollection& variables = gi->second.variables;
			StringList& logs = gi->second.logs;
			int used_lines = 0;
			//Print Header
			memset(line, '=', sizeof(line));line[sizeof(line)-1] = '\0';
			for(unsigned int j=0;j<gi->second.name.length();j++) line[j] = gi->second.name[j];
			printf("%s \n", line);
			used_lines++;
			//Print variables
			for(VariableCollection::iterator var = variables.begin(); var != variables.end(); var++)
			{
				PrintVariableLine(var, interval);
				used_lines++;
			}
			//Print logs
			max_loglines = group_height - 1 - variables.size();
			int logid = max_loglines - logs.size();
			for(StringList::iterator log = logs.begin(); log != logs.end(); log++)
			{
				logid++;
				if (logid <= 0) continue;
				PrintLogLine(log);
				used_lines++;
			}
			//Fill in extra lines
			memset(line, ' ', sizeof(line));
			line[0] = '|';
			line[sizeof(line)-2] = '|';
			line[sizeof(line)-1] = '\0';
			for(int i=0;i<group_height-used_lines;i++)printf("%s\n", line);
			
			lines +=group_height;
		}
		pthread_mutex_unlock(&mutex);
	

#ifdef WIN32
		Sleep((int)(refresh_interval*1000));
		HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
		CONSOLE_SCREEN_BUFFER_INFO curinfo;
		GetConsoleScreenBufferInfo(hOut, &curinfo);
		COORD newpos = curinfo.dwCursorPosition;
		newpos.Y -= lines;
		SetConsoleCursorPosition(hOut, newpos);
		printf("\r");
		fflush(stdout);
#else
		sleep((int)refresh_interval);
		printf("%c%c%dA\r", 0x1B, 0x5B, (int)lines);
		fflush(stdout);
#endif
	}
}

int CUIManager::allocate_handle( void )
{
	return max_handle++;
}

IntVariable* CUIManager::RegisterIntVariable( string name, int* var, int group )
{
	IntVariable* v = new IntVariable(var);
	return (IntVariable*)RegisterVariable(name, group, v);
}

DoubleVariable* CUIManager::RegisterDoubleVariable( string name, double* var, int group )
{
	DoubleVariable* v = new DoubleVariable(var);
	return (DoubleVariable*)RegisterVariable(name, group, v);
}

Variable* CUIManager::RegisterVariable( string name, int groupid, Variable* v )
{
	pthread_mutex_lock(&mutex);
	GroupCollection::iterator group = groups.find(groupid);
	if(group == groups.end()) return NULL;

	group->second.variables[name] = v;
	pthread_mutex_unlock(&mutex);
	return v;
}

void CUIManager::UnregisterVariable( int groupid, string name )
{
	pthread_mutex_lock(&mutex);
	GroupCollection::iterator group = groups.find(groupid);
	if(group == groups.end()) return;
	VariableCollection::iterator variable = group->second.variables.find(name);
	if (variable == group->second.variables.end()) return;
	Variable* pt = variable->second;
	group->second.variables.erase(variable);
	delete pt;
	pthread_mutex_unlock(&mutex);
	
}

int CUIManager::CreateGroup( string name )
{
	for(GroupCollection::iterator g = groups.begin(); g!= groups.end(); g++)
		if(g->second.name.compare(name) == 0)
			return g->first;

	int h = allocate_handle();
	groups[h].name = name;
	return h;
}

int CUIManager::RemoveGroup( int id )
{
	if (id == -1) return 0;
	GroupCollection::iterator group = groups.find(id);
	if(group == groups.end()) return 0;

	ClearGroup(id);

	groups.erase(group);
	return 1;
}

void CUIManager::ClearGroup( int id )
{
	if (id == -1) return;
	GroupCollection::iterator group = groups.find(id);
	if(group == groups.end()) return;

	for(VariableCollection::iterator v = group->second.variables.begin(); v != group->second.variables.end(); v++)
		delete v->second;
	group->second.variables.clear();
}

void CUIManager::PrintLn( int group, string line, bool raw )
{
	pthread_mutex_lock(&mutex);

	StringList* log = NULL;
	if (groups.find(group) != groups.end())
		log = & groups[group].logs;
	else
		log = & groups[-1].logs;
	if (raw)
		log->push_back(string(line));
	else
	{
		char logline[1024];
		time_t t1;
		time(&t1);
		tm* t = localtime(&t1);
		sprintf(logline, "%02d:%02d.%02d: %s", t->tm_hour, t->tm_min, t->tm_sec, line.c_str());
		log->push_back(string(logline));
	}
	if (log->size() > 30)
		log->pop_front();
	pthread_mutex_unlock(&mutex);
}

void CUIManager::ClearLogLines( int group )
{
	pthread_mutex_lock(&mutex);

	StringList* log = NULL;
	if (groups.find(group) != groups.end())
		log = & groups[group].logs;
	else
		log = NULL;

	if (log) log->clear();
	pthread_mutex_unlock(&mutex);
}

/************************************************************************/
/*    Variable -related implementation                                  */
/************************************************************************/
Variable::Variable(void* variable):var(variable){}

int Variable::Draw( double interval, int maxlen, char * Out )
{
	//throw new exception("Invoking abstract method in base class");
	return 0;
}

DoubleVariable::DoubleVariable( void* variable ) : Variable(variable)
{
	//throw new exception("Not implemented");
}

int DoubleVariable::Draw( double interval, int maxlen, char * Out )
{
	//throw new exception("Not implemented");
	return 0;
}

IntVariable::IntVariable( void* variable ):Variable(variable)
{
	style = Raw;
	oldvalue = 0;
}

int IntVariable::Draw( double interval, int maxlen, char * Out )
{
	int len;
	int * value = (int*)var;
	switch (style)
	{
	case Progress:
		if((*value>=min)&&(*value<=max))
		{
			double percentage = ((double)*value - min) / (max-min);
			len = sprintf(Out, "%4.2f%c", percentage*100.0, '%');
			int bar_maxlen = maxlen - len - 1;
			int j;
			for(j=0;j<(int)(bar_maxlen*percentage);j++)
				Out[len+j] = '=';
			Out[len+j] = (bar_maxlen*percentage-j <0.5) ? '-' : '>';
			Out[len+j+1] = '\0';
			len = len+j;
			break;
		}
	case Raw:
		len = sprintf(Out, "%d", *value);
		break;
	case Gradient:
		int newvalue = *value;
		len = sprintf(Out, "%d", (int)((newvalue - oldvalue)/interval));
		oldvalue = newvalue;
		break;
	}
	return len;
}
