#pragma once
#include "common.h"

class CMD5HashReduce
{
public:
	static void Hash_One_Block(char* block, char* digest );
	static bool SelfTest(void);
	static Index_Type Reduce(char* digest, int index);
	CMD5HashReduce(void);
	~CMD5HashReduce(void);
};
