#include <stdio.h>

int var_0 = 1; // Initial value from Z3 model
int var_1 = 0; // Initial value from Z3 model
int var_2 = 0; // Initial value from Z3 model
int var_3 = 0; // Initial value from Z3 model
int var_4 = 0; // Initial value from Z3 model
int var_5 = 1; // Initial value from Z3 model
int var_6 = 1; // Initial value from Z3 model
int var_7 = 1; // Initial value from Z3 model

int main() {
BB0:
    var_0 = 1 * var_6 + 0 * var_4 + -1;
    var_7 = -2147483646 * var_7 + 0 * var_4 + 2147483646;
    if (0 * var_7 + 0 * var_0 + -2147483647 * var_5 + -1 * var_6 + 0 >= 0) goto BB4;
    goto BB7;

BB1:
    var_1 = 0 * var_1 + -2147483646 * var_6 + 2147483646;
    var_7 = 1 * var_7 + -2147483647 * var_1 + 0;
    if (0 * var_7 + 0 * var_1 + 1 * var_5 + 1 * var_0 + -2147483647 >= 0) goto BB3;
    goto BB2;

BB2:
    var_1 = -2147483647 * var_6 + 0 * var_2 + 2147483647;
    var_3 = -2147483647 * var_2 + 1 * var_1 + 0;
    if (0 * var_3 + 0 * var_1 + 1 * var_0 + -2147483647 * var_5 + 0 >= 0) goto BB10;
    goto BB8;

BB3:
    var_2 = -1195473884 * var_5 + 955954005 * var_6 + 916437243;
    var_3 = 654031290 * var_3 + -157018322 * var_2 + -620884388;
    if (1525053814 * var_2 + 481974643 * var_3 + -233871628 * var_7 + -172798888 * var_4 + 569295831 >= 0) goto BB4;
    goto BB6;

BB4:
    var_3 = 1 * var_2 + 1 * var_4 + 0;
    var_5 = -2147483647 * var_1 + 1 * var_7 + 0;
    if (0 * var_3 + 0 * var_5 + 1 * var_4 + 1 * var_1 + 0 >= 0) goto BB9;
    goto BB6;

BB5:
    var_1 = 2019477050 * var_0 + 669102570 * var_4 + -2051236086;
    var_4 = 1694310592 * var_1 + 1580109870 * var_0 + -1713209782;
    if (-796838093 * var_1 + 190981514 * var_4 + 1432846976 * var_2 + 390251659 * var_7 + -1051285377 >= 0) goto BB10;
    goto BB6;

BB6:
    var_0 = 730238160 * var_0 + 615722507 * var_5 + 1414248022;
    var_4 = -2066176040 * var_7 + -281677449 * var_1 + -876274262;
    if (276536255 * var_0 + -1792647022 * var_4 + 427564335 * var_3 + -1028442402 * var_2 + -920788530 >= 0) goto BB3;
    goto BB11;

BB7:
    var_0 = 0 * var_3 + -2147483646 * var_6 + 2147483646;
    var_4 = 1 * var_1 + 1 * var_4 + 0;
    if (0 * var_0 + 0 * var_4 + 1 * var_3 + -2147483647 * var_6 + 0 >= 0) goto BB5;
    goto BB9;

BB8:
    var_2 = 1 * var_3 + -2147483647 * var_2 + 0;
    var_4 = -2147483646 * var_5 + 0 * var_7 + 2147483646;
    if (0 * var_4 + 0 * var_2 + -2147483647 * var_6 + -2147483647 * var_1 + 0 >= 0) goto BB5;
    goto BB4;

BB9:
    var_1 = 1 * var_5 + 0 * var_1 + -1;
    var_4 = 1 * var_5 + 0 * var_0 + -1;
    if (0 * var_1 + 0 * var_4 + 1 * var_5 + -2147483647 * var_6 + 2147483646 >= 0) goto BB1;
    goto BB11;

BB10:
    var_0 = -548067980 * var_5 + -1299155055 * var_6 + -1644068330;
    var_3 = -1413778619 * var_6 + 736968430 * var_5 + -1216000034;
    if (-1405667361 * var_3 + -1356419796 * var_0 + 1033889683 * var_7 + 1698304080 * var_5 + 1541192402 >= 0) goto BB11;
    goto BB1;

BB11:
    var_2 = 1 * var_7 + -2147483647 * var_3 + 0;
    var_6 = 1 * var_1 + 0 * var_2 + -2147483646;
    printf("%d,%d,%d,%d,%d,%d,%d,%d", var_0, var_1, var_2, var_3, var_4, var_5, var_6, var_7);
    return 0;

}
