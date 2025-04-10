#include <stdio.h>

int var_0 = -1; // Initial value from Z3 model
int var_1 = 1; // Initial value from Z3 model
int var_2 = -1; // Initial value from Z3 model
int var_3 = -1; // Initial value from Z3 model
int var_4 = -2; // Initial value from Z3 model
int var_5 = -1; // Initial value from Z3 model
int var_6 = -1; // Initial value from Z3 model
int var_7 = -1; // Initial value from Z3 model

int main() {
BB0:
    var_1 = -1 * var_5 + -1 * var_4 + -1;
    var_2 = -1 * var_0 + -1 * var_2 + -1;
    if (-1 * var_2 + -1 * var_1 + 2 >= 0) goto BB3;
    goto BB6;

BB1:
    var_1 = 1288512947 * var_3 + 812330639 * var_5 + 311190640;
    var_2 = -216631854 * var_3 + -433396697 * var_6 + -1267094467;
    goto BB8;

BB2:
    var_1 = -1222317695 * var_2 + -1812565906 * var_0 + -1925577460;
    var_3 = 1863107771 * var_0 + -1486833767 * var_5 + 280327008;
    if (261729571 * var_3 + -1020814840 * var_1 + 2146218320 >= 0) goto BB3;
    goto BB8;

BB3:
    var_3 = -149169345 * var_6 + -411752780 * var_1 + 1252119119;
    var_7 = -1191875206 * var_0 + 2008785872 * var_4 + -1782143690;
    if (-857842508 * var_7 + 53598546 * var_3 + 979317251 >= 0) goto BB2;
    goto BB9;

BB4:
    var_0 = -788521846 * var_6 + 1888083782 * var_7 + 291887181;
    var_1 = -811502876 * var_7 + 833181156 * var_5 + 1914163382;
    goto BB3;

BB5:
    var_2 = -1 * var_3 + -1 * var_7 + -3;
    var_3 = -1 * var_2 + -1 * var_5 + -1;
    if (-1 * var_3 + -1 * var_2 + -2 >= 0) goto BB7;
    goto BB9;

BB6:
    var_0 = -1 * var_7 + -1 * var_6 + -1;
    var_5 = 1 * var_3 + -1 * var_4 + -1;
    goto BB5;

BB7:
    var_2 = -231140456 * var_3 + -580283520 * var_0 + -664028727;
    var_5 = 621757149 * var_3 + -1292216997 * var_4 + -1110548500;
    if (247673712 * var_5 + 496491431 * var_2 + -1969579048 >= 0) goto BB5;
    goto BB3;

BB8:
    var_2 = -1413949594 * var_6 + 799981166 * var_0 + -659306208;
    var_6 = 1865361627 * var_0 + 28312361 * var_4 + -420029100;
    goto BB7;

BB9:
    var_1 = -1 * var_0 + -1 * var_5 + -1;
    var_7 = -1 * var_0 + -1 * var_5 + -1;
    printf("%d,%d,%d,%d,%d,%d,%d,%d", var_0, var_1, var_2, var_3, var_4, var_5, var_6, var_7);
    return 0;

}
