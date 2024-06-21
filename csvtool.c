#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NOT_SET NULL
#define ARG_EQUALS(arg) (arg != NULL && strcmp(argv[i], arg) == 0)
#define IS_ARG(short_arg, long_arg) (ARG_EQUALS(short_arg) || ARG_EQUALS(long_arg))

void print_usage(FILE *fp) {
    fprintf(fp, "CSVTool V0.1\n\n");
    fprintf(fp, "usage csvtool [options] [file 1] .. [file n] \n\n");
    fprintf(fp, "options:\n");
    fprintf(fp, "        -d, --delimiter <character> the delimiter to use (default ,)\n");
    fprintf(fp, "\n");
}

char *get_arg_value(char *name, int arg, int argc, char **argv) {
    if (arg >= argc) {
        print_usage(stderr);
        fprintf(stderr, "Error: value missing for argument '%s'\n", name);
        exit(1);
    }
    return argv[arg];
}

char delimiter = ',';
char use_stdin = 0;

char **input_files = NULL;
size_t input_files_size = 16;
size_t input_files_count = 0;

// todo: check alloc error
void add_file(char *file_name) {
    if (input_files == NULL) {
        input_files = malloc(sizeof(char *) * input_files_size);
    } else if (input_files_count >= input_files_size) {
        input_files_size *= 2;
        input_files = realloc(input_files, sizeof(char *) * input_files_size);
    }
    input_files[input_files_count++] = file_name;
}

#ifndef UNIT_TEST
int main(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] != '-') {
            add_file(argv[i]);
        } else if (IS_ARG("-h", "--help")) {
            print_usage(stdout);
            return 0;
        } else if (IS_ARG("-d", "--delimiter")) {
            delimiter = get_arg_value("delimiter", ++i, argc, argv)[0];
        } else if (IS_ARG("-c", "--use_stdin")) {
            use_stdin = 1;
        }
    }

    if (argc == 1 || (input_files_count == 0 && use_stdin == 0)) {
        print_usage(stderr);
        fprintf(stderr, "Error: either -c/--use_stdin or a filename has to be given as argument\n");
        return (1);
    }

    printf("%d\n", input_files_count);
    fflush(stdout);
    for (int i = 0; i < input_files_count; i++) {
        printf("%d: '%s'\n", i, input_files[i]);
        fflush(stdout);
    }
    return 0;
}
#endif

/**************\
|  UNIT Tests  |
\**************/

#ifdef UNIT_TEST
int tests_run = 0;
int tests_failed = 0;
int assertions = 0;
int assertions_failed = 0;
int failures = 0;

#define FAIL()           \
    assertions_failed++; \
    printf("failure in %s() line %d\n", __func__, __LINE__)

#define _assert(test)  \
    do {               \
        assertions++;  \
        if (!(test)) { \
            FAIL();    \
        }              \
    } while (0)

#define _assert_not(test) \
    do {                  \
        assertions++;     \
        if ((test)) {     \
            FAIL();       \
        }                 \
    } while (0)

#define _verify(test)                       \
    do {                                    \
        failures = assertions_failed;       \
        printf("running " #test ": ");      \
        test();                             \
        tests_run++;                        \
        if (assertions_failed > failures) { \
            printf("\n" #test " FAILED\n"); \
            tests_failed++;                 \
        } else {                            \
            printf("PASSED\n");             \
        }                                   \
    } while (0)

int _str_equals(char *expected, char *actual) {
    if (actual == NULL) {
        actual = "%NULL%";
    }
    if (strcmp(expected, actual) != 0) {
        printf("\nexpected \"%s\" got \"%s\"\n", expected, actual);
        return 0;
    }
    return 1;
}

int _number_equals(size_t expected, size_t actual) {
    if (expected != actual) {
        printf("\nexpected %d got %d\n", expected, actual);
        return 0;
    }
    return 1;
}

void test_file_list_empty() {
    _assert(input_files == NULL);
    _assert(input_files_count == 0);
}

void test_file_list_add() {
    input_files_size = 2;

    add_file("File 1");
    _assert(_number_equals(1, input_files_count));
    _assert(_number_equals(2, input_files_size));
    _assert(_str_equals("File 1", input_files[0]));

    add_file("File 2");
    _assert(_number_equals(2, input_files_count));
    _assert(_number_equals(2, input_files_size));
    _assert(_str_equals("File 2", input_files[1]));

    add_file("File 3");
    _assert(_number_equals(3, input_files_count));
    _assert(_number_equals(4, input_files_size));
    _assert(_str_equals("File 3", input_files[2]));
}

void test_is_arg() {
    char *argv[] = {
        "-d",
        "--delimiter",
        "--non-matching"};

    int i = 0;
    _assert(IS_ARG("-d", "--delimiter"));
    _assert(IS_ARG("-d", NOT_SET));

    i = 1;
    _assert(IS_ARG("-d", "--delimiter"));
    _assert(IS_ARG(NOT_SET, "--delimiter"));

    i = 3;
    _assert_not(IS_ARG("-d", "--delimiter"));
}

int main(int argc, char **argv) {
    _verify(test_file_list_empty);
    _verify(test_file_list_add);
    _verify(test_is_arg);

    printf("\n");
    printf(" Tests run %d\n", tests_run);
    printf("    failed %d\n", tests_failed);
    printf("\n");
    printf("assertions %d\n", assertions);
    printf("    failed %d\n", assertions_failed);

    if (tests_failed != 0) {
        printf("\nFAILED\n");
        return 1;
    }

    printf("\nPASSED\n");
    return 0;
}
#endif  // UNIT_TEST