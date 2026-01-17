extern void e(int *);
int a, b, c, d;

int main() {
  if (c - a)
    goto g;
  d = a + b;
  if (c)
    a = 7 * d;
  c = 20 * a + 5;
  a = 42 * a - 70;
  b = 6 + c + d;
g:
  e((int[]){a, b, c, c});
  return 0;
}
