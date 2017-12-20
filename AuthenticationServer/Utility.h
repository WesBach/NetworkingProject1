#ifndef _Utility_HG_
#define _Utility_HG_
#include <string>

//Name:			createSalt
//Purpose:		Uses gen random char to generate 20 random characters/numbers for a salt value.
//Return:		std::String
std::string createSalt();

//Name:			createHash
//Purpose:		Hashes the characters passed into the function.
//Return:		std::String
std::string createHash(char* pass);

//Name:			genRandomChar
//Purpose:		Generates a random character from the array of characters.
//Return:		std::String
char genRandomChar();

#endif // !_Utility_HG_

