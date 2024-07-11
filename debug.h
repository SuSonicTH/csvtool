#ifndef DEBUG_INCLUDED

#ifndef DEBUG_ON

#define DEBUG_LINE
#define DEBUG_PRINT

#else

#define DEBUG_LINE(...)                                 \
    printf("DEBUG %s() line %d: ", __func__, __LINE__); \
    printf(__VA_ARGS__);                                \
    fflush(stdout);

#define DEBUG_PRINT(...) \
    printf(__VA_ARGS__); \
    fflush(stdout);

#endif  // DEBUG_ON
#endif  // DEBUG_INCLUDED
