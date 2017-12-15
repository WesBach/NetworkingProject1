#ifndef _SQLWrapper_HG_
#define _SQL_Wrapper_HG_

#include <cppconn\driver.h>
#include <cppconn\exception.h>
#include <cppconn\resultset.h>
#include <cppconn\statement.h>
#include <cppconn\prepared_statement.h>

class SQL_Wrapper {
public:
	SQL_Wrapper(std::string db);
	~SQL_Wrapper();

	std::string dbName;


	void connectToDB();
	//add a client to the database
	bool addAccount(std::string email,std::string password);
	//authenticate the account
	bool authenticateAccount(std::string email, std::string password);


private:
	//private driver and connection
	//only the wrapper needs to know these
	sql::Statement* statement;
	sql::PreparedStatement* prepState;
	sql::Driver* driver;
	sql::Connection* connection;

};

#endif // 

