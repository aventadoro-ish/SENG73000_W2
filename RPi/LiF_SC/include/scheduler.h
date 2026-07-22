#pragma once

#include <iostream>
#include <queue>


class RequestQueue {
    struct Compare {
        bool min_priority = true;

        bool operator()(int lhs, int rhs) const {
            if (min_priority) {
                return lhs > rhs; // Smallest value at top
            }

            return lhs < rhs;     // Largest value at top
        }
    };

    enum class TravelDir : unsigned int {
        UP,
        STATIONARY,
        DOWN
    };

    using PriorityQueue =
    std::priority_queue<int, std::vector<int>, Compare>;

    PriorityQueue pq{Compare{true}};
    TravelDir dir = TravelDir::STATIONARY;

    void rebuild_queue(bool min_priority);


public:
    void set_travel_up();

    void set_travel_down();

    void set_stationary();




};
