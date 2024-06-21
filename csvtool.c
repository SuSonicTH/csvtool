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
size_t minimum_read_size = 4096;

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

char *buffer = NULL;
size_t buffer_size = 0;
char *buffer_pos = NULL;
char *buffer_end = NULL;
char eof = 0;

char *fill_buffer(FILE *fp) {
    int pos = buffer_pos - buffer;
    int space_left = buffer_size - pos;
    memcpy(buffer, buffer_pos, buffer_end - buffer_pos);
    buffer_end = buffer + pos + fread(buffer, buffer_size, 1, fp);
    if (space_left < minimum_read_size) {  // todo: check alloc error
        buffer_size *= 2;
        buffer = realloc(buffer, buffer_size);
        buffer_pos = buffer;
        space_left = buffer_size - pos;
    }
    char *line = buffer + pos;
    buffer_end = buffer + pos + fread(line, 1, space_left, fp);
    return line;
}

char *read_line(FILE *fp) {
    if (buffer == NULL) {
        buffer_size = minimum_read_size * 2;
        buffer = malloc(sizeof(buffer_size));
        buffer_pos = buffer;
        buffer_end = buffer + fread(buffer, 1, buffer_size, fp);
    }

    char *line = buffer_pos;
    char *current = buffer_pos;

    if (buffer_pos >= buffer_end && feof(fp)) {
        printf("EOF");
        fflush(stdout);
        return NULL;
    }

    while (1) {
        while (*current != '\r' && *current != '\n' && current <= buffer_end) {
            current++;
        }
        if (current <= buffer_end) {
            *current = 0;
            buffer_pos = current + 1;
            if (*current == '\r' && current < buffer_end && *(current + 1) == '\n') {
                buffer_pos++;
            }
            return line;
        } else {
            current = fill_buffer(fp);
            line = buffer;

            if (buffer_pos == buffer_end && feof(fp)) {
                *current = 0;
                return line;
            }
        }
    }
}

void process(FILE *fp) {
    char *line;
    int i = 0;
    while ((line = read_line(fp)) != NULL) {
        printf("%d: %s\n", i++, line);
        fflush(stdout);
    }
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

    if (use_stdin) {
        freopen(NULL, "rb", stdin);
        process(stdin);
    } else {
        for (int i = 0; i < input_files_count; i++) {
            // todo: check file open error
            FILE *fp = fopen(input_files[i], "rb");
            process(fp);
            fclose(fp);
        }
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
    char *argv[] = {"-d", "--delimiter", "--non-matching"};

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