#include <stdio.h>

int var_0 = 1; // Initial value from Z3 model
int var_1 = 1; // Initial value from Z3 model
int var_2 = -2147483647; // Initial value from Z3 model
int var_3 = -2147483647; // Initial value from Z3 model
int var_4 = 1; // Initial value from Z3 model

int main() {
BB0:
    var_3 = 0 * var_3 + -2147483646 * var_0 + 2147483646;
    var_4 = -1 * var_2 + 0 * var_1 + -2147483647;
    if (0 * var_4 + 1 * var_3 + -1 * var_2 + 0 >= 0) goto BB4;
    goto BB7;

BB1:
    var_0 = 1250060717 * var_0 + 1614890985 * var_4 + 913574884;
    var_1 = -1965066263 * var_2 + -261306797 * var_1 + -860802682;
    if (-1292827663 * var_1 + -1983701387 * var_0 + 109768442 * var_4 + -1493363988 >= 0) goto BB3;
    goto BB6;

BB2:
    var_1 = -2107249545 * var_2 + -482844037 * var_3 + -1186367518;
    var_4 = -285074449 * var_0 + 780263998 * var_4 + 859192247;
    if (1132710069 * var_4 + -1407916628 * var_1 + -470859720 * var_3 + 660462229 >= 0) goto BB7;
    goto BB9;

BB3:
    var_3 = -2147483646 * var_1 + 0 * var_0 + 2147483646;
    var_4 = -1 * var_2 + 0 * var_3 + -2147483647;
    if (1 * var_4 + 0 * var_3 + -2147483647 * var_1 + 0 >= 0) goto BB2;
    goto BB7;

BB4:
    var_0 = -2147483647 * var_1 + -2147483647 * var_4 + 0;
    var_3 = -1 * var_2 + 0 * var_4 + -2147483647;
    if (0 * var_3 + -1 * var_0 + 1 * var_2 + 0 >= 0) goto BB3;
    goto BB9;

BB5:
    var_1 = -1 * var_0 + -2147483647 * var_1 + 2;
    var_3 = -1 * var_1 + 0 * var_0 + -2;
    if (-1 * var_1 + 0 * var_3 + 1 * var_4 + 0 >= 0) goto BB9;
    goto BB7;

BB6:
    var_0 = 0 * var_2 + 1 * var_0 + 1;
    var_4 = -2147483647 * var_1 + 0 * var_2 + 2147483646;
    if (-1 * var_0 + 0 * var_4 + 1 * var_2 + -2147483643 >= 0) goto BB5;
    goto BB7;

BB7:
    var_2 = 0 * var_0 + -2147483647 * var_4 + -2147483647;
    var_4 = -1 * var_1 + 0 * var_0 + 1;
    if (-2147483644 * var_4 + 0 * var_2 + -1 * var_0 + 0 >= 0) goto BB6;
    goto BB8;

BB8:
    var_1 = 1570793959 * var_1 + 739642032 * var_3 + -438222481;
    var_3 = -763381107 * var_1 + 919929267 * var_4 + -359508203;
    if (720170682 * var_1 + -1242346930 * var_3 + 1783828119 * var_2 + -1719911119 >= 0) goto BB2;
    goto BB9;

BB9:
    var_2 = -2147483647 * var_2 + 0 * var_4 + -2147483647;
    var_4 = 0 * var_0 + -1 * var_2 + -2147483647;
    printf("%d,%d,%d,%d,%d", var_0, var_1, var_2, var_3, var_4);
    return 0;

}
