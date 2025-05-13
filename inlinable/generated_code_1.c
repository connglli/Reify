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
    var_9 = -1023 * var_10 + -1023 * var_12 + -2145746593;
    if (0 * var_9 + -1023 * var_4 + -1023 * var_1 + -1023 * var_2 + 679273 >= 0) goto BB1;
    goto BB2;
    BB1:
    var_12 = -1023 * var_5 + -1023 * var_7 + -2147483647;
    if (-1 * var_12 + -1023 * var_4 + -1023 * var_6 + -1023 * var_3 + -2145541992 >= 0) goto BB15;
    goto BB7;
    BB2:
    var_3 = 1896460095 * var_10 + 2060211319 * var_4 + -1361595014;
    if (-291404498 * var_3 + 821070476 * var_10 + -281431512 * var_8 + 670317284 * var_5 + -1511182175 >= 0) goto BB13;
    goto BB7;
    BB3:
    var_8 = 1202571489 * var_1 + -1914099901 * var_2 + -52698073;
    if (2104551327 * var_8 + -1706245416 * var_4 + 1942727067 * var_9 + -1201941091 * var_3 + 87895911 >= 0) goto BB14;
    goto BB18;
    BB4:
    var_10 = -1023 * var_4 + -2 * var_1 + -2147483647;
    if (-1 * var_10 + -962 * var_14 + 0 * var_0 + -1 * var_3 + -10476237 >= 0) goto BB7;
    goto BB22;
    BB5:
    var_12 = -568665723 * var_0 + -1732068582 * var_13 + 41762133;
    if (3806942 * var_12 + -1939781299 * var_13 + 1207611796 * var_11 + -727797789 * var_6 + -2072614496 >= 0) goto BB12;
    goto BB28;
    BB6:
    var_11 = -1204829408 * var_11 + -960853888 * var_1 + 366700741;
    if (146284224 * var_11 + -377633335 * var_6 + 1472178851 * var_9 + 1250818384 * var_0 + -902494302 >= 0) goto BB3;
    goto BB7;
    BB7:
    var_3 = -1023 * var_0 + -1023 * var_4 + 1067342952;
    if (-2 * var_3 + 0 * var_9 + -2 * var_1 + -1023 * var_14 + -13887238 >= 0) goto BB10;
    goto BB19;
    BB8:
    var_7 = 317113204 * var_9 + -503322099 * var_10 + 1746601781;
    if (-1877973709 * var_7 + -1288689039 * var_12 + 653575897 * var_10 + 1355015190 * var_1 + -1711678951 >= 0) goto BB9;
    goto BB29;
    BB9:
    var_12 = 1616735346 * var_4 + 625193924 * var_6 + -2053226346;
    if (-999366029 * var_12 + 762099599 * var_1 + -1384214677 * var_8 + 613568441 * var_14 + -660795673 >= 0) goto BB17;
    goto BB10;
    BB10:
    var_2 = -1 * var_12 + -1023 * var_13 + -2147483647;
    if (-1023 * var_2 + 0 * var_12 + 0 * var_10 + 1 * var_14 + -1178391128 >= 0) goto BB25;
    goto BB2;
    BB11:
    var_1 = -1188272083 * var_1 + -828183881 * var_9 + -1225052929;
    if (-1328273498 * var_1 + -885331026 * var_2 + -1827864209 * var_3 + 1475963806 * var_4 + 1833395853 >= 0) goto BB9;
    goto BB28;
    BB12:
    var_8 = 1669327629 * var_12 + 1585751733 * var_3 + 2046838451;
    if (-1456563782 * var_8 + -719131121 * var_12 + 1917900995 * var_4 + 52149145 * var_14 + 1882579577 >= 0) goto BB21;
    goto BB3;
    BB13:
    var_8 = -458073824 * var_14 + -1771967036 * var_4 + 1488838645;
    if (137638615 * var_8 + -288873701 * var_3 + 415483667 * var_11 + 1330209733 * var_2 + 1822730625 >= 0) goto BB4;
    goto BB16;
    BB14:
    var_14 = -894873274 * var_0 + -910393873 * var_6 + 1267862683;
    if (-1109815822 * var_14 + -32988764 * var_1 + 31832844 * var_7 + 1470324851 * var_5 + 125381925 >= 0) goto BB21;
    goto BB11;
    BB15:
    var_2 = -1023 * var_3 + 0 * var_9 + -2147483647;
    if (-1 * var_2 + -224 * var_14 + -1023 * var_4 + -1023 * var_3 + -2147331174 >= 0) goto BB21;
    goto BB23;
    BB16:
    var_5 = 1402993943 * var_10 + -1776163607 * var_14 + -1857575657;
    if (-2083768194 * var_5 + -112397019 * var_9 + 995907788 * var_11 + -1033961394 * var_12 + -432386335 >= 0) goto BB5;
    goto BB20;
    BB17:
    var_9 = 1211399215 * var_10 + -1974855498 * var_5 + 324364323;
    if (-932459390 * var_9 + 828918745 * var_10 + -440699172 * var_5 + 951822276 * var_7 + 513637239 >= 0) goto BB15;
    goto BB18;
    BB18:
    var_9 = 431517779 * var_2 + 1553878762 * var_5 + 602391199;
    if (1058922977 * var_9 + -929254892 * var_2 + 2016805651 * var_3 + 1776148164 * var_11 + 340201488 >= 0) goto BB8;
    goto BB21;
    BB19:
    var_4 = -1023 * var_4 + -1023 * var_1 + -396364154;
    if (-1023 * var_4 + -1 * var_2 + 0 * var_9 + -1023 * var_0 + -2147483647 >= 0) goto BB4;
    goto BB21;
    BB20:
    var_7 = 1668376883 * var_3 + -2052634657 * var_0 + 423845147;
    if (-842628773 * var_7 + -121806340 * var_13 + -948618839 * var_1 + -14480494 * var_2 + -347562465 >= 0) goto BB2;
    goto BB6;
    BB21:
    var_1 = -1023 * var_4 + -1 * var_2 + -2147483647;
    if (-2 * var_1 + -1023 * var_8 + -1023 * var_5 + -1023 * var_6 + -1056758 >= 0) goto BB4;
    goto BB27;
    BB22:
    var_9 = -1611379473 * var_3 + 846140111 * var_5 + -1640038516;
    if (1595331434 * var_9 + -490710009 * var_13 + -1042609023 * var_11 + 161423642 * var_3 + -569416490 >= 0) goto BB1;
    goto BB24;
    BB23:
    var_13 = -1023 * var_13 + -1023 * var_4 + -1077577051;
    if (0 * var_13 + -1023 * var_2 + -1023 * var_11 + -1023 * var_14 + -1132460 >= 0) goto BB29;
    goto BB4;
    BB24:
    var_8 = 833991688 * var_0 + 1741032142 * var_12 + -985119725;
    if (1031525629 * var_8 + 125583722 * var_11 + 232550744 * var_1 + 1170862078 * var_4 + -1345272845 >= 0) goto BB29;
    goto BB4;
    BB25:
    var_1 = -1 * var_12 + -1023 * var_11 + -2147483647;
    if (-1023 * var_1 + -1023 * var_13 + -1023 * var_6 + -1023 * var_11 + -2147483647 >= 0) goto BB8;
    goto BB15;
    BB26:
    var_8 = 1317924110 * var_10 + 104286159 * var_8 + -2090624501;
    goto BB19;
    BB27:
    var_1 = 1084853807 * var_4 + -2066200826 * var_14 + 762284173;
    if (59522389 * var_1 + -974920808 * var_3 + 1787989757 * var_4 + -446659125 * var_0 + -2068808805 >= 0) goto BB21;
    goto BB9;
    BB28:
    var_9 = -1775934116 * var_8 + 463506795 * var_2 + -1688930396;
    goto BB6;
    BB29:
    var_2 = 0 * var_9 + -1 * var_13 + -2147483647;
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
gotoFunction(-373, 646, -246, -115, 264, -631, 704, -414, -348, -6, 822, -581, 876, 81, -525, 0);
gotoFunction(-373, -998, 999, -115, 264, -996, -996, -999, 1000, -1000, 1000, -581, -999, -866, -525, 1);
gotoFunction(-373, -1000, 998, -115, 264, -1000, -1000, -1000, 998, -998, 997, -585, -1000, -869, -525, 2);
gotoFunction(-373, -997, 999, -115, 264, -999, -997, -998, 998, -998, 999, -583, -1000, -868, -525, 3);
gotoFunction(-373, -998, 1000, -115, 264, -997, -999, -996, 999, -998, 998, -582, -1000, -867, -525, 4);
gotoFunction(-373, -999, 1000, -115, 264, -998, -998, -997, 998, -999, 998, -584, -1000, -868, -525, 5);
    printf("%d", crc32_context);
    return 0;
}
