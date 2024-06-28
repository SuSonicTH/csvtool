#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint8_t *buffer;
    size_t size;
} csv_line_s;

csv_line_s *csv_line_init(csv_line_s *csv_line, size_t minimum_size) {
    if (csv_line == NULL || (csv_line->buffer = malloc(minimum_size)) == NULL) {
        return NULL;
    }
    csv_line->size = minimum_size;
    return csv_line;
}

csv_line_s *csv_line_new(size_t minimum_size) {
    csv_line_s *csv_line = malloc(sizeof(csv_line_s));
    return csv_line_init(csv_line, minimum_size);
}

void csv_line_deinit(csv_line_s *csv_line) {
    if (csv_line->buffer != NULL) {
        free(csv_line->buffer);
    }
}

void csv_line_free(csv_line_s *csv_line) {
    if (csv_line != NULL) {
        csv_line_deinit(csv_line);
        free(csv_line);
    }
}

#ifdef UNIT_TEST
#include "unit_test.h"

void test_init() {
    csv_line_s csv_line;
    csv_line_s *ret = csv_line_init(&csv_line, 10);
    ut_assert(ret == &csv_line);
    ut_assert(csv_line.size == 10);
}

int main(int argc, char **argv) {
    ut_run(test_init);
    return ut_end();
}

#endif  // UNIT_TEST
