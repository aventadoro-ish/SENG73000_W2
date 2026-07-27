#include <iostream>
#include "database.h"

int main()
{
    DB db;

    // Simulate a request message received from the Floor 2 controller
    int testData[1] = {0x01};

    int result = db.log_can_message(
        0x202,
        testData,
        1
    );

    std::cout << "CAN log result: " << result << std::endl;

    return 0;
}