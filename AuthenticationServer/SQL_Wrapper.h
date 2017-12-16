#ifndef _SQLWrapper_HG_
#define _SQL_Wrapper_HG_
#include <string>

#include <cppconn\driver.h>
#include <cppconn\exception.h>
#include <cppconn\resultset.h>
#include <cppconn\statement.h>
#include <cppconn\prepared_statement.h>

class SQL_Wrapper {
public:

	void connectToDB();
	//add a client to the database.
	//returns -1 for server error 
	//returns 1 for exists
	//returns 0 for added 
	std::pair<int,int> addAccount(std::string email,std::string password);

	//authenticate the account
	// -1 for server error 
	// 1 for invalid credentials
	// 0 for success 
	//first int is above, second int is the userid, string is the date created
	std::pair<std::pair<int,int>,std::string> authenticateAccount(std::string email, std::string password);

	bool execute(const std::string& statement);
	sql::ResultSet* executeSelect(const std::string& statement);
	int executeUpdate(const std::string& statement);
	static SQL_Wrapper* getInstance();

private:
	SQL_Wrapper() {};
	static SQL_Wrapper* theWrapper;
	//private driver and connection
	//only the wrapper needs to know these
	sql::Statement* statement;
	sql::PreparedStatement* prepState;
	sql::Driver* driver;
	sql::Connection* connection;
};

#endif // 

