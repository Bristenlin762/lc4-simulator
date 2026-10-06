#ifndef LC4_HEX_H
#define LC4_HEX_H

#include "lc4_cpu.h"

/* Plain hexadecimal words, optional @address lines, and # or ; comments.
 * Failure leaves CPU memory unchanged. Loading does not change PC. */
int lc4_cpu_load_hex(lc4_cpu *cpu, const char *filename, uint16_t start);

#endif
