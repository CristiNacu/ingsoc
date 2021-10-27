//#include "Communication.h"
//#include "Debug.h"
//
//static BOOLEAN gDefaultQueueInitialized = FALSE;
//
//NTSTATUS
//InitCommQueue(
//    _In_ WDFDEVICE                   ControlDevice,
//    _In_ IO_QUEUE_SETTINGS           *IoQueueSettings,
//    _Inout_ WDFQUEUE                 *Queue
//)
//{
//    NTSTATUS                        status;
//    WDF_IO_QUEUE_CONFIG             ioQueueConfig = { 0 };
//
//    DEBUG_PRINT("InitCommQueue\n");
//
//    if (IoQueueSettings->DefaultQueue && !gDefaultQueueInitialized)
//    {
//        gDefaultQueueInitialized = TRUE;
//        WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig, WdfIoQueueDispatchParallel);
//    }
//    else if(IoQueueSettings->DefaultQueue && gDefaultQueueInitialized)
//    {
//        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[ERROR] Only one default queue alowed\n"));
//        return STATUS_ALREADY_INITIALIZED;
//    }
//    else
//    {
//        WDF_IO_QUEUE_CONFIG_INIT(&ioQueueConfig, WdfIoQueueDispatchManual);
//    }
//
//    ioQueueConfig.Settings.Parallel.NumberOfPresentedRequests   = ((ULONG)-1);
//
//    ioQueueConfig.EvtIoDefault                                  = IoQueueSettings->Callbacks.IoQueueIoDefault;
//    ioQueueConfig.EvtIoRead                                     = IoQueueSettings->Callbacks.IoQueueIoRead;
//    ioQueueConfig.EvtIoWrite                                    = IoQueueSettings->Callbacks.IoQueueIoWrite;
//    ioQueueConfig.EvtIoStop                                     = IoQueueSettings->Callbacks.IoQueueIoStop;
//    ioQueueConfig.EvtIoResume                                   = IoQueueSettings->Callbacks.IoQueueIoResume;
//    ioQueueConfig.EvtIoCanceledOnQueue                          = IoQueueSettings->Callbacks.IoQueueIoCanceledOnQueue;
//
//    ioQueueConfig.EvtIoDeviceControl                            = IoQueueSettings->Callbacks.IoQueueIoDeviceControl;
//    ioQueueConfig.EvtIoInternalDeviceControl                    = IoQueueSettings->Callbacks.IoQueueIoInternalDeviceControl;
//
//    ioQueueConfig.PowerManaged                                  = WdfFalse;
//
//    status = WdfIoQueueCreate(ControlDevice, &ioQueueConfig, NULL, Queue);
//    if (!NT_SUCCESS(status))
//    {
//        DEBUG_PRINT("WdfIoQueueCreate error status %X\n", status);
//        return status;
//    }
//
//    return status;
//}