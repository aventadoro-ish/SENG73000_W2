#include "door.h"
#include <iostream>

bool Door::is_door_open() {
    if (!is_initialized_bool) {
        std::cerr << "Checking door state before Door module is initialized" << std::endl;
    }

    if (was_last_update_with_CAN) {
        return last_CAN_door_open_state;
    } else {
        return last_DB_door_open_state;
    }
}

void Door::update_door_DB(bool new_DB_door_state) {
    if (!was_last_update_with_CAN) {
        // last update used DB and this one is DB too
        // keep using DB door state

        if (last_DB_door_open_state != new_DB_door_state) {
            std::cout << "Door state commanded by DB: "
                << (new_DB_door_state ? "OPEN" : "CLOSED")
                << std::endl;
        }

        last_DB_door_open_state = new_DB_door_state;
    } else {
        // last update used CAN door state
        if (last_DB_door_open_state != new_DB_door_state) {
            was_last_update_with_CAN = false;
            last_DB_door_open_state = new_DB_door_state;
            std::cout << "Door state toggled by DB -> switching to Virtual Door State: "
                << (new_DB_door_state ? "OPEN" : "CLOSED")
                << std::endl;
        } else {
            // was not toggled -> stayed the same
            // ignore
        }
    }

}

void Door::update_door_CAN(bool new_CAN_door_state) {
    if (was_last_update_with_CAN) {
        // if using CAN door state -> keep using it
        if (last_CAN_door_open_state != new_CAN_door_state) {
            std::cout << "Door state commanded by CAN: "
                << (new_CAN_door_state ? "OPEN" : "CLOSED")
                << std::endl;
        }
        
        last_CAN_door_open_state = new_CAN_door_state;
        
    } else {
        // if last state update was DB
        if (last_CAN_door_open_state != new_CAN_door_state) {
            // and if state is toggled
            // then start using CAN door state
            was_last_update_with_CAN = true;
            last_CAN_door_open_state = new_CAN_door_state;
            std::cout << "Door state toggled by CAN -> switching to Physical Door State: "
                << (new_CAN_door_state ? "OPEN" : "CLOSED")
                << std::endl;
        } else {
            // was not toggled -> stayed the same
            // ignore
        }
    }
}

void Door::initialize(bool DB_state, bool CAN_state) {
    is_initialized_bool = true;
    last_CAN_door_open_state = CAN_state;
    last_DB_door_open_state = DB_state;
}

bool Door::is_initialized() {
    return is_initialized_bool;
}
