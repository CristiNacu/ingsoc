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
    //_ptwrite64(0xDEADBEEFDEADBEEF);
    
    //for (int i = 0; i < 3; i++)
    //{
    //    CreateThread(
    //        NULL,
    //        0,
    //        ThreadProc,
    //        NULL,
    //        0,
    //        NULL
    //    );
    //}
    //for (unsigned i = 0; i < 0xff; i++)
    //{
    //    YieldProcessor();
    //    printf_s("Running on AP %d\n", GetCurrentProcessorNumber());
    // }
    
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
    //printf("feels i'm being watched\n");
}