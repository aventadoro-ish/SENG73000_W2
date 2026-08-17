#pragma once

// import general libraries
#include <string>
#include <cstdint>
#include <stdlib.h>
#include <iostream>

// import SQL libraries
#include <mysql_connection.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>


class DB {
    // stores shared DB connection used by DB object
    // not defined yet so nullptr
    sql::Connection *con = nullptr;

    // Highest request ID already returned by read_floor_request().
    // This is only a reading cursor; it does not represent the request
    // currently being executed by the scheduler.
    int last_read_request_id = 0;

    // ID of the newest request that existed when request monitoring started
    int startup_request_id = 0;
    bool request_reader_initialized = false;

public:
    enum class Tables
    {
        CAN_LOGS,     // waiting on RY's schema to log CAN IDs directly
        ELEVATOR_LOGS // elevator request schema that logs types (remote/manual) and a few other details that are not raw CAN
    };

    enum class OperationMode
    {
        NORMAL,
        SABBATH,
        MAINTENANCE,
        FAULT,
        UNKNOWN
    };

    std::string map_table_name(DB::Tables table);
    std::string map_operation_mode(DB::OperationMode operationMode);

    /**
     * @brief Check if the database is active and can be connected to
     * @return true if connected, false if disconnected
     */
    bool db_connect();

    /**
     * @brief Add a database entry with a CAN frame
     * @param id message id as per CAN protocol
     * @param data pointer to int array of the given length
     * @param length length of the data array
     * @return 0 if ok, -1 if error
     */
    int log_can_message(int id, int *data, uint8_t length);

    /**
     * @brief log a manual request into the main elevator request DB
     * @param requestedFloor the floor that was requested for the elevator to move to
     * @param sourceID the sourceID - 0x200, 0x201, etc.
     * @return 1 for success, 0 for invalid/not inserted, -1 for DB failure
     */
    int log_elevator_request(int requestedFloor, int sourceID);

    /**
     * @brief Read new requested floor from DB
     * @return 0 if no new requests have been added, -1 if error, floor number
     * otherwise
     */
    int read_floor_request();

    /**
     * @brief Complete all pending remote requests for a served floor.
     * @param completedFloor Floor at which the elevator stopped.
     * @return true if at least one request was completed, false otherwise.
     */
    bool complete_elevator_request(int completedFloor);

    /**
     * @brief Read current mode of operation from the database
     * @return Mode of operation value
     */
    DB::OperationMode get_operation_mode();

    /**
     * @brief set current mode of operation (write to database)
     * @param operationMode to log the operation elevator is in -> normal, sabbath, etc.
     * @return true for successful update, false for fail
     */
    bool set_operation_mode(OperationMode operationMode);

    /**
     * @brief set the doors state in DB
     * @param true to open the doors
     * @param false to close the doors
     * @return true for success, false for fail
     */
    bool set_doors_open(bool doorsOpen);

    /**
     * @brief get the current door state from DB
     * @returns 1 for doors open, 0 for doors closed, -1 for DB error, 2 for using CAN-based door state (therefore ignore database door state)
     */
    int get_doors_open();
};