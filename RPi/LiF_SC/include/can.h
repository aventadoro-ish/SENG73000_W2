#pragma once

#if !defined(DO_USE_CAN) || defined(_WIN32) || defined(_WIN64)
#include "pcan_proxy.h"
#else 
#include <libpcan.h> // used for TPCANMsg
#endif


class CAN {
private:
    const char* pcan_resource_path = "/dev/pcanusb32";

    HANDLE h2;
    TPCANMsg Rxmsg;
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

    
    
    
public:
    CAN();
    
    ~CAN();
    
    
    int pcanTx(int id, int data);


private:

};