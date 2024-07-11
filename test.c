#include <stdio.h>
#include <string.h>

#include "csvline.h"

int main(int argc, char **argv) {
    csv_line_s csv;
    csv_line_init(&csv, ',', 4 * 1024 * 1024, 10);
    csv_line_open_file(&csv, "VTAS_SINGLE_DB.csv");
    while (csv_line_read_line(&csv)) {
        fwrite(&csv.buffer[csv.start + csv.fields[0]], 1, strlen(&csv.buffer[csv.start + csv.fields[0]]), stdout);
        putc('\n', stdout);
    }
}
