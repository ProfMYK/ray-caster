#include <stdio.h>
#include <stdint.h>

#include "BRICK_4A.Ppm"

#define u32 uint32_t

int main() {
    printf("int T_wall[] = {");
    for (int x = 0; x < 64; x++) {
        for (int y = 0; y < 64; y++) {
            u32 r = T_wall[(y*64+x)*3];
            u32 g = T_wall[(y*64+x)*3+1];
            u32 b = T_wall[(y*64+x)*3+2];
            u32 a = 0;

            u32 hex = (r << 24) | (g << 16) | (b << 8) | a;
            printf("0x%x,", hex);
        }
        printf("\n");
    }
    printf("};");
    
    return 0;
}