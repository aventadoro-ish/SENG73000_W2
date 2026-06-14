#define DEBUG 1  // Set to 1 to enable debug prints, 0 to disable completely

#if DEBUG
    #define DEBUG_BEGIN(speed)   Serial.begin(speed)
    #define DEBUG_PRINT(...)     Serial.print(__VA_ARGS__)
    #define DEBUG_PRINTLN(...)   Serial.println(__VA_ARGS__)
    #define DEBUG_TRACE(msg) \
        Serial.print(F("TRACE [")); \
        Serial.print(F(__FILE__)); \
        Serial.print(F(" -> Line ")); \
        Serial.print(__LINE__); \
        Serial.print(F("]: ")); \
        Serial.println(msg)
#else
    #define DEBUG_BEGIN(speed)
    #define DEBUG_PRINT(...)
    #define DEBUG_PRINTLN(...)
    #define DEBUG_TRACE(msg)
#endif
