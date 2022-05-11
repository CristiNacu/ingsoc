#include <stdio.h>
#include <windows.h>
#include <intrin.h>

DWORD WINAPI ThreadProc(
    _In_ LPVOID lpParameter
)
{
    while (1==1);
}

void main()
{

    int a = 1;
    int b = 2;

    a += b;

    if (a + b > 10)
    {
        printf("A + B > 10");
    }
    else
    {
        printf("A + B <= 10");
    }
}