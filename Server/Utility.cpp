#include "Utility.h"

#include <openssl\conf.h>
#include <openssl\evp.h>
#include <openssl\err.h>
#include <openssl\sha.h>

SHA256_CTX ctx;

char * createSalt()
{
	//fix this to do something 
	char * theSalt;
	return theSalt;
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