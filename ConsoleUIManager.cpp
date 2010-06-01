#define _CRT_SECURE_NO_WARNINGS
#include "UIManager.h"
#ifdef WIN32
#include "windows.h"
#endif
#include "common.h"
#include <string.h> 
#include <memory.h> 


void CConsoleUIManager::PrintVariableLine( VariableCollection::iterator var, double interval )
{
	char line[LINE_WIDTH+1];
	sprintf(line, "|*%s: ", var->first.c_str());
	var->second->Draw(interval, LINE_WIDTH-strlen(line), line+strlen(line));
	for(int j=strlen(line); j<LINE_WIDTH-1;j++) line[j] = ' ';
	line[LINE_WIDTH-1] = '|';
	line[LINE_WIDTH] = '\0';
	printf("%s\n", line);
}

void CConsoleUIManager::PrintLogLine( StringList::iterator logiter )
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

void CConsoleUIManager::Internal_Handler(void)
{
	int lines = 0;
	char line[LINE_WIDTH+1];
	double interval = 1;
	int max_loglines = 10;
#ifdef WIN32
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD ConsoleMode;
	GetConsoleMode(hOut, &ConsoleMode);
	SetConsoleMode(hOut, ConsoleMode & (~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT)));
#endif

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
		//TODO: Clear old lines
		pthread_mutex_unlock(&mutex);


#ifdef WIN32
		Sleep((int)(refresh_interval*1000));
		hOut = GetStdHandle(STD_OUTPUT_HANDLE);
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
