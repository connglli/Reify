#include <stdio.h>

int var_0 = 618936046; // Initial value from Z3 model
int var_1 = 1673121363; // Initial value from Z3 model
int var_2 = -1823985145; // Initial value from Z3 model
int var_3 = -101975509; // Initial value from Z3 model
int var_4 = 550842615; // Initial value from Z3 model
int var_5 = -847764487; // Initial value from Z3 model
int var_6 = -1915015669; // Initial value from Z3 model
int var_7 = 339534863; // Initial value from Z3 model

int pass_counter = 0;
int main() {
BB0:
    var_6 = 1 * var_6 + 1 * var_4 + -783310593;
    if (-1 * var_6 + -1 * var_1 + -1 * var_0 + -21 * var_3 + 150571720 >= 0) goto BB5;
    goto BB7;

BB1:
    var_3 = -1 * var_0 + -618936048 * var_6 + -1;
    if (-2147483648 * var_3 + -1 * var_2 + -32822993 * var_6 + -3 * var_0 + 0 >= 0) goto BB8;
    goto BB9;

BB2:
    var_7 = -1458665191 * var_4 + 724833588 * var_5 + 1993375084;
    goto BB7;

BB3:
    var_7 = -1251303505 * var_5 + 1103296649 * var_6 + -490224433;
    if (840294780 * var_7 + -1646424203 * var_3 + -1481128793 * var_6 + 473208148 * var_0 + 2086034078 >= 0) goto BB2;
    goto BB8;

BB4:
    var_1 = -3 * var_4 + -1 * var_2 + -1;
    if (-1 * var_1 + -1533309634 * var_6 + -3 * var_0 + -3 * var_4 + 2147483647 >= 0) goto BB8;
    goto BB6;

BB5:
    var_5 = -1 * var_2 + -1 * var_0 + -1205049098;
    if (-1 * var_5 + -1 * var_4 + -1 * var_2 + -1 * var_0 + -654206484 >= 0) goto BB6;
    goto BB7;

BB6:
    var_1 = -2147483647 * var_6 + -1 * var_0 + -1528547601;
    if (-2147483648 * var_1 + -2147483647 * var_6 + -1 * var_7 + -2147483648 * var_5 + 339534863 >= 0) goto BB8;
    goto BB1;

BB7:
    var_6 = -1 * var_7 + -1 * var_4 + 890377477;
    goto BB4;

BB8:
    var_6 = 1595594684 * var_7 + -638812651 * var_3 + 2065358213;
    goto BB5;

BB9:
    var_1 = -6 * var_7 + -2147483648 * var_1 + -110274470;
    printf("%d,%d,%d,%d,%d,%d,%d,%d", var_0, var_1, var_2, var_3, var_4, var_5, var_6, var_7);
    return 0;

}
