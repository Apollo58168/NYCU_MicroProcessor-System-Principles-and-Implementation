#include <stdio.h>

#define A    ((int volatile *) 0xC2000000)
#define min  ((int volatile *) 0xC2000020)
#define max  ((int volatile *) 0xC2000024)
#define trig ((int volatile *) 0xC2000028)

int main(void)
{
    A[0] =  17, A[1] =  -24, A[2] = 33, A[3] = 6;
    A[4] = 173, A[5] = -743, A[6] = 12, A[7] = 0;

    // Triger the IP to find mix/max values
    *trig = 1;
    while (*trig) /* busy loop */; // Waiting for IP to respond

    printf("The minimal value is: %d.\n", *min);
    printf("The maximal value is: %d.\n", *max);
}

