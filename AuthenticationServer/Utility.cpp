#include "Utility.h"
#include <openssl\conf.h>
#include <openssl\evp.h>
#include <openssl\err.h>
#include <openssl\sha.h>
#include <cstdlib>
#include <ctime>
using namespace std;
SHA256_CTX ctx;

static const char alphanum[] =
"0123456789"
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz";

char genRandomChar()
{
	int stringLength = sizeof(alphanum) - 1;
	return alphanum[rand() % stringLength];
}

std::string createSalt()
{
	srand(time(0));
	std::string str;
	//generate salt thats 20 symbols
	for (unsigned int i = 0; i < 20; ++i)
	{
		str += genRandomChar();
	}
	//return the salt
	return str;
}

char * createHash(char* pass) {
	char * hash = "";
	//use sha_256 to generate a hash of the passed in string
	unsigned char digest[SHA256_DIGEST_LENGTH];
	SHA256_Update(&ctx, pass, strlen(pass));
	SHA256_Final(digest, &ctx);
	//return the hash
	return hash;
}