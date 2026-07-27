#pragma once

// -----------------------------------------------------------------------------
// Settings
// -----------------------------------------------------------------------------

// #define DO_USE_DB
#define DO_USE_CAN

constexpr unsigned int NUM_FLOORS = 3;

/**
 * @brief How long to wait between moving to the next floor in sabbath mode 
 * (in milliseconds)
 */
constexpr unsigned long int SABBATH_MOVE_DELAY_MS = 5000;

/**
 * @brief How long to wait on a floor if other floor requests are queued in 
 * normal mode (in milliseconds)
 */
constexpr unsigned long int FLOOR_WAIT_DELAY_MS = 3000;

/**
 * @brief What is the maximum expected time to finish a cabin move before 
 * elevator goes into fault mode
 */
constexpr unsigned long int MOVE_FINISH_TIMEOUT_MS = 45000;

/**
 * @brief Period of EC heartbeats in milliseconds
 */
constexpr unsigned long int EC_HEARTBEAT_PERIOD_MS = 500;

/**
 * @brief Used to find max amount of time without receiving EC heartbeat before 
 * elevator goes into fault mode.
 * 
 * MAX_PERIOD = EC_HEARTBEAT_PERIOD_MS * EC_MAX_HEARTBEAT_PERIOD_MULTIPLIER
 */
constexpr int EC_MAX_HEARTBEAT_PERIOD_MULTIPLIER = 3;

/**
 * @brief Where to send the cabin during initialization
 */
constexpr unsigned int INITIAL_FLOOR = 1;
