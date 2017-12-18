#include "SQL_Wrapper.h"
#include "Utility.h"
//SQL_Wrapper* SQL_Wrapper::theWrapper = nullptr;

void SQL_Wrapper::connectToDB()
{
	//try to get a connection
	try {
		driver = get_driver_instance();
		connection = driver->connect("127.0.0.1:3306", "root", "SQL123");
		connection->setSchema("authentication");
	}
	catch (sql::SQLException &exception)
	{
		std::cout << "# ERR: SQLException in " << __FILE__ << std::endl;
		std::cout << "(" << ")" << std::endl;
	}
}

std::pair<int, int> SQL_Wrapper::addAccount(std::string email, std::string password)
{
	//returns -1 for server error 
	//returns 1 for exists
	//returns 0 for added
	std::pair<int, int> returnInfo(-1,-1);
	//std::string fetchUserByEmail = "SELECT * FROM user WHERE email = '" + email + "';";
	sql::ResultSet* result = this->executeSelect("SELECT * FROM web_auth WHERE email = '" + email + "';");

	if (result->next() )
	{
		returnInfo.first = 1;
		return returnInfo;
	}
	else
	{

		//create the salt 
		std::string salt = createSalt();
		//add the salt to the password
		std::string tempPass = password + salt;
		//hash the password
		char* hashedPassword = createHash((char*)tempPass.c_str());
		this->execute("INSERT INTO user last_login,creation_date values(now(),now());");

		//get the users id
		std::string selectLastId = "SELECT LAST_INSERT_ID();";
		sql::ResultSet* result  = this->executeSelect(selectLastId);
		//convert the id to a string for an insert into the next table
		int userID = result->getInt(1);
		std::string userId = std::to_string(userID);

		//add the users web_auth info to the database
		this->execute("INSERT INTO web_auth email,salt,userId,hash values('" + email + "','" + salt + "','" + userId + "','" + hashedPassword + "');");
		
		returnInfo.first = 0;
		returnInfo.first = userID;
		return returnInfo;

	}


	returnInfo.first = -1;
	return returnInfo;
}

std::pair<std::pair<int, int>, std::string> SQL_Wrapper::authenticateAccount(std::string email, std::string password)
{
	// -1 for server error 
	// 1 for invalid credentials
	// 0 for success 
	std::pair<std::pair<int, int>, std::string> returnInfo;
	returnInfo.second = "";
	returnInfo.first.second = -1;

	//find the user by it's email
	std::string selectUserByEmail = "SELECT * FROM user WHERE email =" + email;
	selectUserByEmail += ";";
	sql::ResultSet* userResult = this->executeSelect(selectUserByEmail);

	//TODO::
	//make sure only one user was retrieved
	if (userResult->rowsCount() == 1)
	{
		//get the user salt
		int userId = userResult->getInt(4);
		std::string salt = userResult->getString(3);
		std::string hash = userResult->getString(5);

		std::string tempPass = password + salt;
		//hash the password
		char* hashedPassword = createHash((char*)tempPass.c_str());
		//convert the char* to a string
		std::string hashString(hashedPassword);

		//required uint64 userId = 2;
		//required string creationDate = 3;

		if (hashString.compare(tempPass))
		{
			//they match and were good to go
			returnInfo.first.first =  0;
			std::string getUserById = "SELECT * FROM user WHERE id =" + userId;
			getUserById += ";";

			sql::ResultSet* theUser;
			theUser = executeSelect(getUserById);

			if (theUser->rowsCount() == 1)
			{
				//populate the pair
				returnInfo.first.second = theUser->getInt(1);
				returnInfo.second = theUser->getString(3);

				//update the last login 
				std::string update = "UPDATE user WHERE id = " + userId;
				update += " SET last_login= now();";
				executeUpdate(update);

				return returnInfo;
			}
		}
		else
		{
			returnInfo.first.first = 1;
			return returnInfo;
		}
	}
	returnInfo.first.first = 1;
	return returnInfo;
}

bool SQL_Wrapper::execute(const std::string& statement)
{
	try
	{
		this->prepState = connection->prepareStatement(statement.c_str());
		return this->prepState->execute();
	}
	catch (sql::SQLException &exception)
	{
		std::cout << "# ERR: SQLException in " << __FILE__;
		std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
		std::cout << "# ERR: " << exception.what();
		std::cout << " (MySQL error code: " << exception.getErrorCode();
		std::cout << ", SQLState: " << exception.getSQLState() << " )" << std::endl;
		return false;
	}
	return false;
}

int SQL_Wrapper::executeUpdate(const std::string& statement)
{
	try
	{
		this->prepState = this->connection->prepareStatement(statement.c_str());
		return this->prepState->executeUpdate();
	}
	catch (sql::SQLException &exception)
	{
		std::cout << "# ERR: SQLException in " << __FILE__;
		std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
		std::cout << "# ERR: " << exception.what();
		std::cout << " (MySQL error code: " << exception.getErrorCode();
		std::cout << ", SQLState: " << exception.getSQLState() << " )" << std::endl;
		return false;
	}
	return false;
}

//SQL_Wrapper * SQL_Wrapper::getInstance()
//{
//	if (SQL_Wrapper::theWrapper == nullptr)
//	{
//		SQL_Wrapper::theWrapper = new SQL_Wrapper();
//	}
//
//	return SQL_Wrapper::theWrapper;
//}

sql::ResultSet* SQL_Wrapper::executeSelect(const std::string& statement) {
	try
	{
		this->prepState = this->connection->prepareStatement(statement.c_str());
		return this->prepState->executeQuery();
	}
	catch (sql::SQLException &exception)
	{
		std::cout << "# ERR: SQLException in " << __FILE__;
		std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
		std::cout << "# ERR: " << exception.what();
		std::cout << " (MySQL error code: " << exception.getErrorCode();
		std::cout << ", SQLState: " << exception.getSQLState() << " )" << std::endl;
		return false;
	}
}