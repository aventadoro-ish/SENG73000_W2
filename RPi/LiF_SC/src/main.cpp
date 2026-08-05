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
bool is_EC_enabled = false;

uint64_t time_last_EC_heartbeat;
uint64_t time_move_start;
uint64_t time_move_finish;		// used to wait on a floor for a bit before 
								// moving to the next target
								// TODO: change when servo doors are added

// keep track of floors that have been requested through DB
bool floor_requests_from_DB[NUM_FLOORS];


bool is_CC_door_open = false;  // Effective door state used by the FSM

bool db_door_override_active = false;

bool door_db_monitor_initialized = false;
bool last_DB_door_open = false;

bool can_door_state_initialized = false;
bool last_CAN_door_open = false;




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


void initialize_door_monitor();

void process_DB_door_command();

void update_door_state_from_CAN(bool can_door_open);

void announceFloor(int floor);


// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------
int main() {
	scheduler = Scheduler();


	for (int i = 0; i < (int) NUM_FLOORS; i++) {
		floor_requests_from_DB[i] = 0;
	}


	if (!database.db_connect()) {
		return -1;
	}
	
	// Initializes the request reader and ignores existing requests.
	database.read_floor_request();

	// Remember the initial database door state without treating it as
	// a new command.
	initialize_door_monitor();


	
	printf("\nLithium Firefly Project. Initializing SC FSM\nProgram settings\n");
	printf(" - number of floors: %u\n", NUM_FLOORS);
	printf(" - queued floor wait time (ms): %lu\n", FLOOR_WAIT_DELAY_MS);
	printf(" - sabbath mode wait time (ms): %lu\n", SABBATH_MOVE_DELAY_MS);
	printf(" - initial floor: %u\n", INITIAL_FLOOR);

	while(is_running) { // FSM infinite loop start
		process_DB_door_command();
		process_CAN_msg();
		
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
			sprintf(c_str_reason,
				"Move timed out in NORMAL mode after %lums. Max time is %lu", 
				current_time() - time_move_start, MOVE_FINISH_TIMEOUT_MS
			);	
			FSM_fault_mode(std::string(c_str_reason));
			return;
		}

		// move finished
		if (!is_car_moving) {
			time_move_finish = current_time();
			state = SC_State::IDLE;
			announceFloor(current_floor);
		}
	}

	
	// handle remote requests
	int db_request = database.read_floor_request();
	if (db_request > 0 &&
		db_request <= static_cast<int>(NUM_FLOORS)) {

		std::cout << "DB - Web Request for floor " << db_request << " ";

		if (current_floor == db_request) {
			std::cout << "\t->already on requested floor. Mark Completed" << std::endl;
			database.complete_elevator_request(db_request);
		
		} else {
			std::cout << "\t->Adding DB Web request to scheduler" << std::endl;

			Request rq;
			rq.dir = RequestDir::NA;
			rq.floor = db_request;
			rq.type = RequestType::CAR;

			scheduler.add_request(rq);
		}

	} else if (db_request < 0) {
		std::cerr << "Failed to read DB floor request" << std::endl;
	}
	
	// check for new target floor
	int new_target = scheduler.get_target_floor();
	if (new_target != current_target) {
		// current assumption: extra target verification is not needed
		//	because scheduler handles it

		// only move after spending some time on the floor
		if ((current_time() - time_move_finish > FLOOR_WAIT_DELAY_MS) && state == SC_State::IDLE) {
			if (!is_CC_door_open) {
				can_link.ec_go_to_floor(new_target);

				current_target = new_target;
				is_car_moving = true;
				time_move_start = current_time();
				state = SC_State::MOVING;
			} 
		} else if (state == SC_State::MOVING) {
			// no check for door open, as it was closed when the move started
			can_link.ec_go_to_floor(new_target);
			current_target = new_target;
			is_car_moving = true;
			time_move_start = current_time();
		}
	}


	DB::OperationMode new_db_op_mode = database.get_operation_mode();
	switch (new_db_op_mode) {
	case DB::OperationMode::NORMAL:
		// do nothing
		break;
	case DB::OperationMode::SABBATH:
		std::cout << "Mode transition: Normal -> Sabbath" << std::endl;
		state = is_car_moving ? SC_State::SABBATH_MOVING : SC_State::SABBATH_IDLE;
		break;
	case DB::OperationMode::MAINTENANCE:
		std::cout << "Mode transition: Normal -> Maintenance" << std::endl;
		state = is_car_moving ? SC_State::MAINTENANCE_MOVING : SC_State::MAINTENANCE;
		break;
	case DB::OperationMode::FAULT:
		std::cout << "Mode transition: Normal -> Fault" << std::endl;
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
			sprintf(c_str_reason, 
				"Move timed out in SABBATH mode after %lums. Max time is %lu", 
				current_time() - time_move_start, MOVE_FINISH_TIMEOUT_MS
			);	
			FSM_fault_mode(std::string(c_str_reason));
			return;
		}

		// move finished
		if (!is_car_moving) {
			time_move_finish = current_time();
			state = SC_State::SABBATH_IDLE;
			announceFloor(current_floor);
		}
	}
	


	// only move after spending some time on the floor
	if (current_time() - time_move_finish > SABBATH_MOVE_DELAY_MS && 
			state == SC_State::SABBATH_IDLE) {
		if (is_going_up) {
			if (current_floor < static_cast<int>(NUM_FLOORS)) {
				current_target = current_floor + 1;
			} else {
				is_going_up = false;
				current_target = NUM_FLOORS - 1;
			}
		} else {
			if (current_floor > 1) {
				current_target = current_floor - 1;
			} else {
				is_going_up = true;
				current_target = 2;
			}
		}

		if (!is_CC_door_open) {
			is_car_moving = true;
			can_link.ec_go_to_floor(current_target);
			std::cout << "Sabbath mode move to floor " << current_target << std::endl;
			time_move_start = current_time();
			state = SC_State::SABBATH_MOVING;
		}
	}



	DB::OperationMode new_db_op_mode = database.get_operation_mode();
	switch (new_db_op_mode) {
	case DB::OperationMode::NORMAL:
		std::cout << "Mode transition: Sabbath -> Normal" << std::endl;
		state = is_car_moving ? SC_State::MOVING : SC_State::IDLE;
		break;
	case DB::OperationMode::SABBATH:
		// do nothing
		break;
	case DB::OperationMode::MAINTENANCE:
		std::cout << "Mode transition: Sabbath -> Maintenance" << std::endl;
		state = is_car_moving ? SC_State::MAINTENANCE_MOVING : SC_State::MAINTENANCE;
		break;
	case DB::OperationMode::FAULT:
		std::cout << "Mode transition: Sabbath -> Fault" << std::endl;
		state = SC_State::FAULT;
	default:
		break;
	}


}

void FSM_maintenance_mode() {
    /*
     * Handle completion of a maintenance movement.
     *
     * process_CAN_EC_msg() sets is_car_moving to false when the
     * EC reports that movement has finished.
     */
    if (state == SC_State::MAINTENANCE_MOVING) {
        if (current_time() - time_move_start >
            MOVE_FINISH_TIMEOUT_MS) {

            state = SC_State::FAULT;

            char c_str_reason[1000];
            sprintf(
                c_str_reason,
                "Move timed out in MAINTENANCE mode after "
                "%lums. Max time is %lu",
                current_time() - time_move_start,
                MOVE_FINISH_TIMEOUT_MS
            );

            FSM_fault_mode(std::string(c_str_reason));
            return;
        }

        if (!is_car_moving) {
            time_move_finish = current_time();
            state = SC_State::MAINTENANCE;

            announceFloor(current_floor);

            /*
             * Complete the request only if this floor was actually
             * requested through the database.
             */
            if (floor_requests_from_DB[current_floor - 1]) {
                if (database.complete_elevator_request(
                        current_floor)) {

                    floor_requests_from_DB[
                        current_floor - 1
                    ] = false;
                }
            }
        }
    }

    /*
     * Read the next maintenance request only while stationary.
     *
     * Waiting for closed doors before reading prevents us from
     * consuming a request that cannot yet be dispatched.
     */
    if (state == SC_State::MAINTENANCE &&
        !is_car_moving &&
        !is_CC_door_open) {

        int db_request = database.read_floor_request();

        if (db_request > 0 &&
            db_request <= static_cast<int>(NUM_FLOORS)) {

            std::cout
                << "DB maintenance request to floor "
                << db_request << " ";

            if (current_floor == db_request) {
                std::cout
                    << "already completed"
                    << std::endl;

                if (database.complete_elevator_request(
                        db_request)) {

                    floor_requests_from_DB[
                        db_request - 1
                    ] = false;
                }
            } else {
                std::cout
                    << "starting movement"
                    << std::endl;

                floor_requests_from_DB[
                    db_request - 1
                ] = true;

                current_target = db_request;
                is_car_moving = true;
                time_move_start = current_time();
                state = SC_State::MAINTENANCE_MOVING;

                can_link.ec_go_to_floor(db_request);
            }
        } else if (db_request < 0) {
            std::cerr
                << "Failed to read maintenance DB request"
                << std::endl;
        }
    }

    DB::OperationMode new_db_op_mode =
        database.get_operation_mode();

    switch (new_db_op_mode) {
    case DB::OperationMode::NORMAL:
        std::cout
            << "Mode transition: Maintenance -> Normal"
            << std::endl;

        state = is_car_moving
            ? SC_State::MOVING
            : SC_State::IDLE;
        break;

    case DB::OperationMode::SABBATH:
        std::cout
            << "Mode transition: Maintenance -> Sabbath"
            << std::endl;

        state = is_car_moving
            ? SC_State::SABBATH_MOVING
            : SC_State::SABBATH_IDLE;
        break;

    case DB::OperationMode::MAINTENANCE:
        break;
	
	case DB::OperationMode::UNKNOWN: [[fallthrough]];
    case DB::OperationMode::FAULT:
        std::cout
            << "Mode transition: Maintenance -> Fault"
            << std::endl;

        state = SC_State::FAULT;
        break;
    }
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
		sprintf(c_str_reason, "CAN receive failed with status 0x%x", can_status);	
		FSM_fault_mode(std::string(c_str_reason));
		return;
	}
	
	int data_arr[8];
	for (int i = 0; i < rawRxMsg.LEN; i++) {
		data_arr[i] = rawRxMsg.DATA[i];
	}
	database.log_can_message(rawRxMsg.ID, data_arr, rawRxMsg.LEN);
	
	std::cout << "Rec CAN frame " << std::hex << rawRxMsg.ID << " data: " << static_cast<int>(rawRxMsg.DATA[0]) << std::dec << std::endl;
	
	if (rxMsg.id == CAN::ID::UNKNOWN) {
		state = SC_State::FAULT;
		char c_str_reason[1000];
		sprintf(c_str_reason, 
			"CAN received a message with unknown ID=0x%x, data=0x%d %d %d %d %d %d %d %d, dlc=%d",
			rxMsg.data.unknown.id,
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
    update_door_state_from_CAN(
        rxMsg.data.cc_request.is_door_open
    );

	// Filter state to only be normal operations mode
	//	Ignore floor requests in maintenance and sabbath mode
    if ((state == SC_State::IDLE) ||
        (state == SC_State::MOVING)) {

        if (rxMsg.data.cc_request.floor_request != 0) {
			// Add request to scheduler
            Request rq;
            rq.dir = RequestDir::NA;
            rq.floor = rxMsg.data.cc_request.floor_request;
            rq.type = RequestType::CAR;
            scheduler.add_request(rq);

			// Log request to database
			database.log_elevator_request(rq.floor, (int)rxMsg.id);
        }
    } else {
		std::cout << "Ignored Car Request from floor " << rxMsg.data.cc_request.floor_request << " in non-NORMAL mode of operation" << std::endl;
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
		// Filter state to only be normal operations mode
		//	Ignore floor requests in maintenance and sabbath mode
		if ((state == SC_State::IDLE) ||
	        (state == SC_State::MOVING)) {
			
			// Add request to scheduler
			// TODO: update with request direction
			Request rq;
			rq.dir = RequestDir::UP;	// TODO: fix when new CAN format is implemented
			rq.floor = floor_num;
			rq.type = RequestType::FLOOR;
			scheduler.add_request(rq);

			// Log request to database
			database.log_elevator_request(floor_num, (int)rxMsg.id);
		} else {
			std::cout << "Ignored Floor Request from floor " << floor_num << " in non-NORMAL mode of operation" << std::endl;
		}
	}

}

void process_CAN_EC_msg(CAN::RxFrame rxMsg) {
	CAN::EC_Status stat = rxMsg.data.ec_status;

	is_EC_enabled = stat.is_enabled;

	if (stat.position != current_floor) {
		std::cout << "EC message updating floor to " << (int) stat.position << std::endl; 
	}
	// current position is always updated
	current_floor = stat.position;
	scheduler.update_car_position(current_floor);

	// received this message -> EC heartbeat
	time_last_EC_heartbeat = current_time();

	// check for unexpected move command
	if (!is_car_moving && stat.is_moving) {
		state = SC_State::FAULT;
		char c_str_reason[1000];
		sprintf(c_str_reason, "Unauthorized cabin move reported by EC, current floor=%d", current_floor);	
		FSM_fault_mode(std::string(c_str_reason));
		return;
	}

	// check if EC finished moving the car
	if (is_car_moving && !stat.is_moving) {
		is_car_moving = false;

		// clear database requests if necessary
		if (database.complete_elevator_request(current_floor)) {
			std::cout << "Completed DB request to floor " << current_floor << std::endl;
		}

		if (state == SC_State::MOVING) {
			scheduler.register_car_stop();
		}

		time_move_finish = current_time();
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



void initialize_door_monitor()
{
    int db_door_state = database.get_doors_open();

    if (db_door_state < 0) {
        std::cerr << "Failed to initialize DB door-state monitor"
                  << std::endl;
        return;
    }

    last_DB_door_open = (db_door_state != 0);
    door_db_monitor_initialized = true;
}

void process_DB_door_command() {
    int db_door_state = database.get_doors_open();

    if (db_door_state < 0) {
        // Preserve the existing effective state if the database
        // cannot be read.
        return;
    }

    bool new_DB_door_open = (db_door_state != 0);

    if (!door_db_monitor_initialized) {
        last_DB_door_open = new_DB_door_open;
        door_db_monitor_initialized = true;
        return;
    }

    // A changed DB value is interpreted as a new remote command.
    if (new_DB_door_open != last_DB_door_open) {
        last_DB_door_open = new_DB_door_open;

        is_CC_door_open = new_DB_door_open;
        db_door_override_active = true;

        std::cout << "Door state commanded by DB: "
                  << (is_CC_door_open ? "OPEN" : "CLOSED")
                  << std::endl;
    }
}

void update_door_state_from_CAN(bool can_door_open) {
    if (!can_door_state_initialized) {
        last_CAN_door_open = can_door_open;
        can_door_state_initialized = true;

        /*
         * Do not allow the first CAN status message to cancel a DB
         * command that may have arrived before the first CAN message.
         */
        if (db_door_override_active) {
            return;
        }

        is_CC_door_open = can_door_open;

        if (!door_db_monitor_initialized ||
            last_DB_door_open != can_door_open) {
            database.set_doors_open(can_door_open);
            last_DB_door_open = can_door_open;
            door_db_monitor_initialized = true;
        }

        return;
    }

    /*
     * Only an actual transition in the CAN-reported door state
     * releases the database override. Repeated CAN heartbeat/status
     * messages containing the same state are ignored.
     */
    if (can_door_open != last_CAN_door_open) {
        last_CAN_door_open = can_door_open;
        db_door_override_active = false;
        is_CC_door_open = can_door_open;

        if (!door_db_monitor_initialized ||
            last_DB_door_open != can_door_open) {
            database.set_doors_open(can_door_open);
            last_DB_door_open = can_door_open;
            door_db_monitor_initialized = true;
        }

        std::cout << "Door state toggled by CAN: "
                  << (is_CC_door_open ? "OPEN" : "CLOSED")
                  << std::endl;
    } else if (!db_door_override_active) {
        // Normal operation when no DB override is active.
        is_CC_door_open = can_door_open;
    }
}


void announceFloor(int floor) {
    std::string cmd =
        std::string("/usr/bin/aplay ") +
        AUDIO_PATH +
        "floor" +
        std::to_string(floor) +
        ".wav &";
    system(cmd.c_str());

}

