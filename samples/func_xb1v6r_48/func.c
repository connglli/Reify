extern int computeStatelessChecksum(int num_args, int args[]);
struct func_xb1v6r_48_S0 {
  int f0;
  int f1[1];
  int f2[2];
};
struct func_xb1v6r_48_S1 {
  int f0;
  struct func_xb1v6r_48_S0 f1[2];
};
int func_xb1v6r_48(struct func_xb1v6r_48_S1 v0, int v1, int v2, int v3, int v4, int v5[1][2]) {
  int v6 = 0;
  int v7 = 0;
BB0:
  v7 = (-1 * v5[0][1]) + (-1 * v0.f1[1].f1[0]) + (-1);
  v1 = (-1 / v4) - (-1 * v1) - (1);
  goto BB17;
BB1:
  v7 = (0 - v1) - (0 * v7) - (0);
  v3 = (0 % v7) - (0 - v4) - (0);
  goto BB6;
BB2:
  v7 = (0 - v5[0][1]) - (0 / v4) - (0);
  v3 = (0 - v7) - (0 + v5[0][1]) - (0);
  if ((0 * v7) - (0 / v3) - (0 % v4) - (0) == 0) goto BB14;
  goto BB18;
BB3:
  v2 = (0 % v0.f0) + (0 - v5[0][1]) + (0);
  v6 = (0 - v2) + (0 % v5[0][1]) + (0);
  goto BB5;
BB4:
  v7 = (0 + v3) - (0 / v2) - (0);
  v5[0][1] = (0 - v7) - (0 - v3) - (0);
  if ((0 / v7) + (0 - v5[0][1]) + (0 - v2) + (0) == 0) goto BB1;
  goto BB14;
BB5:
  v6 = (0 / v1) + (0 / v2) + (0);
  v1 = (0 * v1) + (0 - v7) + (0);
  goto BB4;
  do {
BB6:
    v0.f1[1].f0 = (0 * v0.f1[1].f0) + (-2147483648);
    v6 = (0 * v6) + (-2147483648);
    v2 = (0 * v2) + (-1);
    v5[0][1] = (0 * v5[0][1]) + (-2147483648);
    v4 = (0 * v0.f1[1].f2[1]) - (0 * v6) - (0);
    v5[0][1] = (0 % v2) + (0 - v5[0][1]) + (0);
    if ((0 - v4) - (0 + v5[0][1]) - (0 * v2) - (0) == 0) goto BB8;
    goto BB9;
BB7:
    v5[0][1] = (0 - v7) - (0 - v6) - (0);
    v3 = (0 % v6) - (0 / v3) - (0);
    goto BB8;
BB8:
    v4 = (0 + v1) + (0 * v7) + (0);
    v5[0][1] = (0 / v3) - (0 % v4) - (0);
    goto BB9;
BB9:
    v0.f1[1].f0 = (0 % v1) - (0 / v2) - (0);
    v7 = (0 * v5[0][1]) + (0 % v4) + (0);
  } while ((0 / v0.f0) - (0 + v7) - (0 - v4) - (0) < 0);
  goto BB10;
BB10:
  v6 = (0 / v0.f0) + (0 + v7) + (0);
  v7 = (0 / v7) - (0 - v1) - (0);
  if ((0 / v6) + (0 % v7) + (0 / v3) + (0) > 0) goto BB15;
  goto BB16;
BB11:
  v4 = (0 + v4) + (0 - v0.f0) + (0);
  v3 = (0 + v2) + (0 * v7) + (0);
  if ((0 + v4) + (0 % v3) + (0 - v2) + (0) == 0) goto BB2;
  goto BB3;
BB12:
  v6 = (0 % v2) + (0 % v6) + (0);
  v4 = (0 * v7) - (0 * v3) - (0);
  goto BB3;
BB13:
  v4 = (0 / v4) - (0 - v0.f1[1].f0) - (0);
  v3 = (0 + v1) + (0 / v2) + (0);
  goto BB18;
BB14:
  v0.f1[1].f2[1] = (0 % v2) - (0 / v3) - (0);
  v7 = (0 / v3) + (0 + v6) + (0);
  if ((0 * v0.f0) - (0 - v7) - (0 * v5[0][1]) - (0) < 0) goto BB6;
  goto BB17;
BB15:
  v7 = (0 * v1) + (0 * v4) + (0);
  v4 = (0 - v0.f0) - (0 / v4) - (0);
  goto BB11;
BB16:
  v0.f0 = (0 % v4) - (0 - v7) - (0);
  v3 = (0 * v6) - (0 % v2) - (0);
  if ((0 / v0.f0) + (0 + v3) + (0 * v1) + (0) < 0) goto BB17;
  goto BB18;
BB17:
  v6 = (-1 * v5[0][1]) - (-1 % v7) - (2);
  v5[0][1] = (-1 + v0.f0) + (-1 % v4) + (-1);
  if ((-1 / v6) - (-1 * v5[0][1]) - (-1 / v3) - (-1) > 0) goto BB16;
  goto BB18;
BB18:
  v4 = (-1 % v4) + (-1 * v6) + (-1);
  v5[0][1] = (0 - v1) - (-1 * v5[0][1]) - (-1);
  {
    int __chk_args[] = {v0.f0, v0.f1[0].f0, v0.f1[0].f1[0], v0.f1[0].f2[0], v0.f1[0].f2[1], v0.f1[1].f0, v0.f1[1].f1[0], v0.f1[1].f2[0], v0.f1[1].f2[1], v1, v2, v3, v4, v5[0][0], v5[0][1]};
    return computeStatelessChecksum(sizeof(__chk_args)/sizeof(int), __chk_args);
  }
}



// rysmith options: ./build/bin/rysmith --Xnum-bbls-per-fun 15 --Xnum-vars-per-fun 8 --Xnum-assigns-per-bbl 2 --Xnum-vars-per-assign 2 --Xnum-vars-in-cond 3 -m -S -A -U -v -s 468399889 --Xbitwuzla-threads 8 -o samples -n 48 xb1v6r