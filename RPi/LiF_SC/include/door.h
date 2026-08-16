

/**
 * @brief Class to manage door state from CAN and DB updates. 
 * Combines virtual door state updates from 
 */
class Door {
    bool last_DB_door_open_state = true;    // assume door open by default
    bool last_CAN_door_open_state = true;   //  for safety reasons
    bool was_last_update_with_CAN = true;   // use CAN door state as default
    bool is_initialized_bool = false;

public:
    bool is_door_open();

    /**
     * @brief Returns true if state changed
     * @param new_DB_door_state 
     * @return 
     */
    bool update_door_DB(bool new_DB_door_state);
    
    /**
     * @brief Returns true if state changed
     * @param new_CAN_door_state 
     * @return 
     */
    bool update_door_CAN(bool new_CAN_door_state);

    void initialize(bool DB_state, bool CAN_state);

    bool is_initialized();

    bool is_using_DB_door();

};