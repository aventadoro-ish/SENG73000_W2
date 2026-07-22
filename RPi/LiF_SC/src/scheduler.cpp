#include "request_queue.h"

void RequestQueue::set_travel_up() {
    if (this->dir == RequestQueue::TravelDir::UP) {
        return;
    }

    this->dir = RequestQueue::TravelDir::UP;

    std::priority_queue<int, std::vector<int>, std::greater<int>> temp_pq;

    while (!this->pq.empty()) {
        temp_pq.push(this->pq.top());
        this->pq.pop();
    }

    this->pq = temp_pq;

}


void RequestQueue::set_travel_down() {
    if (this->dir == RequestQueue::TravelDir::DOWN) {
        return;
    }

    this->dir = RequestQueue::TravelDir::DOWN;

    std::priority_queue<int, std::vector<int>, std::less<int>> temp_pq;

    while (!this->pq.empty()) {
        temp_pq.push(this->pq.top());
        this->pq.pop();
    }

    this->pq = temp_pq;

}