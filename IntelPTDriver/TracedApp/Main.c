#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

void main()
{
    char lookAtMeIAmABufferOverflow[10];
    int a = 10;
    int b = 20;
    scanf("%s", lookAtMeIAmABufferOverflow);

    int c = a + b;

    for (int i = 0; i < c; i++)
    {
        a++;
    }

    printf("a = %d\n", a);
}