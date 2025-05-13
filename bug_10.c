#include <stdio.h>

int var_0 = -2147483647; // Initial value from Z3 model
int var_1 = -2147483647; // Initial value from Z3 model
int var_2 = -2147483647; // Initial value from Z3 model
int var_3 = 1; // Initial value from Z3 model
int var_4 = 1; // Initial value from Z3 model

int main() {
BB0:
    var_3 = 0 * var_2 + -2147483645 * var_4 + -1;
    var_4 = -1 * var_2 + 0 * var_3 + -2147483647;
    goto BB7;

BB1:
    var_0 = 2121065816 * var_3 + 1066565245 * var_2 + 79202870;
    var_4 = -1435522984 * var_0 + -1357423562 * var_3 + 1929103697;
    if (-1140457343 * var_4 + -328205372 * var_0 + 15056481 * var_2 + -706942427 >= 0) goto BB4;
    goto BB2;

BB2:
    var_0 = 198921711 * var_3 + 2111676761 * var_4 + 1612525909;
    var_1 = -1321897717 * var_3 + -701762368 * var_0 + 947486047;
    goto BB8;

BB3:
    var_3 = -1990267243 * var_1 + -78518659 * var_0 + 1400461868;
    var_4 = -571891845 * var_4 + -124946720 * var_0 + -1661970919;
    goto BB4;

BB4:
    var_0 = -1108785650 * var_4 + 1502219786 * var_0 + 199223334;
    var_3 = 814788700 * var_2 + -1929673744 * var_1 + 181736878;
    goto BB1;

BB5:
    var_1 = 399222902 * var_3 + 1833455991 * var_2 + -1616548445;
    var_3 = 953114657 * var_3 + 4337702 * var_2 + 177201201;
    if (-841949742 * var_3 + -1647327270 * var_1 + -247576187 * var_2 + 1075394118 >= 0) goto BB4;
    goto BB9;

BB6:
    var_2 = 1798847160 * var_3 + 240329485 * var_2 + 502914122;
    var_3 = 88039823 * var_2 + 1439305793 * var_0 + 1993642752;
    if (-344377236 * var_3 + -1990316873 * var_2 + 2022872203 * var_4 + -1239086501 >= 0) goto BB8;
    goto BB1;

BB7:
    var_1 = 0 * var_1 + 1 * var_3 + 2147483646;
    var_3 = 0 * var_0 + -1 * var_2 + -2147483647;
    if (0 * var_3 + -715827882 * var_1 + -1 * var_2 + -715827882 >= 0) goto BB8;
    goto BB9;

BB8:
    var_0 = -1 * var_2 + 0 * var_1 + -2147483647;
    var_3 = 1 * var_1 + 1 * var_2 + 2;
    goto BB7;

BB9:
    var_0 = 0 * var_0 + -715827882 * var_1 + -1;
    var_3 = 0 * var_1 + 1 * var_4 + 1;
    printf("%d,%d,%d,%d,%d", var_0, var_1, var_2, var_3, var_4);
    return 0;

}
