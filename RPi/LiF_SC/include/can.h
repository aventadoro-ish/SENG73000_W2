#pragma once

#ifdef DO_USE_CAN
#include <libpcan.h> // used for TPCANMsg
#else 
#include "pcan_proxy.h"
#endif


class CAN {
private:
    const char* pcan_resource_path = "/dev/pcanusb32";

    HANDLE h2;
    TPCANMsg Txmsg;
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

    // Function declarations
    int pcanTx(int id, int data);

    int pcanRx(int num_msgs);

public:
    CAN();

    ~CAN();


private:

};