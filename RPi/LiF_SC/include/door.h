

/**
 * @brief Class to manage door state from CAN and DB updates. 
 * Combines virtual door state updates from 
 */
class Door {
    bool last_DB_door_state;
    bool last_CAN_door_state;

public:
    bool is_door_open();

    void update_door_DB(bool new_DB_door_state);

    void update_door_CAN(bool new_CAN_door_state);

};