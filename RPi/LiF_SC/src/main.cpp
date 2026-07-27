#include <stdio.h>
#include <string>
#include <cstdlib>
#include <stdlib.h>
#include <unistd.h> 
#include <iostream>
// #include <time.h> // for timeout
#include <chrono>

#include "../include/pcanFunctions.h"
#include "../include/databaseFunctions.h"
#include "../include/mainFunctions.h"

#include "setting.h"
#include "scheduler.h"
#include "can.h"
#include "database.h"

using namespace std;

// -----------------------------------------------------------------------------
// Global Declarations
// -----------------------------------------------------------------------------

/**
 * @brief States of supervisory controller FSM
 */
enum class SC_State : unsigned int {
	INITIALIZE,
	IDLE,
	MOVING,
	SABBATH_IDLE,
	SABBATH_MOVING,
	MAINTENANCE,
	MAINTENANCE_MOVING,
	FAULT
};


// -----------------------------------------------------------------------------
// Global variables
// -----------------------------------------------------------------------------
bool is_running = true;	// main loop
SC_State state = SC_State::INITIALIZE;

Scheduler scheduler;
CAN can_link;
DB database;

int current_floor = INITIAL_FLOOR;
int current_target = INITIAL_FLOOR;
bool is_car_moving = false; 
bool is_CC_door_open = false;
bool is_EC_enabled = false;

uint64_t time_last_EC_heartbeat;
uint64_t time_move_start;
uint64_t time_move_finish;		// used to wait on a floor for a bit before 
								// moving to the next target
								// TODO: change when servo doors are added

// keep track of floors that have been requested through DB
bool floor_requests_from_DB[NUM_FLOORS];



// -----------------------------------------------------------------------------
// Function declarations
// -----------------------------------------------------------------------------
void FSM_normal_mode();

void FSM_sabbath_mode();

void FSM_maintenance_mode();

/**
 * @brief Starts up the CAN network and DB connection. 
 * Makes sure all nodes are present and functional and DB is up.
 */
void FSM_initialize();

/**
 * @brief Stop operations and wait for maintenance command from DB
 * @param fault_reason string description of what caused the fault condition
 */
void FSM_fault_mode(std::string fault_reason = "");


/**
 * @brief Attempts to read and process a received CAN message.
 * May change state, current_floor, is_car_moving, and is_CC_door_open
 */
void process_CAN_msg();


void process_CAN_CC_msg(CAN::RxFrame rxMsg);

void process_CAN_Fx_msg(CAN::RxFrame rxMsg);

void process_CAN_EC_msg(CAN::RxFrame rxMsg);

/**
 * @brief Arduino-style millis function for non-blocking delays
 * @return timestamp in milliseconds
 */
uint64_t current_time();


// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------
int main() {
	// run_scheduler_manual_test();
	// return 0;

	scheduler = Scheduler();

	// for (int i = 0; i < 10; i++) {
	// 	int res = can_link.pcanTx(0x100, i);
	// 	std::cout << "Tx message test result " << std::hex << res << std::dec  << std::endl;
	// 	sleep(1);
	// }
	// return 1;



	
	printf("\nLithium Firefly Project. Initializing SC FSM\nProgram settings\n");
	printf(" - number of floors: %u\n", NUM_FLOORS);
	printf(" - queued floor wait time (ms): %lu\n", FLOOR_WAIT_DELAY_MS);
	printf(" - sabbath mode wait time (ms): %lu\n", SABBATH_MOVE_DELAY_MS);
	printf(" - initial floor: %u\n", INITIAL_FLOOR);


	// initialize variables  
	int targetFloor = 1;
	time_t moveStartTime = 0;

	while(is_running) { // FSM infinite loop start
		
		switch (state) {
			case SC_State::INITIALIZE:
				FSM_initialize();
				break;
			case SC_State::IDLE:
			case SC_State::MOVING:
				FSM_normal_mode();
				break;
			case SC_State::SABBATH_IDLE:
			case SC_State::SABBATH_MOVING:
				FSM_sabbath_mode();
				break;
			case SC_State::MAINTENANCE:
			case SC_State::MAINTENANCE_MOVING:
				FSM_maintenance_mode();
				break;
			default:

				break;
		}

	}
	
	return 0;
}


// -----------------------------------------------------------------------------
// Function definitions
// -----------------------------------------------------------------------------
void FSM_normal_mode() {
	if (state == SC_State::MOVING) {
		// check if move timed out
		if (current_time() - time_move_start > MOVE_FINISH_TIMEOUT_MS) {
			state = SC_State::FAULT;
			char c_str_reason[1000];
			sprintf("Move timed out in NORMAL mode after %lums. Max time is %lu",
				c_str_reason, 
				current_time() - time_move_start, MOVE_FINISH_TIMEOUT_MS
			);	
			FSM_fault_mode(std::string(c_str_reason));
			return;
		}

		// move finished
		if (!is_car_moving) {
			time_move_finish = current_time();
			state = SC_State::IDLE;
		}
	}
	
	// check for new target floor
	int new_target = scheduler.get_target_floor();
	if (new_target != current_target) {
		// current assumption: extra target verification is not needed
		//	because scheduler handles it

		// only move after spending some time on the floor
		if ((current_time() - time_move_finish > FLOOR_WAIT_DELAY_MS) && state == SC_State::IDLE) {
			can_link.ec_go_to_floor(new_target);
			current_target = new_target;
			is_car_moving = true;
			time_move_start = current_time();
			state = SC_State::MOVING;
		}
	}


	DB::OperationMode new_db_op_mode = database.get_operation_mode();
	switch (new_db_op_mode) {
	case DB::OperationMode::NORMAL:
		// do nothing
		break;
	case DB::OperationMode::SABBATH:
		state = is_car_moving ? SC_State::SABBATH_MOVING : SC_State::SABBATH_IDLE;
		break;
	case DB::OperationMode::MAINTENANCE:
		state = is_car_moving ? SC_State::MAINTENANCE_MOVING : SC_State::MAINTENANCE;
		break;
	case DB::OperationMode::FAULT:
		state = SC_State::FAULT;
	default:
		break;
	}

}

void FSM_sabbath_mode() {
	static bool is_going_up = true;
	if (state == SC_State::SABBATH_MOVING) {
		// check if move timed out
		if (current_time() - time_move_start > MOVE_FINISH_TIMEOUT_MS) {
			state = SC_State::FAULT;
			char c_str_reason[1000];
			sprintf("Move timed out in SABBATH mode after %lums. Max time is %lu", 
				c_str_reason, 
				current_time() - time_move_start, MOVE_FINISH_TIMEOUT_MS
			);	
			FSM_fault_mode(std::string(c_str_reason));
			return;
		}

		// move finished
		if (!is_car_moving) {
			time_move_finish = current_time();
			state = SC_State::SABBATH_IDLE;
		}
	}
	


	// only move after spending some time on the floor
	if (current_time() - time_move_finish > SABBATH_MOVE_DELAY_MS && 
			state == SC_State::SABBATH_IDLE) {
		if (is_going_up) {
			if (current_floor < NUM_FLOORS) {
				current_target = current_floor++;
			} else {
				is_going_up = false;
				current_target = NUM_FLOORS - 1;
			}
		} else {
			if (current_floor > 1) {
				current_target = current_floor--;
			} else {
				is_going_up = true;
				current_target = 2;
			}
		}

		can_link.ec_go_to_floor(current_target);
		is_car_moving = true;
		time_move_start = current_time();
		state = SC_State::SABBATH_MOVING;
	}



	DB::OperationMode new_db_op_mode = database.get_operation_mode();
	switch (new_db_op_mode) {
	case DB::OperationMode::NORMAL:
		state = is_car_moving ? SC_State::MOVING : SC_State::IDLE;
		break;
	case DB::OperationMode::SABBATH:
		// do nothing
		break;
	case DB::OperationMode::MAINTENANCE:
		state = is_car_moving ? SC_State::MAINTENANCE_MOVING : SC_State::MAINTENANCE;
		break;
	case DB::OperationMode::FAULT:
		state = SC_State::FAULT;
	default:
		break;
	}


}

void FSM_maintenance_mode() {
	// TODO: implement
}	

void FSM_initialize() {
	static uint64_t time_last_print = 0;
	if (!database.db_connect()) {
		if (current_time() - time_last_print > 1000) {
			time_last_print = current_time();
			std::cout << "Initializing. Waiting on DB connection..." << std::endl;
		}
		return;
	}
	if (!is_EC_enabled) {
		if (current_time() - time_last_print > 1000) {
			time_last_print = current_time();
			std::cout << "Initializing. Waiting on EC homing..." << std::endl;
		}
		return;
	}

	// TODO: add node status checks when new CAN bus protocol is implemented

	// atp both db is connected and EC is enabled -> init finished
	state = SC_State::IDLE;
}

void FSM_fault_mode(std::string fault_reason) {
	// used to check if this is the first time processing a new FAULT state
	static bool is_new_fault = true;
	static uint64_t time_last_print = 0;

	if (is_new_fault) {
		// for a new fault that has not been processed before, we need to inform 
		// 	other nodes of a FAULT condition
		can_link.ec_go_to_floor(0, false);	// disable EC
	}

	if (current_time() - time_last_print > 2000) {
		time_last_print = current_time();
		std::cout << "FAULT MODE. Reason: " << fault_reason << std::endl;
	}

	// used to exit FAULT mode into MAINTENANCE.
	//	If fault occurred in maintenance mode, elevator should not exit fault 
	//	mode by default. Toggle mode from MAINTENANCE to any other mode and put 
	//	it back into MAINTENANCE to exit fault.
	//	To go back to nominal operation, go to MAINTENANCE mode first, then to 
	//	 the required mode
	static DB::OperationMode last_db_op_mode = DB::OperationMode::NORMAL;
	
	DB::OperationMode cur_db_mode = database.get_operation_mode();

	if (last_db_op_mode != DB::OperationMode::FAULT && cur_db_mode != DB::OperationMode::FAULT) {
		// DB has not been informed of a fault yet
		database.set_operation_mode(DB::OperationMode::FAULT);
		last_db_op_mode = DB::OperationMode::FAULT;
		return;
	} else if (last_db_op_mode == DB::OperationMode::FAULT && cur_db_mode == DB::OperationMode::MAINTENANCE) {
		state = SC_State::MAINTENANCE;
		can_link.ec_go_to_floor(0, true);	// enable EC
		is_new_fault = true;	// update static variable for next FAULT condition
	}



}

void process_CAN_msg() {
	CAN::RxFrame rxMsg;
	TPCANMsg rawRxMsg;

	int res = can_link.rx_can_frame(&rxMsg, &rawRxMsg);
	if (res == 0) {
		// no message to process
		return;
	} else if (res == -1) {
		state = SC_State::FAULT;
		unsigned int can_status = can_link.get_status();
		char c_str_reason[1000];
		sprintf("CAN receive failed with status 0x%x", c_str_reason, can_status);	
		FSM_fault_mode(std::string(c_str_reason));
		return;
	}

	int data_arr[8];
	for (int i = 0; i < rawRxMsg.LEN; i++) {
		data_arr[i] = rawRxMsg.DATA[i];
	}
	database.log_can_message(rawRxMsg.ID, data_arr, rawRxMsg.LEN);

	if (rxMsg.id == CAN::ID::UNKNOWN) {
		state = SC_State::FAULT;
		char c_str_reason[1000];
		sprintf("CAN received a message with unknown ID=0x%x, data=0x%d %d %d %d %d %d %d %d, dlc=%d",
			c_str_reason, 
			rxMsg.id,
			rxMsg.data.unknown.data[0], 	rxMsg.data.unknown.data[1],
			rxMsg.data.unknown.data[2],		rxMsg.data.unknown.data[3],
			rxMsg.data.unknown.data[4],		rxMsg.data.unknown.data[5],
			rxMsg.data.unknown.data[6],		rxMsg.data.unknown.data[7],
			rxMsg.data.unknown.dlc
		);	
		FSM_fault_mode(std::string(c_str_reason));
		return;
	}

	// atp assume no errors and message received with valid ID
	switch (rxMsg.id) {
	case CAN::ID::CC_SC_FLOOR_RQ:	process_CAN_CC_msg(rxMsg);		break;
	case CAN::ID::EC_ALL_STATUS:	process_CAN_EC_msg(rxMsg);		break;
	case CAN::ID::F1_RQ:			[[fallthrough]];
	case CAN::ID::F2_RQ:			[[fallthrough]];
	case CAN::ID::F3_RQ:			process_CAN_Fx_msg(rxMsg);		break;
	default:
		std::cerr << "Impossible message ID check on line " << __LINE__ << " in file " << __FILE__ << std::endl;
		break;
	}
}

void process_CAN_CC_msg(CAN::RxFrame rxMsg) {
	is_CC_door_open = rxMsg.data.cc_request.is_door_open;
	
	// only add floor requests in normal operation mode
	if ((state == SC_State::IDLE) || (state == SC_State::MOVING)) {
		Request rq;
		rq.dir = RequestDir::NA;
		rq.floor = rxMsg.data.cc_request.floor_request;
		rq.type = RequestType::CAR;
		scheduler.add_request(rq);
	}
}

void process_CAN_Fx_msg(CAN::RxFrame rxMsg) {
	int floor_num = 0;
	switch (rxMsg.id) {
	case CAN::ID::F1_RQ:	floor_num = 1; 	break;
	case CAN::ID::F2_RQ:	floor_num = 2; 	break;
	case CAN::ID::F3_RQ:	floor_num = 3; 	break;
	default:
		std::cerr << "Impossible message ID check on line " << __LINE__ << " in file " << __FILE__ << std::endl;
		break;
	}

	if (rxMsg.data.fx_request.is_requested) {
		Request rq;
		rq.dir = RequestDir::UP;	// TODO: fix when new CAN format is implemented
		rq.floor = floor_num;
		rq.type = RequestType::FLOOR;
		scheduler.add_request(rq);
	}

}

void process_CAN_EC_msg(CAN::RxFrame rxMsg) {
	CAN::EC_Status stat = rxMsg.data.ec_status;

	is_EC_enabled = stat.is_enabled;

	// current position is always updated
	current_floor = stat.position;
	scheduler.update_car_position(current_floor);

	// received this message -> EC heartbeat
	time_last_EC_heartbeat = current_time();

	// check for unexpected move command
	if (!is_car_moving && stat.is_moving) {
		state = SC_State::FAULT;
		char c_str_reason[1000];
		sprintf("Unauthorized cabin move reported by EC, current floor=%d", c_str_reason, current_floor);	
		FSM_fault_mode(std::string(c_str_reason));
		return;
	}

	// check if EC finished moving the car
	if (is_car_moving && !stat.is_moving) {
		is_car_moving = false;
		scheduler.register_car_stop();
		time_move_finish = current_time();
		
		// check if this floor has been requested by DB
		if (floor_requests_from_DB[current_floor - 1]) {
			// then mark request as complete
			database.complete_elevator_request();	// TODO: add error check here
		}
	}
}

uint64_t current_time() {
    using namespace std::chrono;

    return static_cast<std::uint64_t>(
        duration_cast<milliseconds>(
            steady_clock::now().time_since_epoch()
        ).count()
    );
}
