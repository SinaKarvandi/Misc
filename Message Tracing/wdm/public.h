
#ifndef __PUBLIC__
#define __PUBLIC__


#include "devioctl.h"
#include <dontuse.h>

typedef enum {
    IRP_BASED , 
    EVENT_BASED 
} NOTIFY_TYPE;

typedef struct _REGISTER_EVENT 
{
    NOTIFY_TYPE Type;
    HANDLE  hEvent;

} REGISTER_EVENT , *PREGISTER_EVENT ;

#define SIZEOF_REGISTER_EVENT  sizeof(REGISTER_EVENT )


#define IOCTL_REGISTER_EVENT \
   CTL_CODE( FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS )


#define DRIVER_FUNC_INSTALL     0x01
#define DRIVER_FUNC_REMOVE      0x02

#define DRIVER_NAME       "event"

#endif // __PUBLIC__

// Default buffer size
#define MaximumPacketsCapacity 1000 // number of packets
#define PacketChunkSize		512
#define UsermodeBufferSize  sizeof(UINT32) + PacketChunkSize + 1 /* Becausee of Opeation code at the start of the buffer + 1 for null-termminating */
#define BufferSize MaximumPacketsCapacity * (PacketChunkSize + sizeof(BUFFER_HEADER))
