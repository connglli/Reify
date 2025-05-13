#include <stdio.h>
#include <stdint.h>
const int mod = 1000000007;
static uint32_t crc32_tab[256];
static uint32_t crc32_context = 0xFFFFFFFFUL;
static void crc32_gentab (void)
{	uint32_t crc;	const uint32_t poly = 0xEDB88320UL;	int i, j;
	for (i = 0; i < 256; i++) {		crc = i;		for (j = 8; j > 0; j--) {			if (crc & 1) {				crc = (crc >> 1) ^ poly;			} else {				crc >>= 1;			}		}		crc32_tab[i] = crc;	}
}

static void crc32_byte (uint8_t b) {	crc32_context = ((crc32_context >> 8) & 0x00FFFFFF) ^ crc32_tab[(crc32_context ^ b) & 0xFF];
}

static void crc32_4bytes (uint32_t val)
{	crc32_byte ((val>>0) & 0xff);	crc32_byte ((val>>8) & 0xff);	crc32_byte ((val>>16) & 0xff);	crc32_byte ((val>>24) & 0xff);
}
void computeChecksum(
int var_0, int var_1, int var_2, int var_3, int var_4, int var_5, int var_6, int var_7, int var_8, int var_9, int var_10, int var_11, int var_12, int var_13, int var_14)
{
    crc32_4bytes(var_0);
    crc32_4bytes(var_1);
    crc32_4bytes(var_2);
    crc32_4bytes(var_3);
    crc32_4bytes(var_4);
    crc32_4bytes(var_5);
    crc32_4bytes(var_6);
    crc32_4bytes(var_7);
    crc32_4bytes(var_8);
    crc32_4bytes(var_9);
    crc32_4bytes(var_10);
    crc32_4bytes(var_11);
    crc32_4bytes(var_12);
    crc32_4bytes(var_13);
    crc32_4bytes(var_14);
}
void gotoFunction(int var_0, int var_1, int var_2, int var_3, int var_4, int var_5, int var_6, int var_7, int var_8, int var_9, int var_10, int var_11, int var_12, int var_13, int var_14, int walk_type)
{
    BB0:
    var_12 = -1 * var_10 + -1 * var_2 + -1;
    if (-1 * var_12 + -1 * var_0 + -1 * var_4 + -1 * var_11 + -7 >= 0) goto BB29;
    goto BB2;
    BB1:
    var_4 = -1557831869 * var_7 + 1769516265 * var_6 + -115525981;
    if (1249137758 * var_4 + -1566246569 * var_10 + -1511341813 * var_0 + -542150033 * var_1 + 279957342 >= 0) goto BB10;
    goto BB25;
    BB2:
    var_5 = -1 * var_9 + -1 * var_6 + -1;
    if (-1 * var_5 + -1 * var_8 + -1 * var_12 + -1 * var_0 + -2 >= 0) goto BB4;
    goto BB18;
    BB3:
    var_8 = -1 * var_8 + -1 * var_3 + -4;
    if (-1 * var_8 + -1 * var_3 + -1 * var_4 + -2 * var_1 + 4 >= 0) goto BB28;
    goto BB1;
    BB4:
    var_11 = -2089697333 * var_7 + -863491510 * var_10 + -1569755725;
    if (-1989815867 * var_11 + -2130796066 * var_3 + 1813190886 * var_7 + -2049051927 * var_12 + 1319576738 >= 0) goto BB5;
    goto BB2;
    BB5:
    var_4 = 1616742903 * var_13 + 950922372 * var_6 + -25218729;
    if (1551601252 * var_4 + -215287587 * var_10 + -19847227 * var_2 + 1215631485 * var_5 + 1811791111 >= 0) goto BB11;
    goto BB16;
    BB6:
    var_1 = -1 * var_10 + -2 * var_14 + 1;
    if (-1 * var_1 + -1 * var_14 + -1 * var_6 + -1 * var_4 + 4 >= 0) goto BB26;
    goto BB21;
    BB7:
    var_6 = -572987220 * var_5 + 1032278093 * var_1 + -524321907;
    if (-1358656601 * var_6 + 1024009497 * var_11 + -855135336 * var_9 + 1241859251 * var_14 + 184984598 >= 0) goto BB11;
    goto BB5;
    BB8:
    var_11 = 788003701 * var_0 + 314971012 * var_1 + 2108084126;
    if (-1428616512 * var_11 + -2120661107 * var_6 + 543375266 * var_7 + 1865832327 * var_3 + 343491621 >= 0) goto BB21;
    goto BB22;
    BB9:
    var_2 = -1143202228 * var_12 + 544734727 * var_14 + -2093269235;
    if (1872835333 * var_2 + -1550767218 * var_13 + -730798196 * var_11 + 1619920417 * var_8 + 1845204825 >= 0) goto BB12;
    goto BB8;
    BB10:
    var_13 = 1544229065 * var_3 + 1795148124 * var_7 + 1771794915;
    if (1949025663 * var_13 + -83803237 * var_7 + 2078952552 * var_0 + 1912914500 * var_2 + -228774693 >= 0) goto BB1;
    goto BB29;
    BB11:
    var_0 = -1 * var_7 + -2 * var_3 + -2;
    if (-1 * var_0 + -1 * var_4 + -1 * var_11 + -1 * var_2 + -12 >= 0) goto BB9;
    goto BB25;
    BB12:
    var_3 = -519805467 * var_12 + 663666575 * var_8 + 428118318;
    if (-1488764674 * var_3 + 1636458139 * var_12 + -1527099894 * var_14 + 1612887176 * var_10 + -2140277024 >= 0) goto BB10;
    goto BB13;
    BB13:
    var_6 = -1717288806 * var_14 + -1690397554 * var_7 + 175058819;
    if (966207545 * var_6 + -2111184724 * var_8 + 625557781 * var_4 + -1914485915 * var_5 + 2097212405 >= 0) goto BB6;
    goto BB3;
    BB14:
    var_14 = -2 * var_1 + -7 * var_10 + -1;
    if (-1 * var_14 + -1 * var_2 + -1 * var_9 + -1 * var_4 + 0 >= 0) goto BB6;
    goto BB11;
    BB15:
    var_0 = -1 * var_10 + -1 * var_7 + -1;
    if (-1 * var_0 + -1 * var_9 + -1 * var_13 + -1 * var_8 + 0 >= 0) goto BB14;
    goto BB7;
    BB16:
    var_4 = 454519710 * var_1 + 1985000434 * var_0 + 1624687121;
    if (1077540242 * var_4 + 1760081913 * var_3 + 1145709240 * var_12 + 1534581199 * var_1 + 241994552 >= 0) goto BB28;
    goto BB20;
    BB17:
    var_11 = -1 * var_14 + -1 * var_6 + -1;
    if (-1 * var_11 + -1 * var_4 + -1 * var_8 + -1 * var_6 + 0 >= 0) goto BB6;
    goto BB25;
    BB18:
    var_11 = -2 * var_1 + -1 * var_7 + -1;
    if (-1 * var_11 + -1 * var_6 + -1 * var_13 + -1 * var_9 + -2 >= 0) goto BB19;
    goto BB17;
    BB19:
    var_11 = -1 * var_7 + -1 * var_14 + -1;
    if (-1 * var_11 + -1 * var_6 + -1 * var_12 + -1 * var_9 + 0 >= 0) goto BB3;
    goto BB20;
    BB20:
    var_14 = -1803923193 * var_12 + -110997978 * var_3 + -1538809913;
    if (-2130224495 * var_14 + 1268329945 * var_6 + -1935676041 * var_10 + 1715913847 * var_2 + -1725564850 >= 0) goto BB25;
    goto BB19;
    BB21:
    var_4 = -2 * var_2 + -1 * var_5 + -7;
    if (-2 * var_4 + -1 * var_3 + -1 * var_2 + -1 * var_13 + -21 >= 0) goto BB6;
    goto BB23;
    BB22:
    var_0 = 1347733126 * var_12 + -1450483789 * var_9 + -1756473196;
    if (-844529340 * var_0 + 1826284032 * var_9 + -1867969236 * var_6 + 826952613 * var_7 + -461106628 >= 0) goto BB5;
    goto BB23;
    BB23:
    var_9 = -2 * var_0 + -3 * var_4 + -5;
    if (-1 * var_9 + -1 * var_6 + -1 * var_7 + -1 * var_5 + 0 >= 0) goto BB24;
    goto BB29;
    BB24:
    var_13 = -1 * var_14 + -1 * var_13 + -7;
    if (-1 * var_13 + -1 * var_14 + -1 * var_2 + -1 * var_10 + -11 >= 0) goto BB2;
    goto BB18;
    BB25:
    var_4 = -7 * var_4 + -1 * var_8 + -73;
    if (-2 * var_4 + -1 * var_10 + -1 * var_6 + -1 * var_14 + -76 >= 0) goto BB27;
    goto BB21;
    BB26:
    var_2 = -2 * var_0 + -2 * var_8 + -5;
    if (-5 * var_2 + -1 * var_12 + -2 * var_4 + -4 * var_13 + -39 >= 0) goto BB27;
    goto BB11;
    BB27:
    var_14 = -1 * var_7 + -1 * var_10 + -1;
    if (-1 * var_14 + -1 * var_2 + -1 * var_13 + -1 * var_11 + 0 >= 0) goto BB29;
    goto BB25;
    BB28:
    var_4 = -1 * var_10 + -1 * var_8 + -1;
    goto BB15;
    BB29:
    var_14 = -1 * var_13 + -1 * var_9 + -1;
    computeChecksum(var_0,var_1,var_2,var_3,var_4,var_5,var_6,var_7,var_8,var_9,var_10,var_11,var_12,var_13,var_14);
}
int pass_counter_0 = 0;
int pass_counter_1 = 0;
int pass_counter_2 = 0;
int pass_counter_3 = 0;
int pass_counter_4 = 0;
int pass_counter_5 = 0;
int pass_counter_6 = 0;
int pass_counter_7 = 0;
int pass_counter_8 = 0;
int pass_counter_9 = 0;
int pass_counter_10 = 0;
int pass_counter_11 = 0;
int pass_counter_12 = 0;
int pass_counter_13 = 0;
int pass_counter_14 = 0;
int pass_counter_15 = 0;
int pass_counter_16 = 0;
int pass_counter_17 = 0;
int pass_counter_18 = 0;
int pass_counter_19 = 0;
int pass_counter_20 = 0;
int pass_counter_21 = 0;
int pass_counter_22 = 0;
int pass_counter_23 = 0;
int pass_counter_24 = 0;
int pass_counter_25 = 0;
int pass_counter_26 = 0;
int pass_counter_27 = 0;
int pass_counter_28 = 0;
int pass_counter_29 = 0;
int main() {
crc32_gentab();
gotoFunction(-1, -1, -1, -1, -5, 1, -1, -1, -2, -1, -1, -1, 1, -1, -1, 0);
gotoFunction(786, -999, -1000, -926, 216, -999, 489, 996, -962, 428, -100, 1000, -1000, 114, 803, 1);
gotoFunction(786, -1000, -1000, -923, 218, -1000, 490, 995, -964, 428, -98, 999, -1000, 114, 804, 2);
gotoFunction(786, -999, -1000, -926, 217, -1000, 490, 995, -963, 431, -99, 999, -1000, 115, 803, 3);
    printf("%d", crc32_context);
    return 0;
}
