#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef UNIT_TEST
#define UNIT_TEST
#endif

#define DEBUG_ON

#ifndef DEBUG_ON
#define DEBUG
#else
#define DEBUG(...)       \
    printf(__VA_ARGS__); \
    fflush(stdout);
#endif

typedef struct {
    char *file_name;
    FILE *file;
    uint8_t *buffer;
    size_t size;
    size_t read_size;
    size_t start;
    size_t end;
    uint8_t eof;
    size_t *fields;
    size_t fields_size;
    size_t fields_count;
} csv_line_s;

#define DEFAULT_READ_SIZE 1024
#define DEFAULT_FIELD_SIZE 128

csv_line_s *csv_line_init(csv_line_s *csv, size_t read_size, size_t fields_size) {
    if (csv == NULL) {
        return NULL;
    }
    memset(csv, 0, sizeof(csv_line_s));

    csv->read_size = read_size == 0 ? DEFAULT_READ_SIZE : read_size;
    csv->size = csv->read_size;
    if ((csv->buffer = malloc(csv->size)) == NULL) {
        return NULL;
    }

    csv->fields_size = fields_size == 0 ? DEFAULT_FIELD_SIZE : fields_size;
    if ((csv->fields = malloc(csv->fields_size * sizeof(size_t))) == NULL) {
        free(csv->buffer);
        return NULL;
    }
    return csv;
}

void csv_line_free(csv_line_s *csv) {
    if (csv != NULL) {
        if (csv->buffer != NULL) {
            free(csv->buffer);
            csv->buffer = NULL;
        }
        if (csv->fields != NULL) {
            free(csv->fields);
            csv->fields = NULL;
        }
    }
}

size_t csv_line_fill_buffer(csv_line_s *csv) {
    if (csv->eof) {
        return 0;
    }

    size_t space = csv->size - csv->end;
    if (space < csv->read_size) {
        if (csv->start > 0) {
            size_t len = csv->end - csv->start;
            memcpy(csv->buffer, &csv->buffer[csv->start], len);
            csv->start = 0;
            csv->end = len;
            space = csv->size - csv->end;
        }
        if (space < csv->read_size) {
            csv->size += csv->read_size;
            csv->buffer = realloc(csv->buffer, csv->size);  // todo: check alloc
        }
    }
    size_t read = fread(&csv->buffer[csv->end], 1, csv->read_size, csv->file);
    csv->eof = feof(csv->file);
    csv->end += read;
    // DEBUG("\t\tread: %d\n", read);
    return read;
}

void csv_line_open_file(csv_line_s *csv, char *file_name) {
    csv->file_name = file_name;
    csv->file = fopen(file_name, "rb");  // todo check file open
    csv->start = 0;
    csv->end = 0;
    csv->eof = 0;
    csv_line_fill_buffer(csv);
}

void csv_line_close_file(csv_line_s *csv) {
    if (csv->file != NULL) {
        fclose(csv->file);
    }
}

#define CURRENT csv->buffer[pos]

void csv_line_read_line(csv_line_s *csv) {
    size_t pos = csv->start;
    csv->fields_count = 0;

    // DEBUG("\n");
    while (1) {
        if (CURRENT == '"') {
            csv->fields[csv->fields_count++] = pos;
            while (pos < csv->end && CURRENT != '"') {
                pos++;
                if (pos > csv->end) {
                    csv_line_fill_buffer(csv);
                }
            }
            if (CURRENT != '"') {
                fprintf(stderr, "no matching \"");
                exit(9);
            }
            CURRENT = 0;
            while (pos < csv->end && CURRENT != ',' && CURRENT != '\r' && CURRENT != '\n') {
                if (pos + 1 == csv->end) {
                    csv_line_fill_buffer(csv);
                } else {
                    pos++;
                }
            }
            if (CURRENT == '\r') {
                CURRENT = 0;
                pos++;
                if (CURRENT == '\n') {
                    pos++;
                }
            } else {
                CURRENT = 0;
                pos++;
            }
        } else {
            csv->fields[csv->fields_count++] = pos;
            // DEBUG("field %d: pos: %d: %s\n", csv->fields_count, pos, &CURRENT);
            while (pos < csv->end && CURRENT != ',' && CURRENT != '\r' && CURRENT != '\n') {
                // DEBUG("\tpos: %d < end: %d, char: %c, pos + 1 == csv->end: %d\n", pos, csv->end, CURRENT, csv->end - pos - 1);
                if (pos + 1 == csv->end) {
                    // DEBUG("\tfillBuffer\n");
                    if (!csv_line_fill_buffer(csv)) {
                        // DEBUG("\tbreak\n");
                        pos++;
                        break;
                    }
                } else {
                    pos++;
                }
            }
            if (CURRENT == ',') {
                CURRENT = 0;
                pos++;
            } else if (CURRENT == '\r') {
                CURRENT = 0;
                pos++;
                if (pos + 1 == csv->end) {
                    csv_line_fill_buffer(csv);
                }
                if (CURRENT == '\n') {
                    pos++;
                }
                csv->start = pos;
                return;
            } else {
                CURRENT = 0;
                pos++;
                csv->start = pos;
                return;
            }
        }
    }
}

#ifdef UNIT_TEST
#include "unit_test.h"

void test_init_free() {
    csv_line_s csv;
    csv_line_s *ret = csv_line_init(&csv, 10, 2);

    ut_assert(ret == &csv);

    ut_assert(csv.read_size == 10);
    ut_assert(csv.size == 10);
    ut_assert(csv.fields_size == 2);

    ut_assert(ut_is_not_NULL(csv.buffer));
    ut_assert(ut_is_not_NULL(csv.fields));

    csv_line_free(&csv);
    ut_assert(ut_is_NULL(csv.buffer));
    ut_assert(ut_is_NULL(csv.fields));
}

void test_init_defaults() {
    csv_line_s csv;
    csv_line_s *ret = csv_line_init(&csv, 0, 0);

    ut_assert(ret == &csv);

    ut_assert(csv.read_size == DEFAULT_READ_SIZE);
    ut_assert(csv.size == DEFAULT_READ_SIZE);
    ut_assert(csv.fields_size == DEFAULT_FIELD_SIZE);

    csv_line_free(&csv);
    ut_assert(ut_is_NULL(csv.buffer));
    ut_assert(ut_is_NULL(csv.fields));
}

void create_test_file(char *file_name, char *contents) {
    FILE *fp = fopen(file_name, "wb");
    ut_assert(ut_is_not_NULL(fp));
    fwrite(contents, 1, strlen(contents), fp);
    fclose(fp);
}

void test_fill_buffer() {
    char *TEST_FILE = "test/test_fill_buffer.txt";
    char *TEST_DATA = "This is a test";
    size_t TEST_DATA_LEN = strlen(TEST_DATA);

    create_test_file(TEST_FILE, TEST_DATA);

    csv_line_s csv;
    csv_line_init(&csv, 2, 0);
    csv_line_open_file(&csv, TEST_FILE);

    uint8_t iter = 0;
    while (iter++ < 8 && !csv.eof) {
        csv_line_fill_buffer(&csv);
    }

    ut_assert(csv.eof);
    ut_assert(ut_number_equals(16, csv.size));
    ut_assert(ut_number_equals(TEST_DATA_LEN, csv.end));
    ut_assert(memcmp(csv.buffer, TEST_DATA, TEST_DATA_LEN) == 0);

    fclose(csv.file);
    csv_line_free(&csv);
}

void test_fill_buffer_keep_data() {
    char *TEST_FILE = "test/test_fill_buffer_keep_data.txt";
    char *TEST_DATA = "is a test";
    char *EXPECTED_DATA = "This is a test";
    size_t EXPECTED_DATA_LEN = strlen(EXPECTED_DATA);

    create_test_file(TEST_FILE, TEST_DATA);

    csv_line_s csv;
    csv_line_init(&csv, 10, 0);

    csv.file = fopen(TEST_FILE, "rb");
    ut_assert(ut_is_not_NULL(csv.file));

    csv.start = 5;
    strcat(&csv.buffer[csv.start], "This ");
    csv.end = 10;

    uint8_t iter = 0;
    while (iter++ < 1 && !csv.eof) {
        csv_line_fill_buffer(&csv);
    }

    ut_assert(csv.eof);
    ut_assert(ut_number_equals(20, csv.size));
    ut_assert(ut_number_equals(0, csv.start));
    ut_assert(ut_number_equals(EXPECTED_DATA_LEN, csv.end));
    ut_assert(memcmp(csv.buffer, EXPECTED_DATA, EXPECTED_DATA_LEN) == 0);

    fclose(csv.file);
    csv_line_free(&csv);
}

void assert_columns_equal(csv_line_s *csv, size_t count, char *expected[]) {
    ut_assert(ut_number_equals(count, csv->fields_count));
    for (int i = 0; i < count; i++) {
        ut_assert(ut_str_equals(expected[i], &csv->buffer[csv->fields[i]]));
    }
}

void test_read_line_till_eof() {
    char *TEST_FILE = "test/test_read_line.csv";
    char *TEST_DATA = "ONE,TWO,THREE";
    char *EXPECTED_COLUMNS[] = {"ONE", "TWO", "THREE"};
    create_test_file(TEST_FILE, TEST_DATA);

    csv_line_s csv;
    csv_line_init(&csv, 20, 5);
    csv_line_open_file(&csv, TEST_FILE);
    csv_line_read_line(&csv);
    ut_assert(csv.eof);
    assert_columns_equal(&csv, 3, EXPECTED_COLUMNS);
}

char *SIMPLE_COLUMNS[][3] = {
    {"ONE", "TWO", "THREE"},
    {"1", "2", "3"},
};

size_t READ_SIZE[] = {11, 0};  // 2, 3, 5, 8, 13, 21, 0};
assert_file_matches_simple_columns(char *file_name, size_t read_size) {
    csv_line_s csv;
    csv_line_init(&csv, read_size, 5);
    csv_line_open_file(&csv, file_name);

    csv_line_read_line(&csv);
    assert_columns_equal(&csv, 3, SIMPLE_COLUMNS[0]);

    csv_line_read_line(&csv);
    ut_assert(csv.eof);
    assert_columns_equal(&csv, 3, SIMPLE_COLUMNS[1]);

    csv_line_close_file(&csv);
    csv_line_free(&csv);
}

void test_read_line_lf() {
    char *TEST_FILE = "test/test_read_line_lf.csv";
    char *TEST_DATA = "ONE,TWO,THREE\n1,2,3\n";
    create_test_file(TEST_FILE, TEST_DATA);

    for (int i = 0; READ_SIZE[i] != 0; i++) {
        assert_file_matches_simple_columns(TEST_FILE, READ_SIZE[i]);
    }
}

void test_read_line_cr() {
    char *TEST_FILE = "test/test_read_line_cr.csv";
    char *TEST_DATA = "ONE,TWO,THREE\r1,2,3\r";
    create_test_file(TEST_FILE, TEST_DATA);

    for (int i = 0; READ_SIZE[i] != 0; i++) {
        assert_file_matches_simple_columns(TEST_FILE, READ_SIZE[i]);
    }
}

void test_read_line_cr_lf() {
    char *TEST_FILE = "test/test_read_line_cr_lf.csv";
    char *TEST_DATA = "ONE,TWO,THREE\r\n1,2,3\r\n";
    create_test_file(TEST_FILE, TEST_DATA);

    for (int i = 0; READ_SIZE[i] != 0; i++) {
        assert_file_matches_simple_columns(TEST_FILE, READ_SIZE[i]);
    }
}

int main(int argc, char **argv) {
    // ut_run(test_init_free);
    // ut_run(test_init_defaults);
    // ut_run(test_fill_buffer);
    // ut_run(test_fill_buffer_keep_data);
    ut_run(test_read_line_till_eof);
    ut_run(test_read_line_lf);
    ut_run(test_read_line_cr);
    ut_run(test_read_line_cr_lf);
    return ut_end();
}

#endif  // UNIT_TEST
