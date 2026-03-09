extern int computeStatelessChecksum(int num_args, int args[]);
struct func_xb1v6r_79_S0 {
  int f0;
  int f1[1][3];
};
struct func_xb1v6r_79_S1 {
  int f0[1];
  struct func_xb1v6r_79_S0 f1;
};
int func_xb1v6r_79(int v0, int v1, int v2, int v3, int v4, struct func_xb1v6r_79_S1 v5) {
  int v6 = 0;
  int v7 = -1;
BB0:
  v3 = (-1 / v5.f0[0]) + (-1 / v1) + (-1);
  v6 = (-1 + v7) - (-1 % v4) - (-1);
  if ((-1 / v3) + (-1 / v6) + (-1 - v1) + (2147483647) < 0) goto BB2;
  goto BB5;
BB1:
  v3 = (-1 - v7) - (-1 * v6) - (-1);
  v6 = (-1 + v7) + (-1 + v4) + (-1);
  if ((-1 * v3) + (-1 % v6) + (-1 - v2) + (2147483647) > 0) goto BB4;
  goto BB10;
BB2:
  v7 = (0 * v4) - (0 - v3) - (0);
  v4 = (0 * v7) + (0 % v1) + (0);
  if ((0 - v7) + (0 - v4) + (0 % v0) + (0) < 0) goto BB3;
  goto BB6;
BB3:
  v1 = (0 * v1) + (-1);
  v6 = (0 * v6) + (-1);
  v3 = (0 * v3) + (2);
  v5.f1.f0 = (0 * v5.f1.f0) + (-1);
  v1 = (0 / v1) - (0 / v6) - (0);
  v5.f1.f0 = (0 - v3) + (0 - v5.f1.f0) + (0);
  if ((0 - v1) + (0 - v5.f0[0]) + (0 % v3) + (0) < 0) goto BB6;
  goto BB12;
BB4:
  v3 = (-1 - v3) + (-1 - v1) + (-1);
  v6 = (-1 % v1) + (-1 - v3) + (-1);
  if ((-1 - v3) + (-1 + v6) + (-1 * v0) + (-1) == 0) goto BB6;
  goto BB13;
BB5:
  v2 = (-1 - v3) - (-1 % v7) - (-1);
  v7 = (-1 * v4) + (-1 % v7) + (-1);
  if ((-1 / v2) - (-1 - v7) - (-1 + v4) - (-1) > 0) goto BB1;
  goto BB6;
BB6:
  v0 = (0 * v0) + (-1);
  v7 = (0 * v7) + (-2147483648);
  v1 = (0 * v1) + (-1);
  v6 = (0 % v0) + (0 - v7) + (0);
  v0 = (0 / v6) - (0 % v1) - (0);
  if ((0 * v6) - (0 + v0) - (0 % v2) - (0) > 0) goto BB7;
  goto BB10;
BB7:
  v7 = (0 - v6) - (0 * v3) - (0);
  v3 = (0 * v1) + (0 % v0) + (0);
  if ((0 - v7) - (0 + v3) - (0 / v4) - (0) > 0) goto BB6;
  goto BB9;
BB8:
  v5.f0[0] = (0 + v1) - (0 + v7) - (0);
  v2 = (0 + v5.f0[0]) - (0 * v3) - (0);
  if ((0 % v5.f0[0]) - (0 % v2) - (0 % v4) - (0) < 0) goto BB9;
  goto BB14;
BB9:
  v2 = (0 / v6) - (0 + v4) - (0);
  v7 = (0 % v4) - (0 + v7) - (0);
  goto BB2;
BB10:
  v5.f0[0] = (0 * v5.f0[0]) + (-1);
  v1 = (0 * v1) + (-2147483648);
  v1 = (0 * v1) + (-2147483648);
  v2 = (0 * v2) + (-2147483648);
  v0 = (0 - v5.f0[0]) - (0 - v1) - (0);
  v5.f0[0] = (0 / v1) + (0 * v2) + (0);
  goto BB11;
BB11:
  v3 = (0 / v3) + (0 - v1) + (0);
  v1 = (0 % v3) + (0 + v7) + (0);
  if ((0 % v3) + (0 % v1) + (0 % v7) + (0) == 0) goto BB5;
  goto BB8;
BB12:
  v3 = (-1 % v5.f1.f0) - (-1 * v1) - (0);
  v6 = (-1 * v5.f0[0]) - (-1 % v6) - (2);
  if ((-1 * v3) - (-1 + v6) - (-1 % v7) - (2147483647) > 0) goto BB8;
  goto BB14;
BB13:
  v7 = (-1 / v1) - (-1 % v0) - (2);
  v6 = (-1 / v5.f0[0]) - (-1 % v2) - (2);
  goto BB12;
BB14:
  v6 = (-1 / v0) - (-1 * v2) - (-1);
  v5.f1.f1[0][2] = (-1 + v0) - (-2 * v2) - (-1);
  {
    int __chk_args[] = {v0, v1, v2, v3, v4, v5.f0[0], v5.f1.f0, v5.f1.f1[0][0], v5.f1.f1[0][1], v5.f1.f1[0][2]};
    return computeStatelessChecksum(sizeof(__chk_args)/sizeof(int), __chk_args);
  }
}



// rysmith options: ./build/bin/rysmith --Xnum-bbls-per-fun 15 --Xnum-vars-per-fun 8 --Xnum-assigns-per-bbl 2 --Xnum-vars-per-assign 2 --Xnum-vars-in-cond 3 -m -S -A -U -v -s 1935153793 --Xbitwuzla-threads 8 -o samples -n 79 xb1v6r