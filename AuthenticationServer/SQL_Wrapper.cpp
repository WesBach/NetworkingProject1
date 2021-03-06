#include "SQL_Wrapper.h"
#include "Utility.h"
//SQL_Wrapper* SQL_Wrapper::theWrapper = nullptr;

//Name:			connectToDB
//Purpose:		Tries to connect to the database.
//Return:		void
void SQL_Wrapper::connectToDB()
{
	//try to get a connection
	try {
		driver = get_driver_instance();
		connection = driver->connect("127.0.0.1:3306", "root", "root");
		connection->setSchema("authentication");
	}
	catch (sql::SQLException &exception)
	{
		std::cout << "# ERR: SQLException in " << __FILE__ << std::endl;
		std::cout << "(" << ")" << std::endl;
	}
}

//Name:			addAccount
//Purpose:		Given an email and password try to add the accoun to the MySQL database.
//Return:		std::pair<int, int> (success//failure,userId)
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

		this->execute("INSERT INTO user (last_login,creation_date) values(NOW(),NOW());");
		//get the users id
		std::string selectLastId = "SELECT LAST_INSERT_ID();";
		sql::ResultSet* result  = this->executeSelect(selectLastId);
		int userID = 0;

		//convert the id to a string for an insert into the next table
		if(result->next())
			userID = result->getInt(1);	
		//hash the password
		
		std::string userId = std::to_string(userID);

		std::string hashedPassword = createHash((char*)tempPass.c_str());
		//std::string tempString = hashedPassword;

		std::string insert = "INSERT INTO web_auth (email,salt,userId,hashed_password) values('" + email + "','" + salt + "','" + userId + "','" + hashedPassword + "');";

		//add the users web_auth info to the database
		this->execute(insert);
		
		returnInfo.first = 0;
		returnInfo.second = userID;
		return returnInfo;
	}

	returnInfo.first = -1;
	return returnInfo;
}

//Name:			authenticateAccount
//Purpose:		Given an email and password try to authenticate the account in the MySQL database.
//Return:		std::pair<std::pair<int, int>, std::string> (success//failure,userId),date added
std::pair<std::pair<int, int>, std::string> SQL_Wrapper::authenticateAccount(std::string email, std::string password)
{ 
	// -1 for server error 
	// 1 for invalid credentials
	// 0 for success 
	std::pair<std::pair<int, int>, std::string> returnInfo;
	returnInfo.second = "";
	//user id
	returnInfo.first.second = -1;

	//find the user by it's email
	std::string selectUserByEmail = "SELECT * FROM web_auth WHERE email ='" + email+"';";

	sql::ResultSet* userResult = this->executeSelect(selectUserByEmail);

	//TODO::
	//make sure only one user was retrieved
	if (userResult->rowsCount() == 1)
	{
		//set it to the first item
		userResult->next();
		//get the user id
		int userId = userResult->getInt("userId");
		//get the user salt
		std::string salt = userResult->getString("salt").c_str();
		//get the user hash
		std::string hash = userResult->getString("hashed_password").c_str();

		std::string tempPass = password + salt;
		//hash the password
		std::string hashedPassword = createHash((char*)tempPass.c_str());
		//convert the char* to a string
		//std::string hashString(hashedPassword);

		//required uint64 userId = 2;
		//required string creationDate = 3;

		if (hash == hashedPassword)
		{
			//they match and were good to go
			//success
			returnInfo.first.first =  0;
			std::string getUserById = "SELECT * FROM user WHERE id ='";
			getUserById += std::to_string(userId); ;
			getUserById += "';";

			//TODO: ADD TO AUTH
			sql::ResultSet* theUser;
			theUser = executeSelect(getUserById);

			if (theUser->rowsCount() == 1)
			{
				//set the first item
				theUser->next();
				//populate the pair
				returnInfo.first.second = theUser->getInt(1);
				returnInfo.second = theUser->getString(3).c_str();

				//update the last login 
				std::string update = "UPDATE user SET last_login = NOW() WHERE id = ";
				update += std::to_string(userId);
				update += " ;";
				executeUpdate(update);

				return returnInfo;
			}
		}
		else
		{
			//invalid credentials
			returnInfo.first.first = 1;
			return returnInfo;
		}
	}
	//shouldnt reach this point
	returnInfo.first.first = 1;
	return returnInfo;
}

//Name:			execute
//Purpose:		Execute a passed in SQL statement.
//Return:		bool
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

//Name:			executeUpdate
//Purpose:		Execute a passed in SQL update statement.
//Return:		int
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

//took this out while troubleshooting

//SQL_Wrapper * SQL_Wrapper::getInstance()
//{
//	if (SQL_Wrapper::theWrapper == nullptr)
//	{
//		SQL_Wrapper::theWrapper = new SQL_Wrapper();
//	}
//
//	return SQL_Wrapper::theWrapper;
//}

//Name:			executeSelect
//Purpose:		Execute a passed in SQL select statement.
//Return:		sql::ResultSet*
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