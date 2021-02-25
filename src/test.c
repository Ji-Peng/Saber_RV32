
// #include <stdint.h>
// #include <stdio.h>
// void test1(int32_t in[8], int32_t out[8])
// {
//     int32_t t[8] = {0};
//     int i;
//     for (i = 0; i < 8; i++) {
//         t[i] = in[i];
//     }
//     for (i = 0; i < 4; i++) {
//         t[i + 4] = t[i] - t[i + 4];
//         t[i] = t[i] + t[i + 4];
//     }
//     for (i = 0; i < 2; i++) {
//         t[i + 2] = t[i] - t[i + 2];
//         t[i] = t[i] + t[i + 2];
//     }
//     for (i = 4; i < 6; i++) {
//         t[i + 2] = t[i] - t[i + 2];
//         t[i] = t[i] + t[i + 2];
//     }
//     for (i = 0; i < 8; i++) {
//         out[i] = t[i];
//     }
// }

// void test2(int32_t in[8], int32_t out[8])
// {
//     int32_t t0, t1, t2, t3, t4, t5, t6, t7;
//     int i;
//     t0 = in[0];
//     t1 = in[1];
//     t2 = in[2];
//     t3 = in[3];
//     t4 = in[4];
//     t5 = in[5];
//     t6 = in[6];
//     t7 = in[7];

//     t4 = t0 - t4;
//     t0 = t0 + t4;
//     t5 = t1 - t5;
//     t1 = t1 + t5;
//     t6 = t2 - t6;
//     t2 = t2 + t6;
//     t7 = t3 - t7;
//     t3 = t3 + t7;

//     t2 = t0 - t2;
//     t0 = t0 + t2;
//     t3 = t1 - t3;
//     t1 = t1 + t3;
//     t6 = t4 - t6;
//     t4 = t4 + t6;
//     t7 = t5 - t7;
//     t5 = t5 + t7;

//     out[0] = t0;
//     out[1] = t1;
//     out[2] = t2;
//     out[3] = t3;
//     out[4] = t4;
//     out[5] = t5;
//     out[6] = t6;
//     out[7] = t7;
// }

// int main(void)
// {
//     int32_t in[8], out1[8], out2[8], i;
//     for (i = 0; i < 8; i++) {
//         in[i] = i;
//     }
//     test1(in, out1);
//     test2(in, out2);
//     for (i = 0; i < 8; i++) {
//         printf("%d ", out1[i]);
//     }
//     printf("\n");
//     for (i = 0; i < 8; i++) {
//         printf("%d ", out2[i]);
//     }
// }