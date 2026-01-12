#include <stdio.h>
#include <string.h>

char volatile    *A = (char *) 0xC2000000;
char volatile    *B = (char *) 0xC2000010;
int  volatile    *c = (int  *) 0xC2000020;
int  volatile *trig = (int  *) 0xC2000030;

char a_init[] = {  17, -24, 33, -6, 88, -43, 12,  0 };
char b_init[] = {-127,  -4,  9,  3, 73,  23, 31, 41 };

int main(void)
{
    int i;

    // Copy the A[] and B[] vectors into the IP
    memcpy((void *) A, (void *) a_init, sizeof(a_init));
    memcpy((void *) B, (void *) b_init, sizeof(b_init));

    // Print the vectors A[] and B[]
    printf("A[]={"); for (i=0; i<8; i++) printf("%5d,", A[i]); printf(" }\n");
    printf("B[]={"); for (i=0; i<8; i++) printf("%5d,", B[i]); printf(" }\n\n");

    // Triger the IP to find mix/max values
    *trig = 1;
    while (*trig) /* busy loop */; // Waiting for IP to respond

    printf("The inner product value by the IP is: %d.\n", *c);
    printf("The result %s correct!\n\n", (*c == 4023)? "is" : "isn’t");

    return 0;
}

