#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include "deps/pcg-c-basic/pcg_basic.h"
#include "generate.h"


#define MAX(a,b) (((a) > (b)) ? (a) : (b))

inline static bool one_every(pcg32_random_t* rng, int one_in) {
    return pcg32_random_r(rng) < (UINT32_MAX / one_in);
}

static double random_float(pcg32_random_t* rng) {
    return ldexp(pcg32_random_r(rng), -32);
}

#define RANDOM_RANGE(rng, a,b) ((a) + pcg32_boundedrand_r((rng), (b) - (a)))

static char random_alpha(pcg32_random_t* rng) {
    if (one_every(rng, 2)) {
        return RANDOM_RANGE(rng, 'A', 'Z');
    }
    return RANDOM_RANGE(rng, 'a', 'z');
}

static char random_numeric(pcg32_random_t* rng) {
    return RANDOM_RANGE(rng, '0', '9');
}

static char random_alpha_numeric(pcg32_random_t* rng) {
    if (one_every(rng, 2)) {
        return random_numeric(rng);
    }
    return random_alpha(rng); 
}


static size_t random_cell(pcg32_random_t* rng, char* restrict target, const unsigned int columns, const size_t cell_size_max) {
    size_t written = 0;
    for (unsigned int i = 0; i < columns; i++) {
        if (i > 0) {
            *target++ =',';
            written++;
        }
        size_t cell_size = pcg32_boundedrand_r(rng, random_float(rng) < 0.2 ? cell_size_max : MAX(1, cell_size_max / 40));
        if (cell_size < 2) {
            cell_size = 2;
        }
        if (one_every(rng, 3)) {
            for (size_t c = 0; c < cell_size; c++) {
                *target++ = random_numeric(rng);
            }
        }
        else if (!one_every(rng, 10)) {
            for (size_t c = 0; c < cell_size; c++) {
                *target++ = random_alpha_numeric(rng);
            }
        }
        else {
            *target++ = '"';
            written++;
            cell_size -= 2;
            for (size_t c = 0; c < cell_size; c++) {
                *target++ = random_alpha(rng);
                if (c + 2 < cell_size) {
                    if (one_every(rng, 4)) {
                        *target++ = ' ';
                        cell_size--;
                        written++;
                    }
                    else if (one_every(rng, 6)) {
                        *target++ = ',';
                        cell_size--;
                        written++;
                    }
                    else if (one_every(rng, 100)) {
                        *target++ ='"';
                        *target++ ='"';
                        written += 2;
                        cell_size -= 2;
                    }
                    else if (one_every(rng, 1000)) {
                        *target++ ='\n';
                        cell_size--;
                        written++;
                    }
                }
            }
            *target++ = '"';
            written++;
        }
        written += cell_size;
    }
    *target++ = '\n';
    written++;
    return written;
}

size_t generate_csv(char* restrict buffer, size_t size, size_t* ten_percent, unsigned int seed1, unsigned int seed2, unsigned int columns) {
    const size_t original_size = size;
    char* restrict current_char = buffer;
    for (unsigned int i = 1; i <= columns; i++) {
        if (i > 1) {
            *current_char++ = ',';
            size--;
        }
        memcpy(current_char, "column", 6);
        current_char += 6;
        size -= 6;
        int len = snprintf(current_char, (CHAR_BIT * sizeof(int) - 1) / 3 + 2, "%d", i);
        current_char += len;
        size -= len;
    }
    *current_char++ = '\n';
    size--;

    pcg32_random_t rng;

    pcg32_srandom_r(&rng, seed1, seed2);

    unsigned int cell_large_max = 255;

    while (size > ((cell_large_max + 1) * columns + 1)) {
        size_t written = random_cell(&rng, current_char, columns, cell_large_max);
        current_char += written;
        size -= written;
        if (original_size - size < (original_size / 10)) {
            *ten_percent = original_size - size;
        }
    }
    size_t written = random_cell(&rng, current_char, columns, 2);
    current_char += written;
    return current_char - buffer;
}
