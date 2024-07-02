#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint8_t *buffer;
    size_t size;
    size_t read_size;
    size_t start;
    size_t end;
    uint8_t eof;
    uint8_t **fields;
    size_t fields_size;
    size_t index;
} csv_line_s;

#define DEFAULT_READ_SIZE 4096
#define DEFAULT_FIELD_SIZE 1024

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
    if ((csv->fields = malloc(csv->fields_size)) == NULL) {
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

void csv_line_fill_buffer(csv_line_s *csv, FILE *fp) {
    if (csv->eof) {
        return;
    }

    size_t space = csv->size - csv->end;
    if (space < csv->read_size) {
        if (csv->start > 0) {
            size_t len = csv->end - csv->start;
            memcpy(csv->buffer, &csv->buffer[csv->start], len);
            csv->end = len;
            space = csv->size - csv->end;
        }
        if (space < csv->read_size) {
            csv->size += csv->read_size;
            csv->buffer = realloc(csv->buffer, csv->size);  // todo: check alloc
        }
    }

    size_t read = fread(&csv->buffer[csv->end], 1, csv->read_size, fp);
    csv->eof = feof(fp);
    csv->end += read;
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

    FILE *fp = fopen(TEST_FILE, "rb");
    ut_assert(ut_is_not_NULL(fp));

    csv_line_s csv;
    csv_line_init(&csv, 2, 0);

    uint8_t iter = 0;
    while (++iter < 9 && !csv.eof) {
        csv_line_fill_buffer(&csv, fp);
    }

    ut_assert(csv.eof);
    ut_assert(ut_number_equals(16, csv.size));
    ut_assert(ut_number_equals(TEST_DATA_LEN, csv.end));
    ut_assert(memcmp(csv.buffer, TEST_DATA, TEST_DATA_LEN) == 0);

    fclose(fp);
    csv_line_free(&csv);
}

int main(int argc, char **argv) {
    ut_run(test_init_free);
    ut_run(test_init_defaults);
    ut_run(test_fill_buffer);
    return ut_end();
}

#endif  // UNIT_TEST
