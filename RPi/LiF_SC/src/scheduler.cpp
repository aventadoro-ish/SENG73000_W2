#include "scheduler.h"

#include <iostream>

#include <algorithm>
#include <cctype>
#include <climits>
#include <cstdlib>
#include <sstream>
#include <string>


int Scheduler::run_scheduler() {
    std::cout << "-> running scheduler" << std::endl;
    // First, check if there are no requests. If no requests -> stay still
    bool has_requests = false;
    for (int i = 0; i < static_cast<int>(NUM_FLOORS); i++) {
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
        this->car_dir = TravelDir::STATIONARY;
        std::cout << "\t-> no requests found" << std::endl;
        return this->cur_floor;
    }

    // At this point we know there is at least 1 request that needs to be processed

    if (this->car_dir == TravelDir::STATIONARY) {
        // currently waiting still
        // so simply find the closest request and go there
        
        int closest_rq_floor = -1;

        // max int ensures first comparison is closer than this value
        int closest_rq_distance = INT_MAX;  

        for (int i = 0; i < static_cast<int>(NUM_FLOORS); i++) {
            if (this->floor_requests[i][0] || 
                this->floor_requests[i][1] || 
                this->car_requests[i]) {

                // there's a request at that floor (either floor or car request)
                int distance = abs(this->cur_floor - i - 1);
                if (distance < closest_rq_distance) {
                    closest_rq_distance = distance;
                    closest_rq_floor = i + 1;
                }
            }
        }

        if (closest_rq_floor == -1) {
            std::cerr << "Fault in run_scheduler() function. Stationary car \
            could not find a request with lowest distance" << std::endl;
            return -1;
        }

        // closest request is at floor closest_rq_floor, so go to that floor
        if (closest_rq_floor > static_cast<int>(this->cur_floor)) {
            this->car_dir = TravelDir::UP;
            this->dynamic_travel_limit = closest_rq_floor;
            this->cur_target_floor = closest_rq_floor;
            std::cout << "\t-> stationary car found closest request at floor " << closest_rq_floor << std::endl;
            return closest_rq_floor;
        } else {
            std::cout << "closest request is at floor " << closest_rq_floor << std::endl;
            this->car_dir = TravelDir::DOWN;
            this->dynamic_travel_limit = closest_rq_floor;
            this->cur_target_floor = closest_rq_floor;
            std::cout << "\t-> stationary car found closest request at floor " << closest_rq_floor << std::endl;
            return closest_rq_floor;
        }
    }

   

    // at this point car is moving either UP or DOWN,
    //  so we need to check if there are any requests further in the 
    //  direction of travel
    // by default assume dynamic travel limit will be the new target floor
    //  unless a better target is found
    int new_target_floor = this->dynamic_travel_limit;
    switch (this->car_dir) {
    case TravelDir::UP: {

        // only go through the requests from floors higher than the current position
        //  to avoid abrupt stops at the current floor and backtracking
        //      note that this->cur_floor is used rather than this->cur_floor + 1
        //      due to index i always being less than corresponding floor number
        //      by 1
        // first we want to update the dynamic travel limit
        for (int i = this->cur_floor; i < static_cast<int>(NUM_FLOORS); i++) {
            if (this->floor_requests[i][1]) {
                // found a floor request in the opposite direction of travel
                if (i+1 > this->dynamic_travel_limit) {
                    // if this request is from a higher floor than the current
                    //  dynamic travel limit, we want to 
                    this->dynamic_travel_limit = i+1;
                    new_target_floor = this->dynamic_travel_limit;
                }
            }
        }

        // then we check for stops on the way to the new dynamic travel limit
        for (int i = this->cur_floor; i < static_cast<int>(NUM_FLOORS); i++) {
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
        for (int i = static_cast<int>(this->cur_floor) - 2; i >= 0; --i) {
            if (this->floor_requests[i][1]) {
                // found a floor request in the opposite direction of travel
                if (i+1 < this->dynamic_travel_limit) {
                    // if this request is from a lower floor than the current
                    //  dynamic travel limit, we want to update the dynamic travel
                    //  limit 
                    this->dynamic_travel_limit = i+1;
                    new_target_floor = this->dynamic_travel_limit;
                }
            }
        }

        // then we check for stops on the way to the new dynamic travel limit
        for (int i = static_cast<int>(this->cur_floor) - 2; i >= 0; --i) {
            if (this->car_requests[i] && i+1 > this->dynamic_travel_limit) {
                // found a car request which is on the way to the current destination
                //  so make a stop there

                if (i + 1 > new_target_floor) {
                    // only change target floor if this floor is higher than current target
                    new_target_floor = i+1;
                }
            }

            if (this->floor_requests[i][1]) {
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


    if (this->cur_floor == static_cast<unsigned int>(this->dynamic_travel_limit) && 
        this->cur_floor == this->cur_target_floor && 
        this->cur_floor == static_cast<unsigned int>(new_target_floor)
    ) {
        if (this->car_dir == TravelDir::UP) {
            bool has_requests_below = false;
            for (int i = static_cast<int>(this->cur_floor) - 2; i >= 0; i--) {
                if (this->car_requests[i] || this->floor_requests[i][0] || this->floor_requests[i][1]) {
                    has_requests_below = true;
                    break;
                }
            }
            if (has_requests_below) {
                std::cout << "DTL reached -> reverse to down" << std::endl;
                this->car_dir = TravelDir::DOWN;
            } else {
                std::cout << "DTL reached w/o requests below left -> stay still" << std::endl;
                this->car_dir = TravelDir::STATIONARY;
            }

        } else if (this->car_dir == TravelDir::DOWN) {
            bool has_requests_above = false;
            for (int i = static_cast<int>(this->cur_floor); i < static_cast<int>(NUM_FLOORS); ++i) {
                if (this->car_requests[i] || this->floor_requests[i][0] || this->floor_requests[i][1]) {
                    has_requests_above = true;
                    break;
                }
            }
            if (has_requests_above) {
                std::cout << "DTL reached -> reverse to up" << std::endl;
                this->car_dir = TravelDir::UP;
            } else {
                std::cout << "DTL reached w/o requests above left -> stay still" << std::endl;
                this->car_dir = TravelDir::STATIONARY;
            }

        } else {
            std::cerr << "INVALID TRAVEL DIIIIR" << std::endl;
        }
    }

    std::cout << "\t-> moving car found next request at floor " << new_target_floor << std::endl;
    this->cur_target_floor = new_target_floor;
    return new_target_floor;
}

int Scheduler::add_request(Request new_rq) {
    // if (this->rq_size == rq_buffer_size) {
    //     std::cerr << "Scheduler error! Request buffer size exceeded, unable to add new request: " << new_rq << std::endl;
    // }

    if (new_rq.type == RequestType::CAR) {
        this->car_requests[new_rq.floor-1] = true;

        // car requests in the direction of travel and further away from the 
        //  car than DTL should update DTL
        if (this->car_dir == TravelDir::UP && 
            new_rq.floor > this->dynamic_travel_limit && 
            new_rq.floor > static_cast<int>(this->cur_floor)) {
                this->dynamic_travel_limit = new_rq.floor;

        } else if (this->car_dir == TravelDir::DOWN && 
            new_rq.floor < this->dynamic_travel_limit && 
            new_rq.floor < static_cast<int>(this->cur_floor)) {
                this->dynamic_travel_limit = new_rq.floor;

        }

    } else if (new_rq.type == RequestType::FLOOR) {
        int updown_idx = new_rq.dir == RequestDir::UP ? 0 : 1;
        this->floor_requests[new_rq.floor-1][updown_idx] = true;
    }
    
    int cur_target = this->cur_target_floor;
    this->run_scheduler();

    return (cur_target != static_cast<int>(this->cur_target_floor));
}

void Scheduler::update_car_position(int floor) {
    this->cur_floor = floor;
}

void Scheduler::register_car_stop() {
    const int floor_idx = static_cast<int>(this->cur_floor) - 1;
    const TravelDir arrival_dir = this->car_dir;

    // A car request is always satisfied when the car stops here.
    this->car_requests[floor_idx] = false;

    if (arrival_dir == TravelDir::UP) {
        // Serve an UP floor call while travelling up.
        this->floor_requests[floor_idx][0] = false;

        // At the upper travel limit, the car can also serve a DOWN call
        // before reversing.
        if (this->cur_floor == static_cast<unsigned int>(this->dynamic_travel_limit)) {
            this->floor_requests[floor_idx][1] = false;
        }
    } else if (arrival_dir == TravelDir::DOWN) {
        // Serve a DOWN floor call while travelling down.
        this->floor_requests[floor_idx][1] = false;

        // At the lower travel limit, the car can also serve an UP call
        // before reversing.
        if (this->cur_floor == static_cast<unsigned int>(this->dynamic_travel_limit)) {
            this->floor_requests[floor_idx][0] = false;
        }
    } else {
        // The car was already waiting at this floor.
        this->floor_requests[floor_idx][0] = false;
        this->floor_requests[floor_idx][1] = false;
    }

    // Make the next scheduling decision only after all requests served
    // by this stop have been cleared.
    this->run_scheduler();
}

int Scheduler::get_target_floor() {
    this->run_scheduler();
    std::cout << "get_tgt cur_target_floor=" << this->cur_target_floor << std::endl;
    return this->cur_target_floor;
}


namespace {

const char *to_string(TravelDir dir) {
    switch (dir) {
    case TravelDir::UP:         return "UP";
    case TravelDir::STATIONARY: return "STATIONARY";
    case TravelDir::DOWN:       return "DOWN";
    }
    return "INVALID";
}

std::string trim(std::string text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }

    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

bool parse_floor_number(const std::string &text, unsigned int &floor) {
    if (text.empty() ||
        !std::all_of(text.begin(), text.end(),
                     [](unsigned char ch) { return std::isdigit(ch); })) {
        return false;
    }

    try {
        const unsigned long parsed = std::stoul(text);
        if (parsed < 1 || parsed > NUM_FLOORS) {
            return false;
        }
        floor = static_cast<unsigned int>(parsed);
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace

void Scheduler::print_state(std::ostream &outs) const {
    outs << "Scheduler state\n"
         << "  current floor:        " << this->cur_floor << '\n'
         << "  target floor:         " << this->cur_target_floor << '\n'
         << "  travel direction:     " << to_string(this->car_dir) << '\n'
         << "  dynamic travel limit: " << this->dynamic_travel_limit << '\n'
         << "  requests:\n"
         << "    floor | up | down | car\n"
         << "    ------+----+------+----\n";

    for (unsigned int i = 0; i < NUM_FLOORS; ++i) {
        outs << "    " << (i + 1) << "     | "
             << (this->floor_requests[i][0] ? "X " : ". ")
             << " | " << (this->floor_requests[i][1] ? "X   " : ".   ")
             << " | " << (this->car_requests[i] ? 'X' : '.') << '\n';
    }
}

void run_scheduler_manual_test() {
    Scheduler scheduler;
    unsigned int simulation_floor = INITIAL_FLOOR;
    unsigned long simulation_tick = 0;
    bool car_is_moving = false;
    bool running = true;

    std::cout
        << "Elevator scheduler manual test\n"
        << "Commands: tick, F<floor><U|D>, C<floor>, print, exit\n"
        << "Examples: F2U, F3D, C2\n";

    while (running) {
        std::cout << "\nscheduler> ";

        std::string command;
        if (!std::getline(std::cin, command)) {
            std::cout << "\nInput closed; ending simulation.\n";
            break;
        }
        command = trim(command);

        if (command == "exit") {
            running = false;
        } else if (command == "print") {
            std::cout << "Simulation state\n"
                      << "  tick:             " << simulation_tick << '\n'
                      << "  simulated floor:  " << simulation_floor << '\n'
                      << "  car moving:       "
                      << (car_is_moving ? "yes" : "no") << '\n';
            scheduler.print_state();
        } else if (command == "tick") {
            const int target_floor = scheduler.get_target_floor();
            if (target_floor < 1 ||
                target_floor > static_cast<int>(NUM_FLOORS)) {
                std::cout << "Scheduler returned invalid target floor "
                          << target_floor << ".\n";
                continue;
            }

            ++simulation_tick;
            car_is_moving =
                simulation_floor != static_cast<unsigned int>(target_floor);           
            if (simulation_floor < static_cast<unsigned int>(target_floor)) {
                ++simulation_floor;
                scheduler.update_car_position(
                    static_cast<int>(simulation_floor));
                std::cout << "Tick " << simulation_tick << ": moved up to floor "
                          << simulation_floor << ".\n";
            } else if (simulation_floor >
                       static_cast<unsigned int>(target_floor)) {
                --simulation_floor;
                scheduler.update_car_position(
                    static_cast<int>(simulation_floor));
                std::cout << "Tick " << simulation_tick
                          << ": moved down to floor "
                          << simulation_floor << ".\n";
            } else {
                std::cout << "Tick " << simulation_tick
                          << ": car remains on floor "
                          << simulation_floor << ".\n";
            }

            if (simulation_floor ==
                static_cast<unsigned int>(target_floor)) {
                car_is_moving = false;
                scheduler.register_car_stop();
                std::cout << "Stopped at target floor "
                          << simulation_floor << ".\n";
            }

            std::cout << "Simulation state\n"
                << "  tick:             " << simulation_tick << '\n'
                << "  simulated floor:  " << simulation_floor << '\n'
                << "  car moving:       "
                << (car_is_moving ? "yes" : "no") << '\n';
            scheduler.print_state();

        } else if (command.size() >= 3 &&
                   std::toupper(static_cast<unsigned char>(command.front())) ==
                       'F') {
            const char direction_char = static_cast<char>(
                std::toupper(static_cast<unsigned char>(command.back())));
            unsigned int request_floor = 0;

            if ((direction_char != 'U' && direction_char != 'D') ||
                !parse_floor_number(
                    command.substr(1, command.size() - 2), request_floor)) {
                std::cout << "Invalid floor request. Use F<floor><U|D>, "
                          << "with floor in 1.." << NUM_FLOORS << ".\n";
                continue;
            }

            Request request{
                RequestType::FLOOR,
                direction_char == 'U' ? RequestDir::UP : RequestDir::DOWN,
                static_cast<int>(request_floor)
            };

            scheduler.add_request(request);
            std::cout << "Added " << request << ".\n";
                        std::cout << "Simulation state\n"
                << "  tick:             " << simulation_tick << '\n'
                << "  simulated floor:  " << simulation_floor << '\n'
                << "  car moving:       "
                << (car_is_moving ? "yes" : "no") << '\n';
            scheduler.print_state();

        } else if (command.size() >= 2 &&
                   std::toupper(static_cast<unsigned char>(command.front())) ==
                       'C') {
            unsigned int request_floor = 0;
            if (!parse_floor_number(command.substr(1), request_floor)) {
                std::cout << "Invalid car request. Use C<floor>, "
                          << "with floor in 1.." << NUM_FLOORS << ".\n";
                continue;
            }

            Request request{
                RequestType::CAR,
                RequestDir::NA,
                static_cast<int>(request_floor)
            };
            if (scheduler.add_request(request) == 0) {
                std::cout << "Added " << request << ".\n";
            }
            std::cout << "Simulation state\n"
                << "  tick:             " << simulation_tick << '\n'
                << "  simulated floor:  " << simulation_floor << '\n'
                << "  car moving:       "
                << (car_is_moving ? "yes" : "no") << '\n';
            scheduler.print_state();
        } else if (!command.empty()) {
            std::cout << "Unknown command. Valid commands are: tick, "
                      << "F<floor><U|D>, C<floor>, print, exit.\n";
        }
    }

    std::cout << "Simulation ended.\n";
}
