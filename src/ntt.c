#include "ntt.h"

#include <stdint.h>
#include <stdio.h>

#include "api.h"
#include "reduce.h"

// zeta^{br(1,2,3...)}*RmodM
const int32_t root_table[] = {
    846038,   370173,   -2016489, -1216365, 1843388,  -677362,  -1072953,
    -273335,  571552,   355329,   -1953862, 1203721,  1720831,  965995,
    641414,   1406204,  -869335,  -603157,  348730,   2063531,  -1328182,
    1381183,  -1069471, 184133,   1465601,  -53304,   -1356023, 857228,
    59677,    675693,   -1598775, -136014,  966523,   959523,   846643,
    -86562,   -489847,  136654,   -2088895, 17941,    -1051723, -1316589,
    1814059,  -230501,  1626667,  -1171284, 2085817,  1830521,  -1951522,
    445122,   -1689285, -1551600, -2055310, -1064338, -368446,  535845,
    361370,   676319,   -541241,  1009639,  538875,   -2102677, 1585701,
    1821376,  -1290490, 962214,   -8714,    1546898,  -580673,  -1282351,
    -245053,  -693353,  1553156,  122604,   -1894192, -1170731, 90910,
    1953642,  -835944,  733787,   -494715,  795067,   1382960,  -1495256,
    -1824985, -298978,  1094234,  -302935,  2014429,  -339341,  -2007511,
    -592962,  211939,   -1404409, 744185,   -2022369, -1623695, -139534,
    -1271569, -2060697, 1036874,  1382006,  -646550,  1019883,  1183551,
    52974,    1671898,  -194860,  1601807,  -1589407, 1189812,  -1170767,
    890018,   -280701,  -1808405, 99391,    -638903,  893295,   391700,
    422539,   -1801679, 742283,   166402,   -789669,  -177835,  -222557,
    522210,   1697580,  -39500,   -1675992, -1125921, 1161075,  230880,
    -1259576, 1212857,  476606,   -1494601, -2082167, 50282,    -856554,
    1334236,  -1600445, 413060,   -196179,  -895174,  307664,   22223,
    20225,    1987140,  254428,   -506799,  1232040,  2077610,  1009712,
    -614253,  975552,   -1688398, 139283,   -1100725, 1512274,  2102800,
    223819,   1838169,  -802974,  -632523,  1447210,  -1760267, -596739,
    1342163,  -1363757, -2101846, -321523,  1072908,  -1679204, -855117,
    2053832,  882669,   1859699,  -1970930, 1904476,  1748524,  -1662524,
    383438,   902383,   -405934,  -1539528, 535413,   1609871,  2034135,
    1219933,  731083,   1164150,  2066555,  -711442,  -184610,  -43679,
    1346735,  1300766,  -113408,  731747,   -71331,   349367,   -1854479,
    -1408803, 617098,   1560876,  822840,   -11234,   302949,   1586236,
    507324,   -1869963, 835632,   -1475909, 492808,   -1821321, 536916,
    -1883,    -1192473, 7244,     -1453951, 1005437,  1757304,  -2031374,
    -1733878, 2057388,  -1883916, -741068,  -1902883, 459121,   649938,
    -1381577, -1866872, 1407025,  -1336590, -552932,  -1911274, -1729474,
    -1801797, 1684042,  939533,   -98362,   227434,   2011049,  1499197,
    1294670,  -777851,  1966206,  2048487,  1846352,  -91469,   1243355,
    -23332,   1296571,  15068};

const int32_t root_table_merged[] = {
    846038,   370173,   -2016489, -1216365, 1843388,  -677362,  -1072953,
    -273335,  571552,   355329,   -1953862, 1203721,  1720831,  965995,
    641414,   1406204,  -136014,  966523,   1821376,  -1290490, 962214,
    -8714,    1697580,  -39500,   -1675992, -1125921, 1161075,  230880,
    -1259576, 1212857,  -869335,  959523,   846643,   1546898,  -580673,
    -1282351, -245053,  476606,   -1494601, -2082167, 50282,    -856554,
    1334236,  -1600445, 413060,   -603157,  -86562,   -489847,  -693353,
    1553156,  122604,   -1894192, -196179,  -895174,  307664,   22223,
    20225,    1987140,  254428,   -506799,  348730,   136654,   -2088895,
    -1170731, 90910,    1953642,  -835944,  1232040,  2077610,  1009712,
    -614253,  975552,   -1688398, 139283,   -1100725, 2063531,  17941,
    -1051723, 733787,   -494715,  795067,   1382960,  1512274,  2102800,
    223819,   1838169,  -802974,  -632523,  1447210,  -1760267, -1328182,
    -1316589, 1814059,  -1495256, -1824985, -298978,  1094234,  -596739,
    1342163,  -1363757, -2101846, -321523,  1072908,  -1679204, -855117,
    1381183,  -230501,  1626667,  -302935,  2014429,  -339341,  -2007511,
    2053832,  882669,   1859699,  -1970930, 1904476,  1748524,  -1662524,
    383438,   -1069471, -1171284, 2085817,  -592962,  211939,   -1404409,
    744185,   902383,   -405934,  -1539528, 535413,   1609871,  2034135,
    1219933,  731083,   184133,   1830521,  -1951522, -2022369, -1623695,
    -139534,  -1271569, 1164150,  2066555,  -711442,  -184610,  -43679,
    1346735,  1300766,  -113408,  1465601,  445122,   -1689285, -2060697,
    1036874,  1382006,  -646550,  731747,   -71331,   349367,   -1854479,
    -1408803, 617098,   1560876,  822840,   -53304,   -1551600, -2055310,
    1019883,  1183551,  52974,    1671898,  -11234,   302949,   1586236,
    507324,   -1869963, 835632,   -1475909, 492808,   -1356023, -1064338,
    -368446,  -194860,  1601807,  -1589407, 1189812,  -1821321, 536916,
    -1883,    -1192473, 7244,     -1453951, 1005437,  1757304,  857228,
    535845,   361370,   -1170767, 890018,   -280701,  -1808405, -2031374,
    -1733878, 2057388,  -1883916, -741068,  -1902883, 459121,   649938,
    59677,    676319,   -541241,  99391,    -638903,  893295,   391700,
    -1381577, -1866872, 1407025,  -1336590, -552932,  -1911274, -1729474,
    -1801797, 675693,   1009639,  538875,   422539,   -1801679, 742283,
    166402,   1684042,  939533,   -98362,   227434,   2011049,  1499197,
    1294670,  -777851,  -1598775, -2102677, 1585701,  -789669,  -177835,
    -222557,  522210,   1966206,  2048487,  1846352,  -91469,   1243355,
    -23332,   1296571,  15068};
// zeta^{-i} in intt, montgomery field
const int32_t inv_root_table[] = {
    -15068,   -1296571, 23332,    -1243355, 91469,    -1846352, -2048487,
    -1966206, 777851,   -1294670, -1499197, -2011049, -227434,  98362,
    -939533,  -1684042, 1801797,  1729474,  1911274,  552932,   1336590,
    -1407025, 1866872,  1381577,  -649938,  -459121,  1902883,  741068,
    1883916,  -2057388, 1733878,  2031374,  -1757304, -1005437, 1453951,
    -7244,    1192473,  1883,     -536916,  1821321,  -492808,  1475909,
    -835632,  1869963,  -507324,  -1586236, -302949,  11234,    -822840,
    -1560876, -617098,  1408803,  1854479,  -349367,  71331,    -731747,
    113408,   -1300766, -1346735, 43679,    184610,   711442,   -2066555,
    -1164150, -731083,  -1219933, -2034135, -1609871, -535413,  1539528,
    405934,   -902383,  -383438,  1662524,  -1748524, -1904476, 1970930,
    -1859699, -882669,  -2053832, 855117,   1679204,  -1072908, 321523,
    2101846,  1363757,  -1342163, 596739,   1760267,  -1447210, 632523,
    802974,   -1838169, -223819,  -2102800, -1512274, 1100725,  -139283,
    1688398,  -975552,  614253,   -1009712, -2077610, -1232040, 506799,
    -254428,  -1987140, -20225,   -22223,   -307664,  895174,   196179,
    -413060,  1600445,  -1334236, 856554,   -50282,   2082167,  1494601,
    -476606,  -1212857, 1259576,  -230880,  -1161075, 1125921,  1675992,
    39500,    -1697580, -522210,  222557,   177835,   789669,   -166402,
    -742283,  1801679,  -422539,  -391700,  -893295,  638903,   -99391,
    1808405,  280701,   -890018,  1170767,  -1189812, 1589407,  -1601807,
    194860,   -1671898, -52974,   -1183551, -1019883, 646550,   -1382006,
    -1036874, 2060697,  1271569,  139534,   1623695,  2022369,  -744185,
    1404409,  -211939,  592962,   2007511,  339341,   -2014429, 302935,
    -1094234, 298978,   1824985,  1495256,  -1382960, -795067,  494715,
    -733787,  835944,   -1953642, -90910,   1170731,  1894192,  -122604,
    -1553156, 693353,   245053,   1282351,  580673,   -1546898, 8714,
    -962214,  1290490,  -1821376, -1585701, 2102677,  -538875,  -1009639,
    541241,   -676319,  -361370,  -535845,  368446,   1064338,  2055310,
    1551600,  1689285,  -445122,  1951522,  -1830521, -2085817, 1171284,
    -1626667, 230501,   -1814059, 1316589,  1051723,  -17941,   2088895,
    -136654,  489847,   86562,    -846643,  -959523,  -966523,  136014,
    1598775,  -675693,  -59677,   -857228,  1356023,  53304,    -1465601,
    -184133,  1069471,  -1381183, 1328182,  -2063531, -348730,  603157,
    869335,   -1406204, -641414,  -965995,  -1720831, -1203721, 1953862,
    -355329,  -571552,  273335,   1072953,  677362,   -1843388, 1216365,
    2016489,  -370173,  -846038};

const int32_t inv_root_table_merged[] = {
    -15068,   -1296571, 23332,    -1243355, 91469,    -1846352, -2048487,
    -1966206, -522210,  222557,   177835,   789669,   -1585701, 2102677,
    1598775,  777851,   -1294670, -1499197, -2011049, -227434,  98362,
    -939533,  -1684042, -166402,  -742283,  1801679,  -422539,  -538875,
    -1009639, -675693,  1801797,  1729474,  1911274,  552932,   1336590,
    -1407025, 1866872,  1381577,  -391700,  -893295,  638903,   -99391,
    541241,   -676319,  -59677,   -649938,  -459121,  1902883,  741068,
    1883916,  -2057388, 1733878,  2031374,  1808405,  280701,   -890018,
    1170767,  -361370,  -535845,  -857228,  -1757304, -1005437, 1453951,
    -7244,    1192473,  1883,     -536916,  1821321,  -1189812, 1589407,
    -1601807, 194860,   368446,   1064338,  1356023,  -492808,  1475909,
    -835632,  1869963,  -507324,  -1586236, -302949,  11234,    -1671898,
    -52974,   -1183551, -1019883, 2055310,  1551600,  53304,    -822840,
    -1560876, -617098,  1408803,  1854479,  -349367,  71331,    -731747,
    646550,   -1382006, -1036874, 2060697,  1689285,  -445122,  -1465601,
    113408,   -1300766, -1346735, 43679,    184610,   711442,   -2066555,
    -1164150, 1271569,  139534,   1623695,  2022369,  1951522,  -1830521,
    -184133,  -731083,  -1219933, -2034135, -1609871, -535413,  1539528,
    405934,   -902383,  -744185,  1404409,  -211939,  592962,   -2085817,
    1171284,  1069471,  -383438,  1662524,  -1748524, -1904476, 1970930,
    -1859699, -882669,  -2053832, 2007511,  339341,   -2014429, 302935,
    -1626667, 230501,   -1381183, 855117,   1679204,  -1072908, 321523,
    2101846,  1363757,  -1342163, 596739,   -1094234, 298978,   1824985,
    1495256,  -1814059, 1316589,  1328182,  1760267,  -1447210, 632523,
    802974,   -1838169, -223819,  -2102800, -1512274, -1382960, -795067,
    494715,   -733787,  1051723,  -17941,   -2063531, 1100725,  -139283,
    1688398,  -975552,  614253,   -1009712, -2077610, -1232040, 835944,
    -1953642, -90910,   1170731,  2088895,  -136654,  -348730,  506799,
    -254428,  -1987140, -20225,   -22223,   -307664,  895174,   196179,
    1894192,  -122604,  -1553156, 693353,   489847,   86562,    603157,
    -413060,  1600445,  -1334236, 856554,   -50282,   2082167,  1494601,
    -476606,  245053,   1282351,  580673,   -1546898, -846643,  -959523,
    869335,   -1212857, 1259576,  -230880,  -1161075, 1125921,  1675992,
    39500,    -1697580, 8714,     -962214,  1290490,  -1821376, -966523,
    136014,   -1406204, -641414,  -965995,  -1720831, -1203721, 1953862,
    -355329,  -571552,  273335,   1072953,  677362,   -1843388, 1216365,
    2016489,  -370173,  -846038};
/**
 * Name: fqmul
 *
 * Description: Finite field mod q multiplication
 *
 */
int32_t fqmul(int32_t a, int32_t b)
{
    return montgomery_reduce((int64_t)a * b);
}

/*************************************************
 * Name:        ntt
 *
 * Description: Number-theoretic transform (NTT).
 *              input is in standard order, output is in bitreversed order
 *
 * Arguments:   - int32_t in/out[256]: pointer to input/output polynomial
 **************************************************/
void ntt(const int16_t in[256], int32_t out[256])
{
    unsigned int len, start, j, k;
    int32_t t, zeta;

    k = 0;
    len = 128;
    zeta = root_table[k++];
    // a sepearate first layer for storing results to output polynomial
    for (j = 0; j < len; j++) {
        t = fqmul(zeta, (int32_t)in[j + len]);
        out[j + len] = in[j] - t;
        out[j] = in[j] + t;
    }
    // remaining seven layers
    for (len = 64; len >= 1; len >>= 1) {
        for (start = 0; start < 256; start = j + len) {
            zeta = root_table[k++];
            for (j = start; j < start + len; j++) {
                t = fqmul(zeta, out[j + len]);
                out[j + len] = out[j] - t;
                out[j] = out[j] + t;
            }
        }
    }
}

// input: in[index], in[index+len] output: arg1, arg2
#define BFUNIT_IN(arg1, arg2, index, len)      \
    t = fqmul(zeta, (int32_t)in[index + len]); \
    arg2 = in[index] - t;                      \
    arg1 = in[index] + t;

// input: in[index], in[index+len] output: arg1, arg2
#define BFUNIT_IN_OUT(arg1, arg2, index, len)   \
    t = fqmul(zeta, (int32_t)out[index + len]); \
    arg2 = out[index] - t;                      \
    arg1 = out[index] + t;

// in-place
#define BFUNIT(arg1, arg2) \
    t = fqmul(zeta, arg2); \
    arg2 = arg1 - t;       \
    arg1 = arg1 + t;

// input: arg1, arg2 output: out[index], out[index+len]
#define BFUNIT_OUT(arg1, arg2, index, len) \
    t = fqmul(zeta, arg2);                 \
    out[index + len] = arg1 - t;           \
    out[index] = arg1 + t;

/*************************************************
 * Name:        ntt_merged
 *
 * Description: Number-theoretic transform (NTT).
 *              input is in standard order, output is in bitreversed order
 *
 * Arguments:   - int32_t in/out[256]: pointer to input/output polynomial
 **************************************************/
void ntt_merged(const int16_t in[256], int32_t out[256])
{
    unsigned int i, k;
    int32_t t, zeta;
    int32_t t0, t1, t2, t3, t4, t5, t6, t7;
    int32_t t8, t9, t10, t11, t12, t13, t14, t15;

    // merged layers 1-4, which need 15 canstant
    for (i = 0; i < 16; i++) {
        // layer 1 for loading coefficients to t0-t15
        zeta = root_table_merged[0];
        BFUNIT_IN(t0, t8, 0 * 16 + i, 128)
        BFUNIT_IN(t1, t9, 1 * 16 + i, 128)
        BFUNIT_IN(t2, t10, 2 * 16 + i, 128)
        BFUNIT_IN(t3, t11, 3 * 16 + i, 128)
        BFUNIT_IN(t4, t12, 4 * 16 + i, 128)
        BFUNIT_IN(t5, t13, 5 * 16 + i, 128)
        BFUNIT_IN(t6, t14, 6 * 16 + i, 128)
        BFUNIT_IN(t7, t15, 7 * 16 + i, 128)
        // layer 2
        zeta = root_table_merged[1];
        BFUNIT(t0, t4)
        BFUNIT(t1, t5)
        BFUNIT(t2, t6)
        BFUNIT(t3, t7)
        zeta = root_table_merged[2];
        BFUNIT(t8, t12)
        BFUNIT(t9, t13)
        BFUNIT(t10, t14)
        BFUNIT(t11, t15)
        // layer 3
        zeta = root_table_merged[3];
        BFUNIT(t0, t2)
        BFUNIT(t1, t3)
        zeta = root_table_merged[4];
        BFUNIT(t4, t6)
        BFUNIT(t5, t7)
        zeta = root_table_merged[5];
        BFUNIT(t8, t10)
        BFUNIT(t9, t11)
        zeta = root_table_merged[6];
        BFUNIT(t12, t14)
        BFUNIT(t13, t15)
        // layer 4 for storing results to out
        zeta = root_table_merged[7];
        BFUNIT_OUT(t0, t1, 0 * 16 + i, 16)
        zeta = root_table_merged[8];
        BFUNIT_OUT(t2, t3, 2 * 16 + i, 16)
        zeta = root_table_merged[9];
        BFUNIT_OUT(t4, t5, 4 * 16 + i, 16)
        zeta = root_table_merged[10];
        BFUNIT_OUT(t6, t7, 6 * 16 + i, 16)
        zeta = root_table_merged[11];
        BFUNIT_OUT(t8, t9, 8 * 16 + i, 16)
        zeta = root_table_merged[12];
        BFUNIT_OUT(t10, t11, 10 * 16 + i, 16)
        zeta = root_table_merged[13];
        BFUNIT_OUT(t12, t13, 12 * 16 + i, 16)
        zeta = root_table_merged[14];
        BFUNIT_OUT(t14, t15, 14 * 16 + i, 16)
    }
    // merged layers 5-8, which need 15*16 constant
    k = 15;
    for (i = 0; i < 256; i += 16) {
        // layer 5 for loading coefficients to t0-t15
        zeta = root_table_merged[k++];
        BFUNIT_IN_OUT(t0, t8, i + 0, 8)
        BFUNIT_IN_OUT(t1, t9, i + 1, 8)
        BFUNIT_IN_OUT(t2, t10, i + 2, 8)
        BFUNIT_IN_OUT(t3, t11, i + 3, 8)
        BFUNIT_IN_OUT(t4, t12, i + 4, 8)
        BFUNIT_IN_OUT(t5, t13, i + 5, 8)
        BFUNIT_IN_OUT(t6, t14, i + 6, 8)
        BFUNIT_IN_OUT(t7, t15, i + 7, 8)
        // layer 6
        zeta = root_table_merged[k++];
        BFUNIT(t0, t4)
        BFUNIT(t1, t5)
        BFUNIT(t2, t6)
        BFUNIT(t3, t7)
        zeta = root_table_merged[k++];
        BFUNIT(t8, t12)
        BFUNIT(t9, t13)
        BFUNIT(t10, t14)
        BFUNIT(t11, t15)
        // layer 7
        zeta = root_table_merged[k++];
        BFUNIT(t0, t2)
        BFUNIT(t1, t3)
        zeta = root_table_merged[k++];
        BFUNIT(t4, t6)
        BFUNIT(t5, t7)
        zeta = root_table_merged[k++];
        BFUNIT(t8, t10)
        BFUNIT(t9, t11)
        zeta = root_table_merged[k++];
        BFUNIT(t12, t14)
        BFUNIT(t13, t15)
        // layer 8 for storing results to out
        zeta = root_table_merged[k++];
        BFUNIT_OUT(t0, t1, i + 0, 1)
        zeta = root_table_merged[k++];
        BFUNIT_OUT(t2, t3, i + 2, 1)
        zeta = root_table_merged[k++];
        BFUNIT_OUT(t4, t5, i + 4, 1)
        zeta = root_table_merged[k++];
        BFUNIT_OUT(t6, t7, i + 6, 1)
        zeta = root_table_merged[k++];
        BFUNIT_OUT(t8, t9, i + 8, 1)
        zeta = root_table_merged[k++];
        BFUNIT_OUT(t10, t11, i + 10, 1)
        zeta = root_table_merged[k++];
        BFUNIT_OUT(t12, t13, i + 12, 1)
        zeta = root_table_merged[k++];
        BFUNIT_OUT(t14, t15, i + 14, 1)
    }
}

/*************************************************
 * Name:        ntt_merged
 *
 * Description: Number-theoretic transform (NTT).
 *              input is in standard order, output is in bitreversed order
 *
 * Arguments:   - int32_t in/out[256]: pointer to input/output polynomial
 **************************************************/
void ntt_merged_old(const int16_t in[256], int32_t out[256])
{
    unsigned int len, start, i, j, k;
    int32_t t, zeta;
    int32_t a[16];

    // merged layers 1-4
    for (i = 0; i < 16; i++) {
        // sepearate layer 1 for transfering coefficients to a[16]
        k = 0;
        len = 128;
        zeta = root_table_merged[k++];
        for (start = 0, j = i; j < i + len; start++, j += 16) {
            t = fqmul(zeta, (int32_t)in[j + len]);
            a[start + 8] = in[j] - t;
            a[start] = in[j] + t;
        }
        // in-place layers 2-4
        for (len = 4; len >= 1; len >>= 1) {
            // len:block = 4:2, 2:4, 1:8, block = 16/(2*len)
            for (start = 0; start < 16; start = j + len) {
                zeta = root_table_merged[k++];
                for (j = start; j < start + len; j++) {
                    t = fqmul(zeta, a[j + len]);
                    a[j + len] = a[j] - t;
                    a[j] = a[j] + t;
                }
            }
        }
        len = 128;
        // save results to out
        for (start = 0, j = i; j < i + len; start++, j += 16) {
            out[j + len] = a[start + 8];
            out[j] = a[start];
        }
    }
    // for (i = 0; i < 256; i++) {
    //     printf("%d,", out[i]);
    // }

    // merged layers 5-8
    for (i = 0; i < 256; i += 16) {
        // layer 5-8
        for (len = 8; len >= 1; len >>= 1) {
            // tt1 = 0;
            // len:block = 8:1, 4:2, 2:4, 1:8, block = 16/(2*len)
            for (start = i; start < i + 16; start = j + len) {
                zeta = root_table_merged[k++];
                for (j = start; j < start + len; j++) {
                    t = fqmul(zeta, out[j + len]);
                    out[j + len] = out[j] - t;
                    out[j] = out[j] + t;
                }
                // tt1++;
            }
            // printf("tt1=%d\n", tt1);
            // for (unsigned int ii = i; ii < i + 16; ii++) {
            //     printf("%d,", out[ii]);
            // }
            // printf("\n");
        }
    }
    // printf("k is %d\n", k);
    // printf("\n");
    // for (i = 0; i < 16; i++) {
    //     printf("%d,", a[i]);
    // }
}

/*************************************************
 * Name:        invntt_tomont
 *
 * Description: Inverse number-theoretic transform and
 *              multiplication by Montgomery factor 2^32.
 *              Input is in bitreversed order, output is in standard order
 *
 * Arguments:   - int32_t in/out[256]: pointer to input/output polynomial
 **************************************************/
void invntt(int32_t in[256], int32_t out[256])
{
    unsigned int start, len, j, k;
    int32_t t, zeta;
    // mont^2/256 mod M = (2^32)^2/256 mod M
    const int32_t f = NINV;

    k = 0;
    len = 1;
    // a separate first layer for storing results to output polynomial
    for (start = 0; start < 256; start = j + len) {
        zeta = inv_root_table[k++];
        for (j = start; j < start + len; j++) {
            t = in[j];
            // out[j] = barrett_reduce(t + in[j + len]);
            out[j] = t + in[j + len];
            out[j + len] = t - in[j + len];
            out[j + len] = fqmul(zeta, out[j + len]);
        }
    }
    // remaining seven layers
    for (len = 2; len <= 128; len <<= 1) {
        for (start = 0; start < 256; start = j + len) {
            zeta = inv_root_table[k++];
            for (j = start; j < start + len; j++) {
                t = out[j];
                // out[j] = barrett_reduce(t + out[j + len]);
                out[j] = t + out[j + len];
                out[j + len] = t - out[j + len];
                out[j + len] = fqmul(zeta, out[j + len]);
            }
        }
    }

    // multiply mont^2/256, reduce to centered representatives, get low 13 bits
    for (j = 0; j < 256; j++) {
        out[j] = fqmul(out[j], f);
        out[j] = barrett_reduce(out[j]);
        out[j] &= 0x1fff;
    }
}