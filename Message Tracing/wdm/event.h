
//
// DEFINES
//

#define NTDEVICE_NAME_STRING      L"\\Device\\Event_Sample"
#define SYMBOLIC_NAME_STRING      L"\\DosDevices\\Event_Sample"
#define TAG (ULONG)'TEVE'

#if DBG
#define DebugPrint(_x_) \
                DbgPrint("EVENT.SYS: ");\
                DbgPrint _x_;

#else

#define DebugPrint(_x_)

#endif

//
// DATA
//

typedef struct _NOTIFY_RECORD{
    NOTIFY_TYPE     Type;
    union {
        PKEVENT     Event;
        PIRP        PendingIrp;
    } Message;
    KDPC            Dpc;
} NOTIFY_RECORD, *PNOTIFY_RECORD;

DRIVER_INITIALIZE DriverEntry;

_Dispatch_type_(IRP_MJ_CREATE)
DRIVER_DISPATCH EventCreate;

_Dispatch_type_(IRP_MJ_CLOSE)
DRIVER_DISPATCH EventClose;

_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
DRIVER_DISPATCH EventDispatchIoControl;

DRIVER_UNLOAD EventUnload;

KDEFERRED_ROUTINE CustomTimerDPC;

DRIVER_DISPATCH RegisterEventBasedNotification;

DRIVER_DISPATCH RegisterIrpBasedNotification;

_Use_decl_annotations_ VOID NotifyUsermodeCallback(PKDPC Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);