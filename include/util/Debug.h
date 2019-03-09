#ifndef APOLLO_UTIL_DEBUG_H
#define APOLLO_UTIL_DEBUG_H

void
__apollo_DEBUG_string(const char *str, int cols_per_row) {
    int i;
    for (i = 0; str[i] != '\0'; i++) {
        if ((i % cols_per_row) == 0) {
            printf("\n");
        } else {
            printf("[%02X] %03d %c   ",
                    str[i], (int) str[i], str[i]);
        }
    }
}

#endif //APOLLO_UTIL_DEBUG_H
