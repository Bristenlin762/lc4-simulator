#include "lc4_hex.h"
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int hex_word(const char *text, unsigned long *result)
{
    char *end;
    if (!isxdigit((unsigned char)*text)) return 1;
    errno = 0;
    unsigned long value = strtoul(text, &end, 16);
    while (isspace((unsigned char)*end)) end++;
    if (errno || *end || value > 0xFFFFu) return 1;
    *result = value;
    return 0;
}

int lc4_cpu_load_hex(lc4_cpu *cpu, const char *filename, uint16_t start)
{
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror(filename);
        return 1;
    }
    uint16_t *staged = malloc(sizeof(cpu->memory));
    if (!staged) {
        fclose(file);
        fprintf(stderr, "Cannot allocate staging memory\n");
        return 1;
    }
    memcpy(staged, cpu->memory, sizeof(cpu->memory));
    char line[256];
    unsigned long address = start, value;
    unsigned long line_number = 0, words = 0;
    int failed = 0;
    while (fgets(line, sizeof(line), file)) {
        line_number++;
        if (!strchr(line, '\n') && !feof(file)) {
            failed = 1;
            break;
        }
        char *comment = strpbrk(line, "#;");
        if (comment) *comment = '\0';
        char *text = line;
        while (isspace((unsigned char)*text)) text++;
        if (!*text) continue;
        if (*text == '@') {
            if (hex_word(text + 1, &value)) {
                failed = 1;
                break;
            }
            address = value;
        } else {
            if (hex_word(text, &value) || address >= LC4_MEMORY_SIZE) {
                failed = 1;
                break;
            }
            staged[address++] = (uint16_t)value;
            words++;
        }
    }
    if (ferror(file)) failed = 1;
    if (fclose(file) != 0) failed = 1;
    if (!words) failed = 1;
    if (failed) {
        fprintf(stderr, "Invalid or unreadable hex input near line %lu: %s\n",
                line_number, filename);
    } else {
        memcpy(cpu->memory, staged, sizeof(cpu->memory));
    }
    free(staged);
    return failed;
}
