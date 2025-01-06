/**
 * Simple Bitpacking api to set and read flag from a 1 byte character
 */
#include <stdio.h>

void print_bit(unsigned char *byte) {

  // use LSB 0x01 and shift the bits one by one to mask the left side and
  // compare bye: 0000000|00010010
  //            ^
  // lsb: 0000001
  for (int i = 7; 0 <= i; i--) {
    printf("%d", (*byte >> i) & 0x01);
  }
  printf("\n");
}

void set_flag(unsigned char *byte, int flag, int value) {
  if (value == 0) {
    //~ : 00000001|00000000 -> 11111110|11111111
    *byte &= ~(1 << flag);
  } else if (value == 1) {
    // Classic LSB method, shift 000000001 under the right flag
    *byte |= (1 << flag);
  } else {
    perror("Flags can only accept binary values (0 | 1)");
  }
}

int get_flag(unsigned char *byte, int flag) {
  return (int)((*byte >> flag) & 0x01);
}

void bitpack_example() {

  unsigned char weather = 0;
  /*
  0 - sun
  1 - rain
  2 - night
  3 - cloud
  4 - snow
  5 - spring
  6 - blossom
  7 - rainbow
  */

  set_flag(&weather, 1, 1); // set rain to true
  int rain_flag = get_flag(&weather, 1);
  print_bit(&weather);
  printf("Rain flag: %d\n", rain_flag);

}