#define _CRT_SECURE_NO_WARNINGS
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "TableManager.h"
#include "HashReduce.h"
#include "UIManager.h"

int main(int argc, char* argv[])
{
	CTableManager table;
	CUIManager::getSingleton();
	pthread_t pid;
	pthread_create(&pid, 0, (void *(*)(void *))CUIManager::Handler, 0);

	if (!CMD5HashReduce::SelfTest())
		CUIManager::getSingleton().PrintLn(-1, "Warning: Hash algorithm self check failed!\n");

	if (argc < 3)
		goto display_usage;
	if (strcmp(argv[1], "create") == 0)
	{	
		table.CreateNewTable(argv[2]);
		table.DisplayDescription();
		goto wait_for_terminate;
	}else if (strcmp(argv[1], "generate") == 0)
	{
		if (argc != 4) goto display_usage;

		if (!table.LoadTable(argv[2]))
		{
			CUIManager::getSingleton().PrintLn(-1,"Table with name %s does not exist or config file contains error.\n");
			return 1;
		}
		table.DisplayDescription();
		table.Generate(atoi(argv[3]));
		goto wait_for_terminate;
	}else if (strcmp(argv[1], "lookup") == 0)
	{
		if (argc != 4) goto display_usage;

		if (!table.LoadTable(argv[2]))
		{
			printf("Table with name %s does not exist or config file contains error.\n", argv[2]);
			return 1;
		}
		table.DisplayDescription();
		char line[128];
		do 
		{
			CUIManager::getSingleton().PrintLn(-1, "Input target MD5 hash:");
			if (!fgets(line, sizeof(line),stdin)) break;
			line[32] = '\0';
			if (strlen(line) != 32) break;
			table.Lookup(line, atoi(argv[3]));
		} while (true);
		goto wait_for_terminate;
	}

display_usage:
	CUIManager::getSingleton().PrintLn(-1, "Usage: MD5Rainbow create   <tablename>\n");
	CUIManager::getSingleton().PrintLn(-1, "Usage: MD5Rainbow generate <tablename> NumCores\n");
	CUIManager::getSingleton().PrintLn(-1, "Usage: MD5Rainbow lookup   <tablename> NumCores\n");
wait_for_terminate:
	char c;
	CUIManager::getSingleton().PrintLn(-1, "Press any key to exit.");
	scanf("%c", &c);
}

