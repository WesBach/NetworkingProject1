#ifndef _Utility_HG_
#define _Utility_HG_
#include <string>
//import the sha_256 stuff here and create functions for hashing.

//TODO::
//get libs for salting or create own salt using system time

char * createHash(char* pass);
char * createSalt();

#endif // !_Utility_HG_

