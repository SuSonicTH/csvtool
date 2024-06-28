#ifdef UNIT_TEST
#ifndef UNIT_TEST_INCLUDED
#define UNIT_TEST_INCLUDED
#include <stdio.h>

int ut_tests_run = 0;
int ut_tests_failed = 0;

int ut_assertions = 0;
int ut_assertions_failed = 0;

int ut_failures = 0;

#define UT_MESSAGE_SIZE 1024
char ut_message_buffer[UT_MESSAGE_SIZE] = "";

#ifndef UT_NO_COLOR
#define UT_ANSI_RED
#define UT_ANSI_GREEN
#define UT_ANSI_END
#else UT_NO_COLOR
#define UT_ANSI_RED "\e[0;31m"
#define UT_ANSI_GREEN "\e[0;32m"
#define UT_ANSI_END "\e[0m"
#endif UT_NO_COLOR

#define ut_fail()                                                                         \
    if (ut_assertions_failed == ut_failures) printf(UT_ANSI_RED " FAILED\n" UT_ANSI_END); \
    ut_assertions_failed++;                                                               \
    printf("   failure in %s() line %d", __func__, __LINE__);                             \
    if (ut_message_buffer[0] != 0) printf(": %s", ut_message_buffer);                     \
    printf("\n");

#define ut_assert(test)           \
    do {                          \
        ut_message_buffer[0] = 0; \
        ut_assertions++;          \
        if (!(test)) {            \
            ut_fail();            \
        }                         \
    } while (0)

#define ut_assert_not(test)       \
    do {                          \
        ut_message_buffer[0] = 0; \
        ut_assertions++;          \
        if ((test)) {             \
            ut_fail();            \
        }                         \
    } while (0)

#define ut_run(test)                                           \
    do {                                                       \
        ut_failures = ut_assertions_failed;                    \
        printf("running " #test);                              \
        test();                                                \
        ut_tests_run++;                                        \
        if (ut_assertions_failed > ut_failures) {              \
            printf(UT_ANSI_RED #test " FAILED\n" UT_ANSI_END); \
            ut_tests_failed++;                                 \
        } else {                                               \
            printf(UT_ANSI_GREEN " PASSED\n" UT_ANSI_END);     \
        }                                                      \
    } while (0)

#define ut_message(...) snprintf(ut_message_buffer, UT_MESSAGE_SIZE, __VA_ARGS__)

int ut_str_equals(char *expected, char *actual) {
    if (actual == NULL) {
        actual = "%NULL%";
    }
    if (strcmp(expected, actual) != 0) {
        ut_message("expected \"%s\" got \"%s\"", expected, actual);
        return 0;
    }
    return 1;
}

int ut_number_equals(size_t expected, size_t actual) {
    if (expected != actual) {
        ut_message("expected %d got %d", expected, actual);
        return 0;
    }
    return 1;
}

int ut_is_NULL(void *ptr) {
    if (ptr != NULL) {
        ut_message("expected NULL got \"%p\"", ptr);
        return 0;
    }
    return 1;
}

int ut_is_not_NULL(void *ptr) {
    if (ptr == NULL) {
        ut_message("expected not NULL");
        return 0;
    }
    return 1;
}

int ut_end() {
    printf("\n");
    printf(" Tests run %d\n", ut_tests_run);
    printf("    failed %d\n", ut_tests_failed);
    printf("\n");
    printf("assertions %d\n", ut_assertions);
    printf("    failed %d\n", ut_assertions_failed);

    if (ut_tests_failed != 0) {
        printf(UT_ANSI_RED "\nFAILED\n" UT_ANSI_END);
        return 1;
    }

    printf(UT_ANSI_GREEN "\nPASSED\n" UT_ANSI_END);
    return 0;
}

#endif UNIT_TEST_INCLUDED
#endif UNIT_TEST
