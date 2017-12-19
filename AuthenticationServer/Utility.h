#ifndef _Utility_HG_
#define _Utility_HG_
#include <string>
//import the sha_256 stuff here and create functions for hashing.

//TODO::
//get libs for salting or create own salt using system time

std::string createHash(char* pass);
std::string createSalt();
char genRandomChar();

#endif // !_Utility_HG_

