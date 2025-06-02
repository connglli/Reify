#include <stdio.h>

int var_0 = 1; // Initial value from Z3 model
int var_1 = 1; // Initial value from Z3 model
int var_2 = -2147483647; // Initial value from Z3 model
int var_3 = 1; // Initial value from Z3 model
int var_4 = 1; // Initial value from Z3 model

int main() {
BB0:
    var_0 = -1 * var_2 + 0 * var_3 + -2147483647;
    var_1 = 0 * var_4 + -1 * var_2 + -2147483647;
    var_3 = 0 * var_1 + -2147483647 * var_3 + 2147483647;
    if (0 * var_3 + 0 * var_0 + -2147483647 * var_1 + -2147483647 * var_4 + 0 >= 0) goto BB3;
    goto BB5;

BB1:
    var_0 = -1672117480 * var_2 + 196499277 * var_0 + -51698497;
    var_1 = 639535117 * var_0 + -1065479822 * var_2 + 407239473;
    var_2 = 301141281 * var_3 + -519404511 * var_1 + 19167614;
    if (-1186827482 * var_1 + 1592627415 * var_0 + -898570652 * var_2 + -727922050 * var_3 + -1782691687 >= 0) goto BB10;
    goto BB3;

BB2:
    var_0 = -1356955500 * var_2 + 1849923069 * var_3 + -2125401574;
    var_2 = -1211341831 * var_0 + 11197702 * var_1 + 1654060100;
    var_3 = 1669530156 * var_4 + -1575725277 * var_2 + 1283684983;
    goto BB5;

BB3:
    var_2 = 0 * var_2 + -2147483647 * var_0 + -2147483647;
    var_3 = 0 * var_2 + 1 * var_4 + 1;
    var_4 = 0 * var_0 + -1 * var_3 + -2147483647;
    if (0 * var_2 + 0 * var_3 + -1 * var_4 + 1 * var_0 + 0 >= 0) goto BB6;
    goto BB4;

BB4:
    var_1 = -2147483647 * var_4 + 1 * var_2 + 0;
    var_3 = 1 * var_0 + 0 * var_4 + 1;
    var_4 = 1 * var_2 + -2147483647 * var_4 + 0;
    if (0 * var_1 + 0 * var_3 + 1 * var_4 + -2147483646 * var_0 + 0 >= 0) goto BB7;
    goto BB10;

BB5:
    var_0 = 0 * var_1 + -1 * var_2 + -2147483647;
    var_1 = 0 * var_0 + -2147483646 * var_4 + 2147483646;
    var_3 = -1 * var_2 + 0 * var_0 + -2147483647;
    if (0 * var_3 + 0 * var_1 + 1 * var_0 + -2147483647 * var_4 + 0 >= 0) goto BB8;
    goto BB9;

BB6:
    var_1 = -2147483647 * var_2 + -2147483647 * var_3 + 0;
    var_2 = -1 * var_4 + 0 * var_0 + -2147483647;
    var_4 = 1 * var_4 + 0 * var_2 + 2147483647;
    if (0 * var_4 + 0 * var_2 + 1 * var_1 + 1 * var_0 + 0 >= 0) goto BB3;
    goto BB11;

BB7:
    var_0 = -2147483647 * var_1 + 0 * var_4 + 1;
    var_1 = -2147483647 * var_4 + 0 * var_1 + -1;
    var_4 = 1 * var_2 + 1 * var_3 + 0;
    goto BB10;

BB8:
    var_1 = 360938585 * var_3 + -688364184 * var_2 + 814656631;
    var_2 = 1098638505 * var_3 + -242590307 * var_2 + -1366557520;
    var_4 = 785789401 * var_1 + 1695896027 * var_4 + -566323391;
    goto BB1;

BB9:
    var_2 = -2147483647 * var_1 + 1 * var_0 + 0;
    var_3 = -2147483646 * var_4 + 0 * var_0 + 2147483646;
    var_4 = 1 * var_3 + -2147483647 * var_0 + 0;
    goto BB10;

BB10:
    var_0 = 1 * var_2 + 0 * var_4 + -1;
    var_3 = -2147483647 * var_3 + 0 * var_1 + 1;
    var_4 = -2147483647 * var_1 + 0 * var_2 + -2147483648;
    goto BB3;

BB11:
    var_1 = 1 * var_2 + 0 * var_3 + -2147483647;
    var_3 = 0 * var_0 + -2147483647 * var_4 + -2147483647;
    var_4 = 0 * var_2 + -2147483647 * var_4 + 1;
    printf("%d,%d,%d,%d,%d", var_0, var_1, var_2, var_3, var_4);
    return 0;

}
