#pragma once

#include <iostream>
#include <queue>


enum class TravelDir : unsigned int {
    UP,
    STATIONARY,
    DOWN
};

enum class RequestDir : unsigned int {
    UP,
    DOWN,
    NA
};

enum class RequestType : unsigned int {
    FLOOR,
    CAR,
    WEBSITE,
    MAINTENANCE
};

typedef struct {
    RequestType type;
    RequestDir dir;
    int floor;
} Request;


class Scheduler {
    int dynamic_travel_limit = 0;
    TravelDir car_dir = TravelDir::STATIONARY;

public:
    /**
     * @brief Add a new request to the scheduler algorithm
     * @param new_rq new request parameters
     * @return 0 - request added, and no new action is required. 
     * 1 - request changed which action needs to be performed now.
     * -1 if an error occurred
     */
    int add_request(Request new_rq);

    /**
     * @brief Which floor the cabin needs to go to
     * @return 0 if current floor does not need to change
     * -1 if error. New target floor value otherwise
     */
    int get_target_floor();

};