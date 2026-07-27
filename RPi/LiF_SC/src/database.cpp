// Author: NK
// Brief: various database functions to read/write/update DB. Below is a map of their uses:

// Assuming in main.cpp you do something like:
// DB db;
// then,
// use: bool connected = db.db_connect();
// use: DB::OperationMode mode = db.get_operation_mode();
// use: int floor = db.read_floor_request();
// use: bool success = db.set_operation_mode(DB::OperationMode::SABBATH);
// use: bool success = db.set_doors_open(false); // closing the doors
// use: int logResult = db.log_can_message(id, data, length);

#include "../include/database.h"

// reformatted this block since I'm more used to this type of switch-case, mb
std::string DB::map_table_name(DB::Tables table)
{
    switch (table)
    {
    case Tables::CAN_LOGS:
        return std::string("can_table_name");

    case Tables::ELEVATOR_LOGS:
        return std::string("elevator_requests");

    default:
        std::cerr << "Invalid DB table value" << std::endl;
    }
    return std::string();
}

std::string DB::map_operation_mode(DB::OperationMode operationMode)
{
    switch (operationMode)
    {
    case OperationMode::NORMAL:
        return std::string("normal");

    case OperationMode::SABBATH:
        return std::string("sabbath");

    case OperationMode::MAINTENANCE:
        return std::string("maintenance");

    case OperationMode::FAULT:
        return std::string("fault");

    default:
        std::cerr << "Invalid DB operation mode" << std::endl;
    }
    return std::string();
}

// attempts to establish DB connection (try-catch)
// returns true if exists or active, false if failed
bool DB::db_connect()
{
    // if a connection already exists to the DB do not connect to it again
    if (con != nullptr && !con->isClosed())
    {
        return true;
    }

    // in case con is old, delete and replace (cleanup before trying)
    if (con != nullptr)
    {
        delete con;
        con = nullptr;
    }

    // try to open the SQL connection
    try
    {
        // obtain the SQL driver
        // similar to original code
        sql::Driver *driver = get_driver_instance();

        // connect to MariaDB
        con = driver->connect("tcp://127.0.0.1:3306", "LiF_Admin", "LiF_ESE");

        // select the lif_elevator database to use
        con->setSchema("lif_elevator");

        // if everything works up until this point, no errors occured so return true
        return true;
    }
    catch (sql::SQLException &error)
    {
        // display the issue the SQL returned
        // error.what() apparently formats it to be human-readable and asks the exception for its description
        std::cerr << "Database connection failed: " << error.what() << std::endl;

        // remove any partially created or unusable connection if it failed (cleanup)
        if (con != nullptr)
        {
            delete con;
            con = nullptr;
        }

        // return false since the connection attempt failed
        return false;
    }
}

// read the next pending request in elevator_requests
// returns int for request floor (1-3); 0 for no pending; -1 for error
// use: int floorRequest = db.read_floor_request();
int DB::read_floor_request()
{
    // make sure the DB connectionj exists before querying
    if (!db_connect())
    {
        return -1;
    }

    // hold SQL query statement and returned rows
    sql::Statement *statement = nullptr;
    sql::ResultSet *result = nullptr;

    // query the DB:
    try
    {
        // create an object that can send SQL commands using con
        statement = con->createStatement();

        // find oldest "pending" request
        // formatted in the same way the PHP queries are (see like any PHP for reference lol)
        result = statement->executeQuery(
            "SELECT elevator_request_id, requested_floor "
            "FROM elevator_requests "
            "WHERE request_status = 'pending' "
            "AND request_type = 'remote' "
            "ORDER BY elevator_request_id ASC "
            "LIMIT 1 ");

        // check if the query returned 0 rows
        // basically an empty table/queue case
        if (!result->next())
        {
            // no result returned to reset the request ID
            active_request_id = 0;

            // remove query objects since nothing was returned
            delete result;
            delete statement;

            // return 0 for empty table/nothing to return
            return 0;
        }

        // remember which row was selected
        active_request_id = result->getInt("elevator_request_id");

        // grab the requested floor
        int requested_floor = result->getInt("requested_floor");

        // delete the SQL objects since the values were grabbed and stored into variables
        delete result;
        delete statement;

        // return requested_floor
        return requested_floor;
    }
    catch (sql::SQLException &error)
    {
        // delete either object  if it was created before the error
        if (result != nullptr)
        {
            delete result;
        }
        if (statement != nullptr)
        {
            delete statement;
        }

        // reset request_id since it was not obtained if it failed
        active_request_id = 0;

        // print error description
        std::cerr << "Failed to read floor request from DB " << error.what() << std::endl;

        return -1;
    }
}

// mark the current active request as completed (once the EC confirms its position is the new floor)
// this only updates DB
bool DB::complete_elevator_request()
{
    // ensure there is an active request
    if (active_request_id <= 0)
    {
        std::cerr << "No active elevator request to complete" << std::endl;
        return false;
    }

    if (!db_connect())
    {
        return false;
    }

    sql::PreparedStatement *statement = nullptr;

    try
    {
        statement = con->prepareStatement(
            "UPDATE elevator_requests "
            "SET request_status = 'completed', "
            "completed_at = CURRENT_TIMESTAMP "
            "WHERE elevator_request_id = ? "
            "AND request_status IN ('pending', 'accepted')");

        // mark the request previously found by read_floor_request() as complete (for eleavator actualy moving)
        statement->setInt(1, active_request_id);

        // executeUpdate returns number of updated rows
        int updatedRows = statement->executeUpdate();

        // pointer no longer needed
        delete statement;

        if (updatedRows == 0)
        {
            std::cerr << "Active elevator request could not be completed" << std::endl;

            return false;
        }

        active_request_id = 0;

        return true;
    }
    catch (sql::SQLException &error)
    {
        if (statement != nullptr)
        {
            delete statement;
        }

        std::cerr << "failed to mark a request as complete in DB: " << error.what() << std::endl;

        return false;
    }
}

// log a successful CAN message in the CAN_message_log table in DB
// returns 1 for successful log, 0 for invalid data/no inserted row, and -1 for DB failure
int DB::log_can_message(int id, int *data, uint8_t length)
{
    // the table requires at least one data byte so ensure the passed in values fit
    if (data == nullptr || length == 0 || length > 8)
    {
        std::cerr << "CAN message was not logged: invalid data or length" << std::endl;
        return 0;
    }

    // the rawbyte was stored as a tinyint inside the table so restrict its value
    if (data[0] < 0 || data[0] > 255)
    {
        std::cerr << "CAN message was not logged: data[0] is outside of its 0-255 range" << std::endl;
        return 0;
    }

    // make a small lookup table to convert received CAN data into proper text for logging in the DB
    std::string canIDText;
    std::string direction;
    std::string sourceController;

    // RY also made can IDs store as string in the DB - fine. Will have to use table to convert int to string
    if (id == 0x100)
    {
        canIDText = "0x100";
        direction = "tx";
        sourceController = "SC";
    }
    else if (id == 0x101)
    {
        canIDText = "0x101";
        direction = "rx";
        sourceController = "EC";
    }
    else if (id == 0x200)
    {
        canIDText = "0x200";
        direction = "rx";
        sourceController = "CC";
    }
    else if (id == 0x301)
    {
        canIDText = "0x201";
        direction = "rx";
        sourceController = "F1";
    }
    else if (id == 0x302)
    {
        canIDText = "0x202";
        direction = "rx";
        sourceController = "F2";
    }
    else if (id == 0x303)
    {
        canIDText = "0x203";
        direction = "rx";
        sourceController = "F3";
    }
    else
    {
        std::cerr << "CAN message was not logged: unknown CAN id provided" << std::endl;
        return 0;
    }

    // ensure the shared database connection exists
    if (!db_connect())
    {
        return -1;
    }

    // statement pointer for query
    sql::PreparedStatement *statement = nullptr;

    // query DB
    try
    {
        statement = con->prepareStatement(
            "INSERT INTO can_message_log ( "
            "elevator_request_id, "
            "can_id, "
            "direction, "
            "raw_byte, "
            "dlc, "
            "source_controller "
            ") "

            "VALUES ( "
            "NULLIF(?, 0), "
            "?, "
            "?, "
            "?, "
            "?, "
            "? "
            ") ");

        // active_request_id is 0 when the CAN message is not tied to a remote request
        // tables won't be fully in sync because of this but this is fine because one request can have many CAN messages
        statement->setInt(1, active_request_id);
        statement->setString(2, canIDText);
        statement->setString(3, direction);
        statement->setInt(4, data[0]);
        statement->setInt(5, length);
        statement->setString(6, sourceController);

        // execute
        // executeUpdate function returns # of inserted rows so store it into an int
        int insertedRows = statement->executeUpdate();

        // clean up pointer
        delete statement;

        // ensure the query worked
        if (insertedRows == 1)
        {
            return 1;
        }

        // if function does not return 1, query failed so print error
        std::cerr << "CAN message was not logged: no row was inserted";
        return 0;
    }
    catch (sql::SQLException &error)
    {
        // clean up pointer if it was made but query failed
        if (statement != nullptr)
        {
            delete statement;
        }

        std::cerr << "Failed to log CAN message in DB: " << error.what() << std::endl;
        return -1;
    }
}

// logs a manual (physical) request that was received over CAN into the main elevator DB
// returns 1 for success, 0 for invalid data/insertion failed, and -1 for DB fail
int DB::log_elevator_request(int requestedFloor, int sourceID)
{

    // check if the requested floor is valid
    if (requestedFloor < 1 || requestedFloor > 3)
    {
        std::cerr << "invalid requested floor" << std::endl;
        return 0;
    }

    // hard-coded since all received CAN messages came from manual requests (button presses)
    std::string requestType = "manual";
    std::string sourceController;

    // determine the source controller based on CAN ID
    if (sourceID == 0x200)
    {
        sourceController = "CC";
    }
    else if (sourceID == 0x201)
    {
        sourceController = "F1";
    }
    else if (sourceID == 0x202)
    {
        sourceController = "F2";
    }
    else if (sourceID == 0x203)
    {
        sourceController = "F3";
    }
    else
    {
        std::cerr << "Invalid request source CAN ID" << std::endl;
        return 0;
    }

    // check DB connection and connect if not
    if (!db_connect())
    {
        return -1;
    }

    sql::PreparedStatement *statement = nullptr;

    try
    {
        statement = con->prepareStatement(
            "INSERT INTO elevator_requests ( "
            "request_type, "
            "requested_floor, "
            "requested_by_user_id, "
            "source_controller "
            ") "
            "VALUES ( "
            "?, "
            "?, "
            "NULL, "
            "? "
            ") ");

        // load the values into the '?' field above
        statement->setString(1, requestType);
        statement->setInt(2, requestedFloor);
        statement->setString(3, sourceController);

        // executeUpdate returns int
        int insertedRows = statement->executeUpdate();

        // clean up pointer
        delete statement;

        // should be 1 if success
        if (insertedRows == 1)
        {
            return 1;
        }

        // if failed
        std::cerr << "elevator request was not logged: no row inserted";

        return 0;
    }
    catch (sql::SQLException &error)
    {
        // if made but didn't execute properly
        if (statement != nullptr)
        {
            delete statement;
        }

        std::cerr << "failed to log elevator request in the DB: " << error.what() << std::endl;
        return -1;
    }
}

// function that grabs the current operation mode from the database
DB::OperationMode DB::get_operation_mode()
{
    // make sure the DB connection exists before querying
    if (!db_connect())
    {
        return OperationMode::UNKNOWN;
    }

    // hold the SQL query statement and relevant pointers
    sql::Statement *statement = nullptr;
    sql::ResultSet *result = nullptr;

    // query the DB:
    try
    {
        statement = con->createStatement();

        result = statement->executeQuery(
            "SELECT operation_mode "
            "FROM elevator_state "
            "WHERE state_id = 1 "
            "LIMIT 1 ");

        // check if the query returned 0 rows
        // basically an empty table/queue case
        if (!result->next())
        {

            // remove query objects since nothing was returned
            delete result;
            delete statement;

            // return UNKNOWN for unknown value/fail
            return OperationMode::UNKNOWN;
        }

        // grab the operation_mode
        std::string operationMode = result->getString("operation_mode");

        // obtained the mode from the line above, cleanup objects
        delete result;
        delete statement;

        // map the returned operation_mode into the enum class
        if (operationMode == "normal")
        {
            return OperationMode::NORMAL;
        }
        else if (operationMode == "sabbath")
        {
            return OperationMode::SABBATH;
        }
        else if (operationMode == "maintenance")
        {
            return OperationMode::MAINTENANCE;
        }
        else if (operationMode == "fault")
        {
            return OperationMode::FAULT;
        }

        return OperationMode::UNKNOWN;
    }
    catch (sql::SQLException &error)
    {
        // delete either object if it was created
        if (result != nullptr)
        {
            delete result;
        }
        if (statement != nullptr)
        {
            delete statement;
        }

        // print error description
        std::cerr << "Failed to read operation_mode from DB " << error.what() << std::endl;

        return OperationMode::UNKNOWN;
    }
}

// update the DB with the new method of operation if changed
// true if successfully updated, false if invalid mode or DB error
// use: DB.set_operation_mode(OperationMode::SABBATH);
bool DB::set_operation_mode(OperationMode operationMode)
{
    // prevent UNKNOWN being set as the state
    if (operationMode == OperationMode::UNKNOWN)
    {
        std::cerr << "Cannot set mode to UNKNOWN bruh" << std::endl;
        return false;
    }

    // check if DB connection valid
    if (!db_connect())
    {
        return false;
    }

    // convert given enum value into MariaDB string (using table mapping at top of file)
    // OperationMode::NORMAL -> "normal" (for DB)
    std::string operationModeText = map_operation_mode(operationMode);

    // hold the prepared text in a statement
    sql::PreparedStatement *statement = nullptr;

    try
    {
        // make the statement (? is placeholder)
        statement = con->prepareStatement(
            "UPDATE elevator_state "
            "SET operation_mode = ? "
            "WHERE state_id = 1 ");

        // replace the ? with valid text for the full query
        // following michael's example of pstmt->setInt(1, floorNum)
        statement->setString(1, operationModeText);

        // execute UPDATE command
        statement->executeUpdate();

        // delete statement pointer
        delete statement;

        // if successful logging, return true
        return true;
    }
    catch (sql::SQLException &error)
    {
        // delete statement if created but didn't complete query
        if (statement != nullptr)
        {
            delete statement;
        }

        //  print error
        std::cerr << "failed to updated operation_mode in DB " << error.what() << std::endl;

        // return false for fail
        return false;
    }
}

bool DB::set_doors_open(bool doorsOpen)
{
    // check if DB connection exists
    if (!db_connect())
    {
        return false;
    }

    // hold prepare statement
    sql::PreparedStatement *statement = nullptr;

    // query DB
    try
    {
        // make statement (? is place holder)
        statement = con->prepareStatement(
            "UPDATE elevator_state "
            "SET doors_open = ? "
            "WHERE state_id = 1");

        // replace ? with a valid value
        // true = 1, false = 0
        statement->setInt(1, doorsOpen ? 1 : 0);

        statement->executeUpdate();

        // delete the pointer
        delete statement;

        // true for valid execution
        return true;
    }
    catch (sql::SQLException &error)
    {

        // delete statement if it was created
        if (statement != nullptr)
        {
            delete statement;
        }

        // print the error
        std::cerr << "failed to update door status in DB " << error.what() << std::endl;

        // false for failed to update
        return false;
    }
}

int DB::get_doors_open()
{
    // ensure DB connection exists
    if (!db_connect())
    {
        return -1;
    }

    // query DB to read
    sql::Statement *statement = nullptr;
    sql::ResultSet *result = nullptr;

    try
    {
        statement = con->createStatement();

        result = statement->executeQuery(
            "SELECT doors_open "
            "FROM elevator_state "
            "WHERE state_id = 1 "
            "LIMIT 1");

        // the row was not found???
        if (!result->next())
        {
            delete result;
            delete statement;

            std::cerr << "open_doors was not found in the DB" << std::endl;

            return -1;
        }

        int doorState = result->getInt("doors_open");

        delete result;
        delete statement;

        return doorState;
    }
    catch (sql::SQLException &error)
    {
        // if it was created but query failed, delete it
        if (result != nullptr)
        {
            delete result;
        }

        // if it was created but query failed, delete it
        if (statement != nullptr)
        {
            delete statement;
        }

        std::cerr << "failed to read doors_open status from DB " << error.what() << std::endl;

        return -1;
    }
}
