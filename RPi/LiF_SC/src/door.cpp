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

bool Door::update_door_DB(bool new_DB_door_state) {
    const bool source_changed = was_last_update_with_CAN;

    // Compare against the currently effective door state, not simply
    // the previous value received from the database.
    const bool effective_state_changed =
        was_last_update_with_CAN
            ? last_CAN_door_open_state != new_DB_door_state
            : last_DB_door_open_state != new_DB_door_state;

    last_DB_door_open_state = new_DB_door_state;

    // A 0/1 value returned by get_doors_open() represents an explicit
    // database command, so the database becomes the active source.
    was_last_update_with_CAN = false;

    if (!source_changed && !effective_state_changed) {
        return false;
    }

    if (source_changed) {
        std::cout
            << "Door control switched from CAN to DB: "
            << (new_DB_door_state ? "OPEN" : "CLOSED")
            << std::endl;
    } else {
        std::cout
            << "Door state commanded by DB: "
            << (new_DB_door_state ? "OPEN" : "CLOSED")
            << std::endl;
    }

    // True means the effective command or its source changed.
    return true;
}

bool Door::update_door_CAN(bool new_CAN_door_state) {
    if (was_last_update_with_CAN) {
        // if using CAN door state -> keep using it
        if (last_CAN_door_open_state != new_CAN_door_state) {
            std::cout << "Door state commanded by CAN: "
                << (new_CAN_door_state ? "OPEN" : "CLOSED")
                << std::endl;
            last_CAN_door_open_state = new_CAN_door_state;
            return true;
        }
        last_CAN_door_open_state = new_CAN_door_state;
        return false;
        
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
            return true;
        } else {
            // was not toggled -> stayed the same
            // ignore
        }
    }
    return false;
}

void Door::initialize(bool DB_state, bool CAN_state) {
    is_initialized_bool = true;
    last_CAN_door_open_state = CAN_state;
    last_DB_door_open_state = DB_state;
}

bool Door::is_initialized() {
    return is_initialized_bool;
}

bool Door::is_using_DB_door() {
    return !was_last_update_with_CAN;
}
