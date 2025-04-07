#include <stdio.h>

static int var_0 = 1; // Initial value from Z3 model
static int var_1 = -1; // Initial value from Z3 model
static int var_2 = -1; // Initial value from Z3 model
static int var_3 = 1; // Initial value from Z3 model
static int var_4 = -1; // Initial value from Z3 model
static int var_5 = 1; // Initial value from Z3 model
static int var_6 = -9; // Initial value from Z3 model
static int var_7 = -1; // Initial value from Z3 model

int main() {
BB0:
    var_5 = -1 * var_6 + -1 * var_2 + -11;
    goto BB3;

BB1:
    var_0 = -1 * var_4 + -2147483646 * var_7 + -2147483647;
    if (-1 * var_0 + -2 * var_5 + -3 >= 0) goto BB4;
    goto BB5;

BB2:
    var_6 = -1 * var_1 + 3 * var_6 + -9;
    if (2 * var_6 + -1 * var_1 + 15 >= 0) goto BB3;
    goto BB1;

BB3:
    var_6 = -1 * var_4 + -1 * var_6 + -10;
    if (-3 * var_6 + -1 * var_0 + -3 >= 0) goto BB9;
    goto BB2;

BB4:
    var_3 = -1 * var_7 + -2 * var_5 + -1;
    if (-2 * var_3 + -1 * var_6 + -2147483648 >= 0) goto BB6;
    goto BB8;

BB5:
    var_3 = -2 * var_5 + -3 * var_4 + -1;
    if (-2 * var_3 + -1 * var_4 + 6 >= 0) goto BB6;
    goto BB4;

BB6:
    var_1 = -1034123402 * var_2 + 580214587 * var_6 + -878613173;
    if (-1963412294 * var_1 + -311877855 * var_7 + -1046678980 >= 0) goto BB9;
    goto BB7;

BB7:
    var_7 = -34035920 * var_4 + 1589304441 * var_3 + 1379578321;
    if (-1657949640 * var_7 + 918775082 * var_1 + 1991407432 >= 0) goto BB10;
    goto BB9;

BB8:
    var_3 = -1073741821 * var_1 + -2 * var_6 + -19;
    if (-2 * var_3 + -162688155 * var_6 + 357913943 >= 0) goto BB3;
    goto BB2;

BB9:
    var_0 = -1 * var_7 + -1 * var_1 + -2147483648;
    goto BB11;

BB10:
    var_4 = 1827704111 * var_5 + -2113222787 * var_2 + -814100521;
    goto BB8;

BB11:
    var_7 = -1 * var_0 + -1 * var_4 + -2147483648;
    printf("%d,%d,%d,%d,%d,%d,%d,%d", var_0, var_1, var_2, var_3, var_4, var_5, var_6, var_7);
    return 0;

}
