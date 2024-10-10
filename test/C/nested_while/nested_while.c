int main(int argc, char ** v) {
  int j = 0;
  int acc = argc;
  do {
    int i = 0;
    do {
      ++i;
      acc += i;
    } while (i < 50);
    j++;
  } while (j < 50);
  return acc;
}
