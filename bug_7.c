#include <stdio.h>

int var_0 = 0; // Initial value from Z3 model
int var_1 = 0; // Initial value from Z3 model
int var_2 = -2147483647; // Initial value from Z3 model
int var_3 = -2147483647; // Initial value from Z3 model
int var_4 = 0; // Initial value from Z3 model
int var_5 = 1; // Initial value from Z3 model

int main() {
BB0:
    var_2 = 0 * var_2 + 1 * var_0 + -2147483646;
    var_5 = 1 * var_3 + 0 * var_5 + -1;
    goto BB2;

BB1:
    var_2 = -1773194267 * var_5 + -1456012416 * var_4 + 602875657;
    var_4 = 107554126 * var_2 + 387819236 * var_0 + 155303997;
    goto BB4;

BB2:
    var_1 = -1 * var_3 + 0 * var_0 + -2147483647;
    var_4 = 1 * var_2 + 0 * var_0 + 2147483646;
    if (-1 * var_4 + 0 * var_1 + 0 >= 0) goto BB5;
    goto BB4;

BB3:
    var_0 = -2147483647 * var_5 + -2147483647 * var_4 + 0;
    var_2 = 1 * var_1 + -2147483647 * var_5 + 0;
    if (0 * var_0 + -2147483647 * var_2 + 0 >= 0) goto BB6;
    goto BB8;

BB4:
    var_0 = -2147483647 * var_1 + -2147483647 * var_2 + 0;
    var_5 = 0 * var_0 + -1 * var_4 + 2147483646;
    if (1 * var_0 + 0 * var_5 + 0 >= 0) goto BB9;
    goto BB5;

BB5:
    var_3 = -2147483647 * var_1 + -2147483647 * var_4 + 0;
    var_5 = -2147483647 * var_1 + 1 * var_4 + 0;
    if (0 * var_3 + -1 * var_5 + -2147483646 >= 0) goto BB2;
    goto BB3;

BB6:
    var_0 = -2147483647 * var_4 + 1 * var_3 + 0;
    var_5 = 1 * var_3 + -2147483647 * var_2 + 0;
    if (1 * var_0 + 0 * var_5 + 0 >= 0) goto BB7;
    goto BB4;

BB7:
    var_1 = -2147483647 * var_3 + 0 * var_2 + 1;
    var_3 = 1 * var_3 + 1 * var_0 + 0;
    if (0 * var_3 + -2147483647 * var_1 + 0 >= 0) goto BB1;
    goto BB5;

BB8:
    var_2 = 1610109307 * var_3 + 1532074467 * var_5 + -486940834;
    var_3 = 611417263 * var_2 + 284684492 * var_0 + -363481816;
    goto BB4;

BB9:
    var_0 = -2147483647 * var_2 + 0 * var_0 + -2147483647;
    var_4 = 0 * var_2 + 1 * var_5 + -2147483647;
    printf("%d,%d,%d,%d,%d,%d", var_0, var_1, var_2, var_3, var_4, var_5);
    return 0;

}
