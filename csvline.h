#ifndef CSV_LINE_INCLUDED
#define CSV_LINE_INCLUDED
#include <stdint.h>
typedef struct {
    char *file_name;
    FILE *file;
    uint8_t *buffer;
    size_t size;
    size_t read_size;
    char separator;

    size_t start;
    size_t next;
    size_t end;

    size_t *fields;
    size_t fields_size;
    size_t fields_count;
} csv_line_s;

csv_line_s *csv_line_init(csv_line_s *csv, char separator, size_t read_size, size_t fields_size);
void csv_line_free(csv_line_s *csv);
void csv_line_open_file(csv_line_s *csv, char *file_name);
size_t csv_line_read_line(csv_line_s *csv);

#endif  // CSV_LINE_INCLUDED
