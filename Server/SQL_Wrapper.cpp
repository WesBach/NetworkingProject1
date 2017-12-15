#include "SQL_Wrapper.h"

SQL_Wrapper::SQL_Wrapper(std::string db) {
	this->dbName = db;
}

SQL_Wrapper::~SQL_Wrapper() {
	//close the connection
	connection->close();
}

void SQL_Wrapper::connectToDB()
{
	//try to get a connection
	
		this->driver = get_driver_instance();
		this->connection = this->driver->connect("127.0.0.1", "root", "root");
		this->connection->setSchema(this->dbName);	
	
}

bool SQL_Wrapper::addAccount(std::string email, std::string password)
{
	//generate the userId.
	long userId;// = math.rand();
	this->statement = connection->createStatement();
	//this->statement->execute("INSERT INTO web_auth email,salt,userId values("+email+","+password+","++")");

	delete statement;
	return true;
}

bool SQL_Wrapper::authenticateAccount(std::string email, std::string password)
{
	return false;
}

