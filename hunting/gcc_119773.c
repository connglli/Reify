int a = 1, b, c = 1, d;

int main() {
e:
  d = 1 - a;
  if (d + c <= 0)
    goto g;
  b = 1 - a;
  if (b - 2147483647 * c >= 0)
    goto g;
  b = 1;
  c = 0;
  goto e;
g:
  if (b)
    return 0;
  goto e;
}
