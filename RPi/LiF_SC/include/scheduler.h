#pragma once

#include <iostream>
#include <queue>

#include "setting.h"


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

inline std::ostream & operator << (std::ostream & outs, const Request & rq) {
    std::string rq_dir_str; 
    switch (rq.dir) {
    case RequestDir::UP:        rq_dir_str = "UP";          break;
    case RequestDir::DOWN:      rq_dir_str = "DOWN";        break;
    case RequestDir::NA:        rq_dir_str = "NO DIR";      break;    
    default:                    rq_dir_str = "invalid dir"; break;
    }

    std::string rq_type_str;
    switch (rq.type){
    case RequestType::FLOOR:        rq_type_str = "FLOOR";          break;
    case RequestType::CAR:          rq_type_str = "CAR";            break;
    case RequestType::WEBSITE:      rq_type_str = "WEBSITE";        break;
    case RequestType::MAINTENANCE:  rq_type_str = "MAINTENANCE";    break;
    default:                        rq_type_str = "invalid type";   break;
    }
    return outs << "RQ{type=" << rq_type_str << ", dir=" << rq_dir_str << ", floor=" << rq.floor << "}";
}


class Scheduler {
    int dynamic_travel_limit = 0;
    TravelDir car_dir = TravelDir::STATIONARY;
    unsigned int cur_floor = 0;
    unsigned int cur_target_floor = 0;

    /**
     * @brief Stores requests from floors. 
     * First idx = floor number - 1
     * Seconds idx: 0 => up; 1 => down
     */
    bool floor_requests[NUM_FLOORS][2];

    /**
     * @brief Stores requests made from inside the car
     * Idx = floor number - 1
     */
    bool car_requests[NUM_FLOORS];


    /**
     * @brief Perform scheduling algorithm using current state of member elements
     * @return Target floor to go to. 0 if no action is required
     */
    int run_scheduler();


public:
    Scheduler(unsigned int initial_floor = INITIAL_FLOOR) {
        this->cur_floor = initial_floor;
        this->cur_target_floor = initial_floor;
        for (int i = 0; i < static_cast<int>(NUM_FLOORS); i++) {
            this->car_requests[i] = false;
            this->floor_requests[i][0] = false;
            this->floor_requests[i][1] = false;
        }
    }


    /**
     * @brief Add a new request to the scheduler algorithm
     * @param new_rq new request parameters
     * @return 0 - request added, and no new action is required. 
     * 1 - request changed which action needs to be performed now.
     * -1 if an error occurred
     */
    int add_request(Request new_rq);

    /**
     * @brief Update current car position for scheduling algorithm
     * @param floor current floor
     */
    void update_car_position(int floor);

    /**
     * @brief This is used to remove requests from the request tables when the 
     * car has reached a target floor.
     * Does not clear floor requests in the opposite direction of travel, assuming 
     * cooperation from users -> if the elevator is not going the way you need it 
     * to go, you simply keep waiting on the floor
     */
    void register_car_stop();

    /**
     * @brief Run scheduler and determine which floor the cabin needs to go to
     * @return -1 if error. New target floor value otherwise
     */
    int get_target_floor();


    /**
     * @brief Print all scheduler state used by the scheduling algorithm.
     */
    void print_state(std::ostream &outs = std::cout) const;
};

/**
 * @brief Run an interactive, one-floor-per-tick scheduler simulation.
 *
 * Commands:
 *   tick  - advance the simulation by one step
 *   FdD   - add a floor request (for example F2U or F3D)
 *   Cd    - add a car request (for example C2)
 *   print - print scheduler and simulation state
 *   exit  - end the simulation
 */
void run_scheduler_manual_test();