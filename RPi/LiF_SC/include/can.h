#pragma once

#if !defined(DO_USE_CAN) || defined(_WIN32) || defined(_WIN64)
#include "pcan_proxy.h"
#else 
#include <libpcan.h> // used for TPCANMsg
#endif


class CAN {
public:
    /**
     * @brief Message IDs as per Elevator CAN protocol v1.1
     */
    enum class ID {
        SC_EC_FLOOR_RQ  = 0x100,    // command EC to move cabin
        EC_ALL_STATUS   = 0x101,    // EC cur position broadcast + heartbeat
        CC_SC_FLOOR_RQ  = 0x200,    // Car floor request
        SC_CC_VIRT_DOOR = 0x201,    // SC open/close "virtual" cabin door
        F1_RQ           = 0x301,    // Floor 1 request
        F2_RQ           = 0x302,    // Floor 2 request
        F3_RQ           = 0x303,    // Floor 3 request  
        UNKNOWN                     // any other ID                  
    };

    typedef struct {
        bool is_moving;
        bool is_enabled;
        uint8_t position;
    } EC_Status;

    typedef struct {
        bool is_requested;
    } Fx_Request;

    typedef struct {
        bool is_door_open;
        uint8_t floor_request;
    } CC_Request;

    typedef struct {
        uint16_t id;
        uint8_t data[8];
        uint8_t dlc;
    } Unknown_Frame;

    union RxMessageData {
        EC_Status ec_status;
        Fx_Request fx_request;
        CC_Request cc_request;
        Unknown_Frame unknown;
    };

    typedef struct {
        ID id;
        RxMessageData data;        
    } RxFrame;

private:
    const char* pcan_resource_path = "/dev/pcanusb32";

    HANDLE h2;
    DWORD status;

    /**
     * @brief Initialize CAN interface and claim this resource from OS
     * @return 
     */
    int pcanInit();

    /**
     * @brief Close CAN interface and release this resource from OS
     * @return 
     */
    int pcanClose();

    /**
     * @brief Attempt to receive a message. 
     * 
     * Helper function deprecated pcanFunction module.
     * @param msg pointer for where to save the message to
     * @return if a message is received = 1, no message received = 0, error = -1
     */
    int pcanRxState(TPCANMsg *msg);
    
    // ID intToID(int int_id);

public:
    CAN();
    
    ~CAN();
    
    
    int pcanTx(int id, int data);
    int pcanTx(ID id, int data);

    /**
     * @brief Command Elevator Controller to go to a certain floor
     * @param floor 
     */
    void ec_go_to_floor(unsigned int floor, bool ec_enable = true);

    /**
     * @brief Attempt to open/close door in the car cabin. Physical door switch 
     * in "doors open" position takes priority over this task
     * @param is_open 
     */
    void cc_set_doors(bool is_open);

    /**
     * @brief Attempt to receive and decode a CAN message
     * @param rx_buffer pointer to data buffer to store the received and decoded data
     * @param raw_msg pointer to data buffer to store the received raw data (for DB logging)
     * @return 1 if message is received and decoded, 0 if no message, -1 if error
     */
    int rx_can_frame(RxFrame* rx_buffer, TPCANMsg* raw_msg = nullptr);


    unsigned int get_status();
};