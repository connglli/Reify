int a = 1, b, c;
int main() {
  c = -2147483647 * b - 2147483647 * a;
  if (b - 2147483647 * a >= 0)
    __builtin_abort();
}