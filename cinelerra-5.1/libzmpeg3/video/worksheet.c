#include <stdio.h>
#include <string.h>

inline static void 
mmx_601(unsigned long y, unsigned long *output)
{
  asm(" movd (%0), %%mm0;   /* Load y   0x00000000000000yy */\
        psllw $6, %%mm0;    /* Shift y coeffs 0x0000yyy0yyy0yyy0 */\
        movd %%mm0, (%1);   /* Store output */"
      :
      : "r" (&y), "r" (output));
}

int main(int argc, char *argv[])
{
  unsigned char output[16];
  memset(output, 0, sizeof(output));
  mmx_601(1, (unsigned long*)output);
  printf("%02x%02x\n", *(unsigned char*)&output[1], *(unsigned char*)&output[0]);
  return 0;
}

