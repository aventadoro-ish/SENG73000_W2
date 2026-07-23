#pragma once

// -----------------------------------------------------------------------------
// Settings
// -----------------------------------------------------------------------------

// #define DO_USE_DB
// #define DO_USE_CAN

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
 * @brief Where to send the cabin during initialization
 */
constexpr unsigned int INITIAL_FLOOR = 1;
