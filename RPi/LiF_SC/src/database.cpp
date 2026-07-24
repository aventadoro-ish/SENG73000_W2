#include "database.h"
#include <iostream>


std::string DB::map_table_name(DB::Tables table) {
    switch (table)
    {
    case Tables::CAN_LOGS:      return std::string("can_table_name");           break;
    default:
        std::cerr << "Invalid DB table value" << std::endl;
        break;
    }
    return std::string();
}
