#include "lc4_cpu.h"
#include "lc4_hex.h"
#include <assert.h>
#include <stdio.h>

static lc4_cpu cpu;

static void fresh(void)
{
    lc4_cpu_init(&cpu, 0);
    cpu.regs[0] = 0xFFFA; /* -6 */
    cpu.regs[1] = 3;
}

static void execute(uint16_t instruction)
{
    cpu.memory[cpu.pc] = instruction;
    assert(lc4_cpu_step(&cpu) == 0);
}

static void test_results(void)
{
    struct { uint16_t instruction, expected; uint8_t nzp; } cases[] = {
        {0x1401, 0xFFFD, LC4_N}, /* ADD */
        {0x1409, 0xFFEE, LC4_N}, /* MUL */
        {0x1411, 0xFFF7, LC4_N}, /* SUB */
        {0x1419, 0xFFFE, LC4_N}, /* DIV */
        {0x143F, 0xFFF9, LC4_N}, /* ADD immediate -1 */
        {0x5401, 2, LC4_P},      /* AND */
        {0x5409, 5, LC4_P},      /* NOT */
        {0x5411, 0xFFFB, LC4_N}, /* OR */
        {0x5419, 0xFFF9, LC4_N}, /* XOR */
        {0x543E, 0xFFFA, LC4_N}, /* AND immediate -2 */
        {0x9400, 0, LC4_Z},      /* CONST zero */
        {0x94FF, 255, LC4_P},
        {0x9500, 0xFF00, LC4_N}, /* CONST -256 */
        {0x95FF, 0xFFFF, LC4_N}, /* CONST -1 */
        {0xA401, 0xFFF4, LC4_N}, /* SLL */
        {0xA411, 0xFFFD, LC4_N}, /* SRA */
        {0xA421, 0x7FFD, LC4_P}, /* SRL */
        {0xA431, 0, LC4_Z},      /* MOD */
        {0xA410, 0xFFFA, LC4_N}, /* SRA by zero */
        {0xA41F, 0xFFFF, LC4_N}, /* SRA by 15 */
        {0xA42F, 1, LC4_P}       /* SRL by 15 */
    };
    for (unsigned int i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        fresh();
        execute(cases[i].instruction);
        assert(cpu.regs[2] == cases[i].expected);
        assert(cpu.nzp == cases[i].nzp);
        assert(cpu.pc == 1);
    }
    fresh();
    cpu.regs[2] = 0xAB34;
    execute(0xD512); /* HICONST R2, x12 */
    assert(cpu.regs[2] == 0x1234 && cpu.nzp == LC4_P);

    fresh();
    cpu.regs[0] = 0x7FFF;
    cpu.regs[1] = 1;
    execute(0x1401);
    assert(cpu.regs[2] == 0x8000 && cpu.nzp == LC4_N);
    fresh();
    cpu.regs[0] = 0x8000;
    cpu.regs[1] = 0xFFFF;
    execute(0x1419); /* -32768 / -1, retained as 16 bits */
    assert(cpu.regs[2] == 0x8000);
    fresh();
    cpu.regs[0] = 0xFFF9; /* -7 */
    execute(0xA431);
    assert(cpu.regs[2] == 0xFFFF); /* C remainder convention */
}

static void test_compare_and_branch(void)
{
    const uint16_t instructions[] = {0x2001, 0x2081, 0x2100, 0x2183};
    const uint8_t expected[] = {LC4_N, LC4_P, LC4_N, LC4_P};
    for (unsigned int i = 0; i < 4; i++) {
        fresh();
        execute(instructions[i]);
        assert(cpu.nzp == expected[i] && cpu.regs[0] == 0xFFFA);
    }
    fresh();
    cpu.regs[0] = 0x8000;
    cpu.regs[1] = 1;
    execute(0x2001);
    assert(cpu.nzp == LC4_N); /* no overflow in comparison */
    fresh();
    cpu.regs[0] = 3;
    execute(0x2001);
    assert(cpu.nzp == LC4_Z);

    const uint8_t flags[] = {LC4_N, LC4_Z, LC4_P};
    for (unsigned int mask = 0; mask < 8; mask++) {
        for (unsigned int i = 0; i < 3; i++) {
            fresh();
            cpu.nzp = flags[i];
            execute((uint16_t)((mask << 9) | 0x01FF)); /* offset -1 */
            assert(cpu.pc == ((mask & flags[i]) ? 0 : 1));
            assert(cpu.nzp == flags[i]);
        }
    }
    fresh();
    cpu.pc = 0xFFFF;
    execute(0x0000);
    assert(cpu.pc == 0);
}

static void test_control_flow(void)
{
    fresh();
    execute(0x4801); /* JSR x0010 */
    assert(cpu.pc == 0x0010 && cpu.regs[7] == 1 && cpu.nzp == LC4_P);
    fresh();
    cpu.pc = 0x8123;
    execute(0x4FFF);
    assert(cpu.pc == 0xFFF0 && cpu.regs[7] == 0x8124);
    fresh();
    cpu.regs[7] = 0x1234;
    execute(0x41C0); /* JSRR R7 */
    assert(cpu.pc == 0x1234 && cpu.regs[7] == 1);
    fresh();
    execute(0xCFFF); /* JMP -1 */
    assert(cpu.pc == 0 && cpu.nzp == LC4_Z);
    fresh();
    cpu.regs[7] = 0x1234;
    execute(0xC1C0); /* JMPR R7 / RET */
    assert(cpu.pc == 0x1234);
    fresh();
    execute(0xF025);
    assert(cpu.pc == 0x8025 && cpu.regs[7] == 1 && cpu.privileged);
    execute(0x8000);
    assert(cpu.pc == 1 && !cpu.privileged);
}

static void test_memory_and_errors(void)
{
    fresh();
    cpu.regs[2] = 0x2001;
    cpu.nzp = LC4_P;
    execute(0x70BF); /* STR R0, R2, #-1 */
    assert(cpu.memory[0x2000] == 0xFFFA && cpu.nzp == LC4_P);
    execute(0x66BF); /* LDR R3, R2, #-1 */
    assert(cpu.regs[3] == 0xFFFA && cpu.nzp == LC4_N);
    fresh();
    cpu.regs[2] = 0;
    cpu.memory[0xFFFF] = 0x1234;
    execute(0x66BF);
    assert(cpu.regs[3] == 0x1234);

    const uint16_t invalid[] = {0x3000, 0xB000, 0xE000, 0x8000};
    for (unsigned int i = 0; i < 4; i++) {
        fresh();
        cpu.memory[0] = invalid[i];
        assert(lc4_cpu_step(&cpu) != 0);
        assert(cpu.pc == 0 && cpu.nzp == LC4_Z && cpu.regs[0] == 0xFFFA);
    }
    const uint16_t zero_divisor[] = {0x1419, 0xA431};
    for (unsigned int i = 0; i < 2; i++) {
        fresh();
        cpu.regs[1] = 0;
        cpu.regs[2] = 0xBEEF;
        cpu.memory[0] = zero_divisor[i];
        assert(lc4_cpu_step(&cpu) != 0);
        assert(cpu.pc == 0 && cpu.regs[2] == 0xBEEF && cpu.nzp == LC4_Z);
    }
    fresh();
    cpu.halted = true;
    assert(lc4_cpu_step(&cpu) != 0 && cpu.pc == 0);
}

int main(void)
{
    test_results();
    test_compare_and_branch();
    test_control_flow();
    test_memory_and_errors();
    lc4_cpu_init(&cpu, 0);
    assert(lc4_cpu_load_hex(&cpu, "examples/sum.hex", 0) == 0);
    for (int i = 0; i < 21; i++) assert(lc4_cpu_step(&cpu) == 0);
    assert(cpu.pc == 9 && cpu.regs[0] == 15 && cpu.regs[1] == 0);
    assert(cpu.regs[3] == 15 && cpu.memory[0x2000] == 15);
    puts("CPU instruction and sum integration checks passed");
    return 0;
}
