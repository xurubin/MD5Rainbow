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
#ifdef WIN32
		instance = new CWindowUIManager();
#else
		instance = new CConsoleUIManager();
#endif
	return *instance;
}

CUIManager* CUIManager::instance = 0;
 CacheVisualiser CUIManager::cache;
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

DoubleVariable::DoubleVariable( void* variable ) : Variable(variable)
{
	//throw new exception("Not implemented");
}

int DoubleVariable::DrawConsole( double interval, int maxlen, char * Out )
{
	//throw new exception("Not implemented");
	return 0;
}

#ifdef WIN32
int DoubleVariable::DrawWindow(double interval, int x, int y, int width, HDC dc)
{
	return 0;
}
#endif

IntVariable::IntVariable( void* variable ):Variable(variable)
{
	style = Raw;
	oldvalue = 0;
}

void CacheVisualiser::Hit(int position) {
	int o = ((long long)(position-range_min))*(long long)segments/(long long)(range_max-range_min);
	if((o >= 0)&&(o < segments)) 
	{
		data[o] += 255; 
		if (data[o] > 255) data[o] = 255;
		numaccesses++;
		if (cacheset.find(o) != cacheset.end()) numhits++;
		if (cachequeue.size() > 1000)
		{
			int victim = cachequeue.front();
			cachequeue.pop_front();
			cacheset.erase(victim);
		}else
		{
			cachequeue.push_back(o);
			cacheset.insert(o);
		}
	}
}

void CacheVisualiser::Damp(long now) { 
	if ((now-timetag) < 500) return;
	timetag = now;
	for(int i=0;i<segments;i++) data[i] /= 2;
}
