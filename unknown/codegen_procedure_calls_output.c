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
static uint32_t context_free_crc32_byte(uint32_t context, uint8_t b) {	return ((context >> 8) & 0x00FFFFFF) ^ crc32_tab[(context ^ b) & 0xFF];
}

static uint32_t context_free_crc32_4bytes(uint32_t context, uint32_t val) {	context = context_free_crc32_byte(context, (val >> 0) & 0xFF);	context = context_free_crc32_byte(context, (val >> 8) & 0xFF);	context = context_free_crc32_byte(context, (val >> 16) & 0xFF);	context = context_free_crc32_byte(context, (val >> 24) & 0xFF);	return context;
}void computeChecksum(
int var_0, int var_1, int var_2, int var_3, int var_4, int var_5, int var_6, int var_7)
{
    crc32_4bytes(var_0);
    crc32_4bytes(var_1);
    crc32_4bytes(var_2);
    crc32_4bytes(var_3);
    crc32_4bytes(var_4);
    crc32_4bytes(var_5);
    crc32_4bytes(var_6);
    crc32_4bytes(var_7);
}
int computeContextFreeChecksum(
int var_0, int var_1, int var_2, int var_3, int var_4, int var_5, int var_6, int var_7)
{
 long running_sum = 0;
    running_sum = (running_sum + var_0) % mod;
    running_sum = (running_sum + var_1) % mod;
    running_sum = (running_sum + var_2) % mod;
    running_sum = (running_sum + var_3) % mod;
    running_sum = (running_sum + var_4) % mod;
    running_sum = (running_sum + var_5) % mod;
    running_sum = (running_sum + var_6) % mod;
    running_sum = (running_sum + var_7) % mod;
    running_sum = (running_sum + mod) % mod;
    return running_sum;
}
int proc_0(int var_0_walk_0, int var_1_walk_0, int var_2_walk_0, int var_3_walk_0, int var_4_walk_0, int var_5_walk_0, int var_6_walk_0, int var_7_walk_0);
int proc_1(int var_0_walk_0, int var_1_walk_0, int var_2_walk_0, int var_3_walk_0, int var_4_walk_0, int var_5_walk_0, int var_6_walk_0, int var_7_walk_0);
int proc_0(int var_0, int var_1, int var_2, int var_3, int var_4, int var_5, int var_6, int var_7){
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
BB0:
    var_1 = -1 * var_4 + 1 * var_7 + -1073741824;
    goto BB9;
BB1:
    var_3 = 79848666 * var_2 + -1809357915 * var_0 + -470550735;
    goto BB8;
BB2:
    var_1 = 409763103 * var_6 + 245433873 * var_7 + 643362096;
    if (1361163352 * var_1 + -1623034934 * var_2 + 979645116 * var_6 + -2085791564 * var_4 + -1688656108 >= 0) goto BB3;
    goto BB4;
BB3:
    var_6 = -87206074 * var_2 + 128875092 * var_6 + 685566787;
    if (258460508 * var_6 + 1385006029 * var_0 + 508153160 * var_4 + -218570622 * var_1 + -161419162 >= 0) goto BB4;
    goto BB2;
BB4:
    var_3 = 1064351605 * var_6 + -598681452 * var_4 + -1892814467;
    goto BB8;
BB5:
    var_6 = 942163647 * var_6 + -2089139792 * var_7 + 1783754468;
    if (7012376 * var_6 + -1394920946 * var_7 + -1042108477 * var_5 + 797755652 * var_0 + -1451278733 >= 0) goto BB4;
    goto BB1;
BB6:
    var_3 = 1952304511 * var_3 + 1051909148 * var_7 + -1059279127;
    if (316929169 * var_3 + 826495037 * var_4 + 607361877 * var_7 + 1248585099 * var_5 + -852877165 >= 0) goto BB2;
    goto BB5;
BB7:
    var_1 = -458508786 * var_2 + 445400581 * var_0 + 377038464;
    if (-1195893000 * var_1 + 1334571058 * var_0 + -519425173 * var_3 + -1962809272 * var_4 + 304742689 >= 0) goto BB1;
    goto BB8;
BB8:
    var_0 = -689269633 * var_4 + 317406029 * var_6 + -1662353441;
    if (-619146971 * var_0 + -1965247495 * var_6 + 2138723656 * var_2 + 44279852 * var_4 + 322357190 >= 0) goto BB9;
    goto BB4;
BB9:
    var_1 = 1 * var_2 + 0 * var_7 + 1073741824;
    computeChecksum(var_0,var_1,var_2,var_3,var_4,var_5,var_6,var_7);
    return computeContextFreeChecksum(var_0,var_1,var_2,var_3,var_4,var_5,var_6,var_7);
}
int proc_1(int var_0, int var_1, int var_2, int var_3, int var_4, int var_5, int var_6, int var_7){
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
BB0:
    var_3 = (proc_0(-2147483648, -2147483648, -2147483648, -2147483648, -2147483583, -2147483648, -2147483648, -2147483648) + -73741817) * var_4 + (proc_0(-2147483648, -2147483648, -2147483648, -2147483648, -2147483583, -2147483648, -2147483648, -2147483648) + -73741816) * var_3 + -1;
    goto BB9;
BB1:
    var_0 = proc_1(1400112355,-84655644,703698002,478205216,464739819,-1633760701,-759480379,-275189783) * var_2 + proc_1(17143091,375383921,1494082259,-1722754970,273162470,1066769022,2046656846,-880163319) * var_4 + -1967216021;
    goto BB8;
BB2:
    var_0 = -796175294 * var_4 + 1096592422 * var_5 + -517704314;
    if (516185762 * var_0 + -315873847 * var_2 + 1579669815 * var_7 + -2105314179 * var_1 + -2098759013 >= 0) goto BB4;
    goto BB3;
BB3:
    var_4 = -2036530797 * var_7 + proc_0(1885504338,-142011999,159662507,-224312500,1088009499,-2080252834,-469226631,-820327887) * var_6 + -1720387204;
    if (2106541800 * var_4 + -645350239 * var_5 + 1154398345 * var_6 + 542308518 * var_2 + -1559640787 >= 0) goto BB2;
    goto BB4;
BB4:
    var_2 = -367687581 * var_5 + proc_0(-1117118953,-1055570611,957080261,1049075040,-1385737299,2137983017,283120379,-264292344) * var_4 + 1216833783;
    goto BB8;
BB5:
    var_1 = proc_0(-1286935472,-223751187,424603306,723527143,2130909557,122939563,608472889,1080989579) * var_7 + 285522757 * var_3 + -1236045064;
    if (-546234264 * var_1 + 326960829 * var_6 + 1444436788 * var_7 + 762694694 * var_0 + 1717907438 >= 0) goto BB4;
    goto BB1;
BB6:
    var_0 = 1625096085 * var_0 + -436470158 * var_6 + 1332847004;
    if (1934170586 * var_0 + -195476188 * var_5 + 641022123 * var_7 + 18817631 * var_6 + 2004078382 >= 0) goto BB5;
    goto BB2;
BB7:
    var_0 = proc_0(1138730993,1549121464,480316875,654878321,1424285290,1962745658,-1547154278,-1805139611) * var_5 + proc_0(919334765,1893178229,-355828529,1293231196,-519708985,-6382080,1459285035,-2013550698) * var_7 + 1789794426;
    if (1603320531 * var_0 + 1440627563 * var_6 + -1444352052 * var_3 + 1031599537 * var_7 + 63436009 >= 0) goto BB8;
    goto BB1;
BB8:
    var_5 = 1316862154 * var_7 + 215647151 * var_6 + -1029023154;
    if (42091409 * var_5 + 113170614 * var_1 + -232552281 * var_3 + 82373305 * var_7 + 1370573294 >= 0) goto BB4;
    goto BB9;
BB9:
    var_1 = (proc_0(0, 0, 0, 0, 0, 0, 0, 0) + -73741816) * var_1 + (proc_0(0, 0, 0, 0, 0, 0, 0, 0) + -73741817) * var_2 + 1073741824;
    computeChecksum(var_0,var_1,var_2,var_3,var_4,var_5,var_6,var_7);
    return computeContextFreeChecksum(var_0,var_1,var_2,var_3,var_4,var_5,var_6,var_7);
}
int main() {
crc32_gentab();
proc_0(-2147483648, -2147483648, -2147483648, -2147483648, -2147483583, -2147483648, -2147483648, -2147483648);
proc_0(0, 0, 0, 0, 0, 0, 0, 0);
proc_1(-2147483648, -2147483648, -2147483648, -2147483647, -2147483648, -2147483648, -2147483648, -2147483648);
proc_1(0, 0, 0, 0, 0, 0, 0, 0);
    printf("%d", crc32_context);
    return 0;
}
