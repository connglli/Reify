extern int g(void);
int a, b, c;

int e(int f) {
  int d = 0;
  for (; d < 6; d++) {
    a = f <<= 1;
    if (f & 64)
      f ^= 67;
  }
  return a;
}

void h() {
  int i = 0;
  if (c)
    goto j;
  i = -32644994;
k:
  b = 0;
j:
  if (g() - 508050053 + e(i + 79))
    goto k;
}

int main() {
  while (a)
    h();
  return 0;
}
