#pragma once

#include <string>
#include <cstdint>


class DB {
    // Nick, put whatever state/variables you need here
        
public:
    enum class Tables {
        CAN_LOGS
        // etc...
    };

    enum class OperationMode {
        NORMAL,
        SABBATH,
        MAINTENANCE,
        FAULT
    };

    std::string map_table_name(DB::Tables table);
    
    /**
     * @brief Add a database entry with a CAN frame
     * @param id message id as per CAN protocol
     * @param data pointer to int array of the given length 
     * @param length length of the data array
     * @return 0 if ok, -1 if error
     */
    int log_can_message(int id, int* data, uint8_t length);

    /**
     * @brief Read new requested floor from DB
     * @return 0 if no new requests have been added, -1 if error, floor number 
     * otherwise
     */
    int read_floor_request();

    /**
     * @brief Read current mode of operation from the database
     * @return Mode of operation value
     */
    DB::OperationMode get_operation_mode();


    /**
     * @brief Check if the database is active and can be connected to
     * @return true if connected, false if disconnected
     */
    bool is_db_connected();

};