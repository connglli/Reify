#include <stdio.h>

int var_0 = -2147483647; // Initial value from Z3 model
int var_1 = -2147483647; // Initial value from Z3 model
int var_2 = -2147483647; // Initial value from Z3 model
int var_3 = -2147483647; // Initial value from Z3 model
int var_4 = 0; // Initial value from Z3 model
int var_5 = -2147483647; // Initial value from Z3 model
int var_6 = 1; // Initial value from Z3 model
int var_7 = -1; // Initial value from Z3 model
int var_8 = 1; // Initial value from Z3 model
int var_9 = 1; // Initial value from Z3 model

int main() {
BB0:
    var_3 = 0 * var_3 + -1 * var_1 + -2147483647;
    if (-2147483647 * var_3 + 0 * var_5 + -2147483647 * var_9 + 0 >= 0) goto BB12;
    goto BB4;

BB1:
    var_7 = -2147483647 * var_7 + 1 * var_0 + 0;
    if (0 * var_7 + 1 * var_5 + -2147483647 * var_3 + -2 >= 0) goto BB2;
    goto BB4;

BB2:
    var_0 = 2139314931 * var_4 + -1987028133 * var_5 + -2118961976;
    if (495472069 * var_0 + 1814615977 * var_6 + 1353220761 * var_7 + -65909537 >= 0) goto BB11;
    goto BB9;

BB3:
    var_2 = 1586287289 * var_6 + 1515421347 * var_5 + 2008274611;
    goto BB10;

BB4:
    var_9 = 0 * var_6 + 1 * var_7 + -1;
    if (1 * var_9 + 2147483646 * var_8 + 1 * var_5 + 2 >= 0) goto BB6;
    goto BB12;

BB5:
    var_8 = -664075845 * var_1 + -2036622591 * var_3 + -2042221754;
    goto BB1;

BB6:
    var_2 = -2147483647 * var_6 + -2147483647 * var_3 + 0;
    if (0 * var_2 + 1 * var_3 + -2147483647 * var_6 + 0 >= 0) goto BB13;
    goto BB1;

BB7:
    var_9 = 1880641922 * var_8 + 1683764778 * var_4 + 1037878865;
    if (-633617596 * var_9 + 395051776 * var_4 + 2002303294 * var_0 + -813050872 >= 0) goto BB6;
    goto BB8;

BB8:
    var_3 = 1715382624 * var_9 + -1880022535 * var_4 + -365929290;
    if (-802501352 * var_3 + 1631488244 * var_0 + 1540786364 * var_5 + -1542761984 >= 0) goto BB14;
    goto BB1;

BB9:
    var_9 = 707240099 * var_5 + -899795386 * var_8 + -677837678;
    if (-566695975 * var_9 + 887896671 * var_4 + -1726307826 * var_2 + -944819955 >= 0) goto BB4;
    goto BB13;

BB10:
    var_4 = -2147483647 * var_4 + -1 * var_5 + -2147483646;
    if (0 * var_4 + 1 * var_6 + 1 * var_9 + 2 >= 0) goto BB4;
    goto BB14;

BB11:
    var_2 = 862494391 * var_8 + -1409334672 * var_1 + 1059462557;
    if (-36192996 * var_2 + 163134258 * var_7 + -1193215874 * var_8 + -192650320 >= 0) goto BB10;
    goto BB7;

BB12:
    var_5 = 0 * var_2 + -1 * var_7 + -2147483647;
    goto BB10;

BB13:
    var_1 = -1625685337 * var_2 + -556294369 * var_7 + 1351222609;
    goto BB6;

BB14:
    var_5 = -2147483647 * var_6 + -1 * var_0 + -2147483647;
    printf("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d", var_0, var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8, var_9);
    return 0;

}
