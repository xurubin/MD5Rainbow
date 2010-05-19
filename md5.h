
typedef struct MD5state_st
{
	unsigned int A,B,C,D;
	// simplified from openssl
} MD5_CTX;

// num is the number of 512-bit blocks in p
extern "C" void md5_block_data_order (MD5_CTX *context, const void *p,int num);
