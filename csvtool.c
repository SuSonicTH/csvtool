#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #define UNIT_TEST 1

#define NOT_SET NULL
#define ARG_EQUALS(arg) (arg != NULL && strcmp(argv[i], arg) == 0)
#define IS_ARG(short_arg, long_arg) (ARG_EQUALS(short_arg) || ARG_EQUALS(long_arg))

void print_usage(FILE *fp) {
    fprintf(fp, "CSVTool V0.1\n\n");
    fprintf(fp, "usage csvtool [options] [file 1] .. [file n] \n\n");
    fprintf(fp, "options:\n");
    fprintf(fp, "        -                       stop processing arguments, extras are filenames\n");
    fprintf(fp, "        -d, --delimiter <char>  the delimiter to use (default ,)\n");
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
size_t input_files_count = 0;

char *buffer = NULL;
size_t buffer_size = 0;
char *buffer_pos = NULL;
char *buffer_end = NULL;
char eof = 0;

#define EXIT_IF(expression, ...)                                        \
    if ((expression)) {                                                 \
        fprintf(stderr, "Error in %s() line %d: ", __func__, __LINE__); \
        fprintf(stderr, __VA_ARGS__);                                   \
        fprintf(stderr, "\n");                                          \
        exit(9);                                                        \
    }

void fill_buffer(FILE *fp, char **current, char **line) {
    int line_len = (*current - *line);
    int space_left = buffer_size - line_len;
    if (space_left) {
        memcpy(buffer, *line, line_len);
    }
    if (space_left < minimum_read_size) {  // todo: check alloc error
        buffer_size *= 2;
        buffer = realloc(buffer, buffer_size);
        EXIT_IF(buffer == NULL, "could not allocate memory for buffer");
        space_left = buffer_size - line_len;
    }
    *current = buffer + line_len;
    *line = buffer;
    buffer_end = buffer + line_len + fread(*current, 1, space_left, fp);
}

void read_line_init(FILE *fp) {
    buffer_size = minimum_read_size * 2;
    buffer = (char *)malloc(buffer_size);
    EXIT_IF(buffer == NULL, "could not allocate memory for buffer");
    buffer_pos = buffer;
    int read = fread(buffer, 1, buffer_size, fp);
    buffer_end = buffer + read;
}

char *read_line(FILE *fp) {
    char *line = buffer_pos;
    char *current = buffer_pos;

    if (buffer_pos >= buffer_end && feof(fp)) {
        return NULL;
    }

    while (1) {
        while (*current != '\r' && *current != '\n' && current < buffer_end) {
            current++;
        }
        if (current < buffer_end) {
            buffer_pos = current + 1;
            if (*current == '\r') {
                if (current < buffer_end && *(current + 1) == '\n') {
                    buffer_pos++;
                } else {
                    fill_buffer(fp, &current, &line);
                    if (*(current + 1) == '\n') {
                        buffer_pos++;
                    }
                }
            }
            *current = 0;
            return line;
        } else {
            fill_buffer(fp, &current, &line);

            if (current == buffer_end && feof(fp)) {
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
        fwrite(line, strlen(line), 1, stdout);
        fputc('\n', stdout);
    }
}

#ifndef UNIT_TEST
int main(int argc, char **argv) {
    for (int i = 1; i < argc && input_files == NULL; i++) {
        if (argv[i][0] != '-') {
            input_files = &argv[i];
            input_files_count = argc - i;
        } else if (IS_ARG("-", NOT_SET)) {
            i++;
            if (i < argc) {
                input_files = &argv[i];
                input_files_count = argc - i;
            }
        } else if (IS_ARG("-h", "--help")) {
            print_usage(stdout);
            return 0;
        } else if (IS_ARG("-d", "--delimiter")) {
            delimiter = get_arg_value("delimiter", ++i, argc, argv)[0];
        } else if (IS_ARG("-c", "--use_stdin")) {
            use_stdin = 1;
        } else {
            print_usage(stderr);
            fprintf(stderr, "Error: unknown argument '%s'\n", argv[i]);
        }
    }

    if (argc == 1 || (input_files == NULL && use_stdin == 0)) {
        print_usage(stderr);
        fprintf(stderr, "Error: either -c/--use_stdin or a filename has to be given as argument\n");
        return (1);
    }

    if (use_stdin) {
        freopen(NULL, "rb", stdin);
        process(stdin);
    } else {
        for (int i = 0; i < input_files_count; i++) {
            FILE *fp = fopen(input_files[i], "rb");
            EXIT_IF(fp == NULL, "could not open file '%s' for reading", input_files[i]);
            if (i == 0) {
                read_line_init(fp);
            }
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

int _is_NULL(void *ptr) {
    if (ptr != NULL) {
        printf("\nexpected NULL got \"%p\"\n", ptr);
        return 0;
    }
    return 1;
}

int _is_not_NULL(void *ptr) {
    if (ptr == NULL) {
        printf("\nexpected not NULL\n");
        return 0;
    }
    return 1;
}

void test_is_arg() {
    char *argv[] = {"-d", "--delimiter", "--non-matching"};

    int i = 0;
    _assert(IS_ARG("-d", "--delimiter"));
    _assert(IS_ARG("-d", NOT_SET));

    i = 1;
    _assert(IS_ARG("-d", "--delimiter"));
    _assert(IS_ARG(NOT_SET, "--delimiter"));

    i = 2;
    _assert_not(IS_ARG("-d", "--delimiter"));
}

void _write_lines_to_file(char *file_name, char *eol, char **lines) {
    size_t eol_size = strlen(eol);
    FILE *fp = fopen(file_name, "wb");
    assert(fp != NULL);
    while (*lines != NULL) {
        fwrite(*lines, strlen(*lines), 1, fp);
        fwrite(eol, eol_size, 1, fp);
        lines++;
    }
    fclose(fp);
}

void test_readLine() {
    char *TEST_FILE_NAME = "./test/lineRederTest.txt";
    char *test_lines[] = {"Test", "Test2", "Test3", "much longer line that causes the buffer to grow multiple times", "Test4", NULL};
    _write_lines_to_file(TEST_FILE_NAME, "\n", test_lines);

    FILE *fp = fopen(TEST_FILE_NAME, "rb");
    assert(fp != NULL);

    minimum_read_size = 2;  // tet extra low to trigger multiple buffer size doublings
    read_line_init(fp);

    char **line = test_lines;
    while (*line != NULL) {
        _assert(_str_equals(*line, read_line(fp)));
        line++;
    }
    _assert(_is_NULL(read_line(fp)));
    fclose(fp);
}

int main(int argc, char **argv) {
    _verify(test_is_arg);
    _verify(test_readLine);

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