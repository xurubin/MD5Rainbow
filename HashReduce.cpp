#include "HashReduce.h"
#include "md5.h"
CMD5HashReduce::CMD5HashReduce(void)
{
}

CMD5HashReduce::~CMD5HashReduce(void)
{
}

void CMD5HashReduce::Hash_One_Block(char* block, char* digest )
{
	MD5_CTX* ctx = (MD5_CTX*) digest;
	ctx->A = 0x67452301;
	ctx->B = 0xefcdab89;
	ctx->C = 0x98badcfe;
	ctx->D = 0x10325476;
	md5_block_data_order(ctx, block, 1);

}

Index_Type CMD5HashReduce::Reduce(char* digest, int index )
{
	return (*(Index_Type*)digest) +(Index_Type)index;
}

bool CMD5HashReduce::SelfTest( void )
{
	char block[64];
	MD5_CTX ctx;
	for(int i=0;i<sizeof(block);i++) block[i] = 0;
	block[0] = (char)0x80; //A block for empty string -> d41d8cd98f00b204e9800998ecf8427e
	Hash_One_Block(block, (char*)&ctx);
	return ((ctx.A == 0xd98c1dd4) &&
			(ctx.B == 0x04b2008f) &&
			(ctx.C == 0x980980e9) &&
			(ctx.D == 0x7e42f8ec));
/*
	return ( (ctx.A == 0xd41d8cd9) &&
			 (ctx.B == 0x8f00b204) &&
			 (ctx.C == 0xe9800998) &&
			 (ctx.D == 0xecf8427e) )
*/
}
