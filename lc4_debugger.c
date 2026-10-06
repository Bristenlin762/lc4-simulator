#include "lc4_cpu.h"
#include "lc4_hex.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* Check for non-negative integer */
static int parse_number(const char *text, int base,
                        unsigned long limit,
                        unsigned long *value)
{
    char *end;

    if (text[0] == '-' || text[0] == '\0') {
        return 1;
    }

    errno = 0;
    unsigned long result = strtoul(text, &end, base);

    if (errno != 0 || *end != '\0' || result > limit) {
        return 1;
    }

    *value = result;
    return 0;
}

static void print_help(void)
{
    printf("Commands:\n");
    printf("  step            Execute one instruction\n");
    printf("  run N           Execute at most N instructions (1-1000000)\n");
    printf("  regs            Show CPU registers\n");
    printf("  mem ADDRESS N   Show N words; ADDRESS is hexadecimal\n");
    printf("  help            Show commands\n");
    printf("  quit            Exit debugger\n");
}

int main(int argc, char **argv)
{
    static lc4_cpu cpu;
    char line[256];

    if (argc != 2 && argc != 3) {
        fprintf(stderr, "Usage: %s program.hex [START_ADDRESS]\n", argv[0]);
        return 1;
    }

    unsigned long start = 0;
    if (argc == 3 && parse_number(argv[2], 16, 0xFFFFu, &start) != 0) {
        fprintf(stderr, "Invalid hexadecimal start address\n");
        return 1;
    }
    lc4_cpu_init(&cpu, (uint16_t)start);

    if (lc4_cpu_load_hex(&cpu, argv[1], (uint16_t)start) != 0) {
        fprintf(stderr, "Failed to load: %s\n", argv[1]);
        return 1;
    }

    printf("LC4 Debugger\n");
    printf("Loaded: %s\n", argv[1]);
    printf("Entry address: x%04X\n\n", (unsigned int)cpu.pc);
    print_help();

    while (1) {
        printf("\nlc4> ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }

        /* Discard overly long input to avoid being split into multiple commands */
        if (strchr(line, '\n') == NULL && !feof(stdin)) {
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) {
            }
            printf("Command is too long\n");
            continue;
        }

        char command[16];
        char arg1[32];
        char arg2[32];
        char extra[2];

        int fields = sscanf(line, "%15s %31s %31s %1s",
                            command, arg1, arg2, extra);

        if (fields < 1) {
            continue;
        }

        if (strcmp(command, "quit") == 0 && fields == 1) {
            break;

        } else if (strcmp(command, "help") == 0 && fields == 1) {
            print_help();

        } else if (strcmp(command, "regs") == 0 && fields == 1) {
            lc4_cpu_print_regs(&cpu);
            printf("Privileged: %d\n", (int)cpu.privileged);

        } else if (strcmp(command, "step") == 0 && fields == 1) {
            uint16_t old_pc = cpu.pc;
            uint16_t instruction = cpu.memory[old_pc];

            if (lc4_cpu_step(&cpu) == 0) {
                printf("Executed x%04X at x%04X\n",
                       (unsigned int)instruction,
                       (unsigned int)old_pc);

                lc4_cpu_print_regs(&cpu);
            }

        } else if (strcmp(command, "run") == 0 && fields == 2) {
            unsigned long budget;
            if (parse_number(arg1, 10, 1000000u, &budget) != 0 || budget == 0) {
                printf("Use: run COUNT (decimal 1-1000000)\n");
                continue;
            }
            unsigned long executed = 0;
            while (executed < budget && !cpu.halted) {
                if (lc4_cpu_step(&cpu) != 0) {
                    break;
                }
                executed++;
            }
            printf("Executed %lu instruction(s); paused\n", executed);
            lc4_cpu_print_regs(&cpu);

        } else if (strcmp(command, "mem") == 0 && fields == 3) {
            unsigned long address;
            unsigned long count;

            if (parse_number(arg1, 16, 0xFFFFu, &address) != 0 ||
                parse_number(arg2, 10, 256u, &count) != 0 ||
                count == 0) {
                printf("Use: mem ADDRESS COUNT\n");
                printf("ADDRESS: hex 0000–FFFF; COUNT: decimal 1–256\n");
                continue;
            }

            lc4_cpu_print_memory(&cpu,
                                 (uint16_t)address,
                                 (uint32_t)count);

        } else {
            printf("Unknown command or invalid arguments. Type help.\n");
        }
    }

    printf("Debugger closed\n");
    return 0;
}