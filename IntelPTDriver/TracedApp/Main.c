#include <stdio.h>
#include <windows.h>

DWORD WINAPI ThreadProc(
    _In_ LPVOID lpParameter
)
{
    while (1==1);
}

void main()
{
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
    while (1 == 1)
    {
        YieldProcessor();
     }
     

    //printf("feels i'm being watched\n");
}