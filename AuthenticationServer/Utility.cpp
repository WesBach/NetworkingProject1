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

//Name:			genRandomChar
//Purpose:		Generates a random character from the array of characters.
//Return:		char
char genRandomChar()
{
	int stringLength = sizeof(alphanum) - 1;
	return alphanum[rand() % stringLength];
}

//Name:			createSalt
//Purpose:		Uses gen random char to generate 20 random characters/numbers for a salt value.
//Return:		std::Strin
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

//Name:			createHash
//Purpose:		Hashes the characters passed into the function.
//Return:		std::String
std::string createHash(char* pass) {
	//use sha_256 to generate a hash of the passed in string
	unsigned char digest[SHA256_DIGEST_LENGTH];

	//SHA256((unsigned char*)&pass, strlen(pass), (unsigned char*)&digest);

	SHA256_Init(&ctx);
	SHA256_Update(&ctx, pass, strlen(pass));
	SHA256_Final(digest, &ctx);

	char mdString[SHA256_DIGEST_LENGTH * 2 + 1];
	for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
		sprintf(&mdString[i * 2], "%02x", (unsigned int)digest[i]);
	
	//return the hash
	return mdString;
}