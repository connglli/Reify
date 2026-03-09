extern int computeStatelessChecksum(int num_args, int args[]);
struct func_xb1v6r_88_S0 {
  int f0;
};
struct func_xb1v6r_88_S1 {
  int f0;
  int f1;
  struct func_xb1v6r_88_S0 f2[1];
};
struct func_xb1v6r_88_S2 {
  int f0;
  struct func_xb1v6r_88_S0 f1;
  struct func_xb1v6r_88_S1 f2;
};
int func_xb1v6r_88(int v0, struct func_xb1v6r_88_S2 v1, int v2, int v3, int v4, struct func_xb1v6r_88_S2 v5) {
  int v6 = -1;
  int v7[3][1] = {-1, -1, -1};
BB0:
  v7[2][0] = (-1 / v4) - (-1 * v7[2][0]) - (1);
  v1.f0 = (-1 - v7[2][0]) + (-1 * v5.f2.f2[0].f0) + (-1);
  goto BB5;
BB1:
  v5.f2.f1 = (-1 / v0) - (-1 % v4) - (-1);
  v7[2][0] = (-1 % v6) - (-1 / v3) - (2);
  if ((-1 % v5.f0) - (-1 / v7[2][0]) - (-1 - v2) - (-1) == 0) goto BB10;
  goto BB13;
BB2:
  v3 = (0 / v5.f1.f0) + (0 / v6) + (0);
  v0 = (0 / v7[2][0]) - (0 * v3) - (0);
  goto BB3;
BB3:
  v7[2][0] = (0 / v5.f2.f2[0].f0) - (0 * v6) - (0);
  v0 = (0 * v3) + (0 / v4) + (0);
  if ((0 * v7[2][0]) + (0 * v0) + (0 + v2) + (0) == 0) goto BB4;
  goto BB8;
BB4:
  v1.f2.f2[0].f0 = (-1 + v7[2][0]) + (-1 - v2) + (-1);
  v3 = (-1 * v5.f1.f0) + (-1 / v6) + (-1);
  if ((-1 * v1.f0) - (-1 % v3) - (-1 / v0) - (-1) == 0) goto BB6;
  goto BB7;
BB5:
  v1.f2.f1 = (-1 * v2) - (2 - v4) - (-1);
  v7[2][0] = (-5 - v0) - (-1 + v6) - (-1);
  if ((-1 + v1.f2.f2[0].f0) - (-1 + v7[2][0]) - (-1 - v5.f0) - (-1) == 0) goto BB3;
  goto BB7;
BB6:
  v6 = (-1 % v1.f1.f0) + (-1 - v0) + (-1);
  v0 = (-1 + v3) + (-1 + v2) + (-1);
  if ((-1 * v6) + (-1 - v0) + (-1 * v2) + (-1) > 0) goto BB1;
  goto BB7;
BB7:
  v6 = (-1 % v2) + (-1 + v7[2][0]) + (-1);
  v3 = (-1 / v2) + (-1 - v1.f0) + (-1);
  if ((-1 + v6) - (-1 / v3) - (-1 * v5.f0) - (-1) == 0) goto BB1;
  goto BB4;
BB8:
  v0 = (0 / v4) + (0 - v6) + (0);
  v3 = (0 * v4) - (0 / v7[2][0]) - (0);
  if ((0 + v0) - (0 * v3) - (0 + v6) - (0) < 0) goto BB1;
  goto BB9;
BB9:
  v4 = (0 * v3) + (0 / v2) + (0);
  v0 = (0 / v5.f0) - (0 * v4) - (0);
  if ((0 + v4) + (0 + v0) + (0 % v5.f2.f1) + (0) > 0) goto BB11;
  goto BB12;
BB10:
  v2 = (-1 - v3) + (-1 * v4) + (-1);
  v3 = (-1 + v6) - (-1 + v5.f2.f1) - (-1);
  if ((-1 + v2) - (-1 - v3) - (-1 / v0) - (-1) == 0) goto BB11;
  goto BB14;
BB11:
  v0 = (0 * v0) + (1073741824);
  v7[2][0] = (0 * v7[2][0]) + (-1073741825);
  v1.f2.f0 = (0 * v1.f2.f0) + (1);
  v7[2][0] = (0 * v7[2][0]) + (-1073741825);
  v2 = (0 + v0) - (0 - v7[2][0]) - (0);
  v6 = (0 % v1.f2.f2[0].f0) + (0 + v7[2][0]) + (0);
  goto BB4;
BB12:
  v1.f1.f0 = (0 * v0) + (0 + v7[2][0]) + (0);
  v7[2][0] = (0 + v6) + (0 * v0) + (0);
  goto BB3;
BB13:
  v5.f2.f2[0].f0 = (0 % v6) - (0 % v5.f0) - (0);
  v1.f0 = (0 / v3) - (0 - v4) - (0);
  goto BB3;
BB14:
  v3 = (-1 / v2) + (-1 % v5.f1.f0) + (-1);
  v1.f2.f0 = (-1 + v3) - (-1 * v2) - (-4);
  {
    int __chk_args[] = {v0, v1.f0, v1.f1.f0, v1.f2.f0, v1.f2.f1, v1.f2.f2[0].f0, v2, v3, v4, v5.f0, v5.f1.f0, v5.f2.f0, v5.f2.f1, v5.f2.f2[0].f0};
    return computeStatelessChecksum(sizeof(__chk_args)/sizeof(int), __chk_args);
  }
}



// rysmith options: ./build/bin/rysmith --Xnum-bbls-per-fun 15 --Xnum-vars-per-fun 8 --Xnum-assigns-per-bbl 2 --Xnum-vars-per-assign 2 --Xnum-vars-in-cond 3 -m -S -A -U -v -s 1043830061 --Xbitwuzla-threads 8 -o samples -n 88 xb1v6r