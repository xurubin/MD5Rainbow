#define _CRT_SECURE_NO_WARNINGS
#include "UIManager.h"

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
		//instance = new CWindowUIManager();
		instance = new CConsoleUIManager();
	return *instance;
}

CUIManager* CUIManager::instance = 0;

void CUIManager::Handler(void)
{
	CUIManager::getSingleton().Internal_Handler();
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
	if(group != groups.end())
		group->second.variables[name] = v;
	else
		v = NULL;
	pthread_mutex_unlock(&mutex);
	return v;
}

void CUIManager::UnregisterVariable( int groupid, string name )
{
	

	pthread_mutex_lock(&mutex);
	GroupCollection::iterator group = groups.find(groupid);
	if(group != groups.end())
	{
		VariableCollection::iterator variable = group->second.variables.find(name);
		if (variable != group->second.variables.end())
		{
			Variable* pt = variable->second;
			group->second.variables.erase(variable);
			delete pt;
		}
	}
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
	int len = 0;
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
