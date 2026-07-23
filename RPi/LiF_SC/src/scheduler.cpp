#include "scheduler.h"

#include <iostream>

int Scheduler::run_scheduler() {

    // First, check if there are no requests. If no requests -> stay still
    bool has_requests = false;
    for (int i = 0; i < NUM_FLOORS; i++) {
        if (this->floor_requests[i][0] || this->floor_requests[i][1]) {
            has_requests = true;
            break;
        }
        if (this->car_requests[i]) {
            has_requests = true;
            break;
        }
    }
    if (!has_requests) {
        this->car_dir == TravelDir::STATIONARY;
        return this->cur_floor;
    }

    // At this point we know there is at least 1 request that needs to be processed

    if (this->car_dir == TravelDir::STATIONARY) {
        // currently waiting still
        // so simply find the closest request and go there
        
        int closest_rq_floor = -1;

        // max int ensures first comparison is closer than this value
        int closest_rq_distance = INT_MAX;  

        for (int i = 0; i < NUM_FLOORS; i++) {
            if (this->floor_requests[i][0] || 
                this->floor_requests[i][1] || 
                this->car_requests[i]) {

                // there's a request at that floor (either floor or car request)
                int distance = abs(this->cur_floor - i);
                if (distance < closest_rq_distance) {
                    closest_rq_distance = distance;
                    closest_rq_floor = i;
                }
            }
        }

        if (closest_rq_floor == -1) {
            std::cerr << "Fault in run_scheduler() function. Stationary car \
            could not find a request with lowest distance" << std::endl;
            return -1;
        }

        // closest request is at floor closest_rq_floor, so go to that floor
        if (closest_rq_floor > this->cur_floor) {
            this->car_dir = TravelDir::UP;
            this->dynamic_travel_limit = closest_rq_floor + 1;
            return closest_rq_floor + 1;
        }
    }

   

    // at this point car is moving either UP or DOWN,
    //  so we need to check if there are any requests further in the 
    //  direction of travel
    // by default assume current target floor will be the new target floor
    //  unless a better target is found
    int new_target_floor = this->cur_target_floor;
    switch (this->car_dir) {
    case TravelDir::UP: {

        // only go through the requests from floors higher than the current position
        //  to avoid abrupt stops at the current floor and backtracking
        //      note that this->cur_floor is used rather than this->cur_floor + 1
        //      due to index i always being less than corresponding floor number
        //      by 1
        // first we want to update the dynamic travel limit
        for (int i = this->cur_floor; i <= NUM_FLOORS; i++) {
            if (this->floor_requests[i][1]) {
                // found a floor request in the opposite direction of travel
                if (i+1 > this->dynamic_travel_limit) {
                    // if this request is from a higher floor than the current
                    //  dynamic travel limit, we want to 
                    this->dynamic_travel_limit = i+1;
                }
            }
        }

        // then we check for stops on the way to the new dynamic travel limit
        for (int i = this->cur_floor; i <= NUM_FLOORS; i++) {
            if (this->car_requests[i] && i+1 < this->dynamic_travel_limit) {
                // found a car request which is on the way to the current destination
                //  so make a stop there

                if (i < new_target_floor) {
                    // only change target floor if this floor is lower than current target
                    new_target_floor = i+1;
                }
            }

            if (this->floor_requests[i][0]) {
                // found a floor request in the direction of travel

                if (i+1 < this->dynamic_travel_limit && i+1 < new_target_floor) {
                    // this request comes from a floor which is lower than DTL 
                    //  and current target floor, so stop on the way
                    // this increases occupancy of the elevator but may optimize
                    //  the route if the person who gets in on this floor wants
                    //  to go to a destination which is already on the way
                    new_target_floor = i+1;
                }
            }
        }
        break;
    }
    case TravelDir::DOWN: {
        // only go through the requests from floors lower than the current position
        //  to avoid abrupt stops at the current floor and backtracking
        //      note that this->cur_floor - 1 is used rather than this->cur_floor
        //      due to index i always being less than corresponding floor number
        //      by 1
        // first we want to update the dynamic travel limit
        for (int i = this->cur_floor - 1; i >= 0; i--) {
            if (this->floor_requests[i][1]) {
                // found a floor request in the opposite direction of travel
                if (i+1 < this->dynamic_travel_limit) {
                    // if this request is from a lower floor than the current
                    //  dynamic travel limit, we want to update the dynamic travel
                    //  limit 
                    this->dynamic_travel_limit = i+1;
                }
            }
        }

        // then we check for stops on the way to the new dynamic travel limit
        for (int i = this->cur_floor; i < NUM_FLOORS; i++) {
            if (this->car_requests[i] && i+1 > this->dynamic_travel_limit) {
                // found a car request which is on the way to the current destination
                //  so make a stop there

                if (i > new_target_floor) {
                    // only change target floor if this floor is higher than current target
                    new_target_floor = i+1;
                }
            }

            if (this->floor_requests[i][0]) {
                // found a floor request in the direction of travel

                if (i+1 > this->dynamic_travel_limit && i+1 > new_target_floor) {
                    // this request comes from a floor which is higher than DTL 
                    //  and current target floor, so stop on the way
                    // this increases occupancy of the elevator but may optimize
                    //  the route if the person who gets in on this floor wants
                    //  to go to a destination which is already on the way
                    new_target_floor = i+1;
                }
            }
        }
        break;
    }
    default:
        std::cerr << "Car travel direction is invalid in run_scheduler()" << std::endl;
        break;
    }

    return new_target_floor;
}

int Scheduler::add_request(Request new_rq)
{
    // if (this->rq_size == rq_buffer_size) {
    //     std::cerr << "Scheduler error! Request buffer size exceeded, unable to add new request: " << new_rq << std::endl;
    // }

    if (new_rq.type == RequestType::CAR) {
        this->car_requests[new_rq.floor-1] = true;
    } else if (new_rq.type == RequestType::FLOOR) {
        int updown_idx = new_rq.dir == RequestDir::UP ? 0 : 1;
        this->floor_requests[new_rq.floor-1][updown_idx] = true;
    }

    return 0;
}

void Scheduler::update_car_position(int floor) {
    this->cur_floor = floor;
}

void Scheduler::register_car_stop() {
    if (this->car_dir == TravelDir::UP) {
        this->floor_requests[this->cur_floor - 1][0] = false;
        this->car_requests[this->cur_floor - 1] = false;
    } else if (this->car_dir == TravelDir::DOWN) {
        this->floor_requests[this->cur_floor - 1][1] = false;
        this->car_requests[this->cur_floor - 1] = false;
    }
}

int Scheduler::get_target_floor() {
    this->run_scheduler();
    return this->cur_target_floor;
}
