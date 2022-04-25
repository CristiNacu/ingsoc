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
     

    //printf("feels i'm being watched\n");
}