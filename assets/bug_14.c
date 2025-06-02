#include <stdio.h>

int var_0 = 88; // Initial value from Z3 model
int var_1 = 25; // Initial value from Z3 model
int var_2 = 64; // Initial value from Z3 model
int var_3 = -1; // Initial value from Z3 model
int var_4 = 35; // Initial value from Z3 model
int var_5 = -20; // Initial value from Z3 model
int var_6 = 28; // Initial value from Z3 model
int var_7 = -8; // Initial value from Z3 model

int pass_counter = 0;
int main() {
BB0:
    var_6 = -9 * var_5 + -10 * var_6 + -2941659;
    goto BB7;

BB1:
    var_5 = -9 * var_0 + -9 * var_5 + -2147483647;
    if (0 * var_5 + -9 * var_6 + 1 * var_7 + -2147483556 >= 0) goto BB8;
    goto BB14;

BB2:
    var_6 = -1886227667 * var_2 + 1226820210 * var_7 + 1675487860;
    if (1571816612 * var_6 + -1942409686 * var_3 + 928889548 * var_4 + 2119105676 >= 0) goto BB4;
    goto BB3;

BB3:
    var_5 = -9 * var_0 + -9 * var_5 + -2147483035;
    if (0 * var_5 + -9 * var_6 + -9 * var_7 + -1909201319 >= 0) goto BB10;
    goto BB5;

BB4:
    var_2 = -543310427 * var_0 + -685046786 * var_2 + 792815945;
    if (1921950082 * var_2 + 562286690 * var_3 + -1334214203 * var_5 + 2053083120 >= 0) goto BB14;
    goto BB2;

BB5:
    var_1 = -9 * var_5 + -9 * var_0 + -2147483647;
    if (-1 * var_1 + 0 * var_2 + 1 * var_0 + -2147482897 >= 0) goto BB12;
    goto BB6;

BB6:
    var_4 = -287421077 * var_5 + -359320745 * var_3 + -426657811;
    if (298099288 * var_4 + -987421181 * var_2 + -1435725184 * var_1 + -524027101 >= 0) goto BB13;
    goto BB11;

BB7:
    var_2 = -9 * var_1 + -9 * var_7 + -2147483494;
    if (0 * var_2 + 1 * var_7 + 1 * var_4 + -26 >= 0) goto BB13;
    goto BB4;

BB8:
    var_3 = -9 * var_7 + -9 * var_0 + -2147482927;
    if (0 * var_3 + 1 * var_0 + 1 * var_7 + -79 >= 0) goto BB9;
    goto BB14;

BB9:
    var_6 = -9 * var_6 + 0 * var_5 + -2147483648;
    if (1 * var_6 + -9 * var_4 + 0 * var_5 + 2942364 >= 0) goto BB13;
    goto BB3;

BB10:
    var_5 = -9 * var_1 + -9 * var_0 + -238608277;
    if (-9 * var_5 + -9 * var_4 + -9 * var_1 + -2147483105 >= 0) goto BB9;
    goto BB1;

BB11:
    var_1 = -9 * var_6 + 0 * var_5 + -2147483647;
    if (-9 * var_1 + -9 * var_7 + -9 * var_3 + -89 >= 0) goto BB8;
    goto BB12;

BB12:
    var_6 = 0 * var_2 + 1 * var_1 + 1908873516;
    if (-9 * var_6 + 0 * var_7 + 1 * var_5 + -2147483464 >= 0) goto BB11;
    goto BB8;

BB13:
    var_6 = -9 * var_7 + -9 * var_6 + -238609375;
    goto BB3;

BB14:
    var_0 = -9 * var_7 + -9 * var_6 + -2147483647;
    printf("%d,%d,%d,%d,%d,%d,%d,%d", var_0, var_1, var_2, var_3, var_4, var_5, var_6, var_7);
    return 0;

}
