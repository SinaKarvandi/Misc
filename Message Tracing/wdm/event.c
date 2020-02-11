#include <ntddk.h>
#include <ntstrsafe.h>
#include "public.h" 
#include "event.h" 


// =====================================================================

// Message buffer structure
typedef struct _BUFFER_HEADER {
	UINT32 OpeationNumber;	// Operation ID to user-mode
	UINT32 BufferLength;	// The actual length
	BOOLEAN Valid;			// Determine whether the buffer was valid to send or not
} BUFFER_HEADER, * PBUFFER_HEADER;

// Core-specific buffers
typedef struct _CORE_BUFFER_INFORMATION {

	UINT32 CoreIndex;						// Index of the core

	PNOTIFY_RECORD NotifyRecored;			// The record of the thread in IRP Pending state

	UINT64 BufferStartAddress;				// Start address of the buffer
	UINT64 BufferEndAddress;					// End address of the buffer

	KSPIN_LOCK BufferLock;					// SpinLock to protect access to the queue

	UINT32 CurrentIndexToSend;    // Current buffer index to send to user-mode
	UINT32 CurrentIndexToWrite;   // Current buffer index to write new messages

} CORE_BUFFER_INFORMATION, * PCORE_BUFFER_INFORMATION;



// Global Variable for buffer on all cores
CORE_BUFFER_INFORMATION* CoreBufferInformation;

// Illustrator
/*
A core buffer is like this , it's divided into MaximumPacketsCapacity chucks,
each chunk has PacketChunkSize + sizeof(BUFFER_HEADER) size

			 _________________________
			|      BUFFER_HEADER      |
			|_________________________|
			|						  |
			|           BODY		  |
			|         (Buffer)		  |
			| size = PacketChunkSize  |
			|						  |
			|_________________________|
			|      BUFFER_HEADER      |
			|_________________________|
			|						  |
			|           BODY		  |
			|         (Buffer)		  |
			| size = PacketChunkSize  |
			|						  |
			|_________________________|
			|						  |
			|						  |
			|						  |
			|						  |
			|			.			  |
			|			.			  |
			|			.			  |
			|						  |
			|						  |
			|						  |
			|						  |
			|_________________________|
			|      BUFFER_HEADER      |
			|_________________________|
			|						  |
			|           BODY		  |
			|         (Buffer)		  |
			| size = PacketChunkSize  |
			|						  |
			|_________________________|
			
*/


//////////////////////////////////// Methods \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

BOOLEAN SendBuffer(UINT32 OperationCode, PVOID Buffer, UINT32 BufferLength, UINT32 CoreIndex)
{
	KIRQL OldIRQL;
	
	if (BufferLength > PacketChunkSize)
	{
		// We can't save this huge buffer
		return FALSE;
	}
	// Acquire the lock 
	KeAcquireSpinLock(&CoreBufferInformation[CoreIndex].BufferLock, &OldIRQL);

	// check if the buffer is filled to it's maximum index or not
	if (CoreBufferInformation[CoreIndex].CurrentIndexToWrite > MaximumPacketsCapacity - 1)
	{
		// start from the begining
		CoreBufferInformation[CoreIndex].CurrentIndexToWrite = 0;
	}

	// Compute the start of the buffer header
	BUFFER_HEADER* Header = (BUFFER_HEADER*)((UINT64)CoreBufferInformation[CoreIndex].BufferStartAddress + (CoreBufferInformation[CoreIndex].CurrentIndexToWrite * (PacketChunkSize + sizeof(BUFFER_HEADER))));

	// Set the header
	Header->OpeationNumber = OperationCode;
	Header->BufferLength = BufferLength;
	Header->Valid = TRUE;

	/* Now it's time to fill the buffer */

	// compute the saving index
	PVOID SavingBuffer = ((UINT64)CoreBufferInformation[CoreIndex].BufferStartAddress + (CoreBufferInformation[CoreIndex].CurrentIndexToWrite * (PacketChunkSize + sizeof(BUFFER_HEADER))) + sizeof(BUFFER_HEADER));

	// copy the buffer
	RtlCopyBytes(SavingBuffer, Buffer, BufferLength);

	// Increment the next index to write
	CoreBufferInformation[CoreIndex].CurrentIndexToWrite = CoreBufferInformation[CoreIndex].CurrentIndexToWrite + 1;

	// check if there is any thread in IRP Pending state, so we can complete their request
	if (CoreBufferInformation[CoreIndex].NotifyRecored != NULL)
	{
		/* there is some threads that needs to be completed */

		// Insert dpc to queue
		KeInsertQueueDpc(&CoreBufferInformation[CoreIndex].NotifyRecored->Dpc, CoreBufferInformation[CoreIndex].NotifyRecored, NULL);
	}	
	
	// Release the lock
	KeReleaseSpinLock(&CoreBufferInformation[CoreIndex].BufferLock, OldIRQL);
}

/* return of this function shows whether the read was successfull or not (e.g FALSE shows there's no new buffer available.)*/
BOOLEAN ReadBuffer(UINT32 CoreIndex, PVOID BufferToSaveMessage, UINT32* ReturnedLength) {
	KIRQL OldIRQL;

	// Acquire the lock 
	KeAcquireSpinLock(&CoreBufferInformation[CoreIndex].BufferLock, &OldIRQL);

	// Compute the current buffer to read
	BUFFER_HEADER* Header = (BUFFER_HEADER*)((UINT64)CoreBufferInformation[CoreIndex].BufferStartAddress + (CoreBufferInformation[CoreIndex].CurrentIndexToSend * (PacketChunkSize + sizeof(BUFFER_HEADER))));

	if (!Header->Valid)
	{
		// there is nothing to send
		return FALSE;
	}

	/* If we reached here, means that there is sth to send  */

	// First copy the header 
	RtlCopyBytes(BufferToSaveMessage, &Header->OpeationNumber, sizeof(UINT32));
	
	// Second, save the buffer contents
	PVOID SendingBuffer = ((UINT64)CoreBufferInformation[CoreIndex].BufferStartAddress + (CoreBufferInformation[CoreIndex].CurrentIndexToSend * (PacketChunkSize + sizeof(BUFFER_HEADER))) + sizeof(BUFFER_HEADER));
	PVOID SavingAddress = ((UINT64)BufferToSaveMessage + sizeof(UINT32)); // Because we want to pass the header of usermode header
	RtlCopyBytes(SavingAddress, SendingBuffer, Header->BufferLength);

	// Finally, set the current index to invalid as we sent it
	Header->Valid = FALSE;

	// Set the length to show as the ReturnedByted in usermode ioctl funtion + size of header
	*ReturnedLength = Header->BufferLength + sizeof(UINT32);

	
	// Last step is to clear the current buffer (we can't do it once when CurrentIndexToSend is zero because
	// there might be multiple messages on the start of the queue that didn't read yet)
	// we don't free the header
	RtlZeroMemory(SendingBuffer, Header->BufferLength);

	// Check to see whether we passed the index or not
	if (CoreBufferInformation[CoreIndex].CurrentIndexToSend > MaximumPacketsCapacity - 2)
	{
		CoreBufferInformation[CoreIndex].CurrentIndexToSend = 0;
	}
	else
	{
		// Increment the next index to read
		CoreBufferInformation[CoreIndex].CurrentIndexToSend = CoreBufferInformation[CoreIndex].CurrentIndexToSend + 1;
	}

	// check if there is any NotifyRecored available so we can free it
	if (CoreBufferInformation[CoreIndex].NotifyRecored != NULL)
	{
		// set notify routine to null
		CoreBufferInformation[CoreIndex].NotifyRecored = NULL;
	}
	// Release the lock
	KeReleaseSpinLock(&CoreBufferInformation[CoreIndex].BufferLock, OldIRQL);

}

/* return of this function shows whether the read was successfull or not (e.g FALSE shows there's no new buffer available.)*/
BOOLEAN CheckForNewMessage(UINT32 CoreIndex) {
	KIRQL OldIRQL;

	// Compute the current buffer to read
	BUFFER_HEADER* Header = (BUFFER_HEADER*)((UINT64)CoreBufferInformation[CoreIndex].BufferStartAddress + (CoreBufferInformation[CoreIndex].CurrentIndexToSend * (PacketChunkSize + sizeof(BUFFER_HEADER))));

	if (!Header->Valid)
	{
		// there is nothing to send
		return FALSE;
	}

	/* If we reached here, means that there is sth to send  */
	return TRUE;
}

// Send string messages and tracing for logging and monitoring
BOOLEAN SendMessageToQueue(UINT32 OperationCode, UINT32 CoreIndex, const char* Fmt, ...)
{
	NTSTATUS Status = STATUS_SUCCESS;
	va_list ArgList;
	size_t WrittenSize;
	char LogMessage[PacketChunkSize];

	// It's actually not necessary to use -1 but because user-mode code might assume a null-terminated buffer so
	// it's better to use - 1
	va_start(ArgList, Fmt);
	Status = RtlStringCchVPrintfA(LogMessage, PacketChunkSize - 1, Fmt, ArgList);
	va_end(ArgList);

	RtlStringCchLengthA(LogMessage, PacketChunkSize - 1, &WrittenSize);


	if (LogMessage[0] == '\0') {

		// nothing to write
		DbgBreakPoint();
		return FALSE;
	}

	if (Status == STATUS_SUCCESS)
	{
		return SendBuffer(OperationCode, LogMessage, WrittenSize, CoreIndex);
	}

	return FALSE;
}
// =====================================================================

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, EventCreate)
#pragma alloc_text (PAGE, EventClose)
#pragma alloc_text (PAGE, EventUnload)
#endif

_Use_decl_annotations_ NTSTATUS DriverEntry(PDRIVER_OBJECT  DriverObject, PUNICODE_STRING RegistryPath)
{
	PDEVICE_OBJECT      DeviceObject;
	UNICODE_STRING      NtDeviceName;
	UNICODE_STRING      SymbolicLinkName;
	NTSTATUS            Status;

	UNREFERENCED_PARAMETER(RegistryPath);

	// Opt-in to using non-executable pool memory on Windows 8 and later.
	// https://msdn.microsoft.com/en-us/library/windows/hardware/hh920402(v=vs.85).aspx
	ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

	// Create the device object
	RtlInitUnicodeString(&NtDeviceName, NTDEVICE_NAME_STRING);

	Status = IoCreateDevice(DriverObject,               // DriverObject
		0,                          // DeviceExtensionSize
		&NtDeviceName,              // DeviceName
		FILE_DEVICE_UNKNOWN,        // DeviceType
		FILE_DEVICE_SECURE_OPEN,    // DeviceCharacteristics
		FALSE,                      // Not Exclusive
		&DeviceObject               // DeviceObject
	);

	if (!NT_SUCCESS(Status)) {
		DebugPrint(("\tIoCreateDevice returned 0x%x\n", Status));
		return(Status);
	}

	// Set up dispatch entry points for the driver.
	DriverObject->MajorFunction[IRP_MJ_CREATE] = EventCreate;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = EventClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = EventDispatchIoControl;
	DriverObject->DriverUnload = EventUnload;

	// Create a symbolic link for userapp to interact with the driver.
	RtlInitUnicodeString(&SymbolicLinkName, SYMBOLIC_NAME_STRING);
	Status = IoCreateSymbolicLink(&SymbolicLinkName, &NtDeviceName);

	if (!NT_SUCCESS(Status)) {
		IoDeleteDevice(DeviceObject);
		DebugPrint(("\tIoCreateSymbolicLink returned 0x%x\n", Status));
		return(Status);
	}

	// Establish user-buffer access method.
	DeviceObject->Flags |= DO_BUFFERED_IO;

	ASSERT(NT_SUCCESS(Status));
	return Status;
}

_Use_decl_annotations_ VOID EventUnload(PDRIVER_OBJECT DriverObject)
{

	PDEVICE_OBJECT deviceObject;
	UNICODE_STRING symbolicLinkName;

	deviceObject = DriverObject->DeviceObject;

	// Delete the user-mode symbolic link and deviceobjct.
	RtlInitUnicodeString(&symbolicLinkName, SYMBOLIC_NAME_STRING);
	IoDeleteSymbolicLink(&symbolicLinkName);
	IoDeleteDevice(deviceObject);
}

_Use_decl_annotations_ NTSTATUS EventCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	NTSTATUS Status;
	int CoreCount;

	Status = STATUS_SUCCESS;
	PAGED_CODE();

	CoreCount = KeQueryActiveProcessorCount(0);
	// Initialize buffers for trace message and data messages
	CoreBufferInformation = ExAllocatePoolWithTag(NonPagedPool, sizeof(CORE_BUFFER_INFORMATION)* CoreCount, TAG);

	if (!CoreBufferInformation)
	{
		Status = STATUS_INSUFFICIENT_RESOURCES;
		goto End;
	}

	// Zeroing the memory
	RtlSecureZeroMemory(CoreBufferInformation, sizeof(CORE_BUFFER_INFORMATION) * CoreCount);

	// Allocate buffer for messages and initialize the core buffer information 
	for (int i = 0; i < CoreCount; i++)
	{
		// set the core index
		CoreBufferInformation[i].CoreIndex = i;

		// initialize the lock
		KeInitializeSpinLock(&CoreBufferInformation[i].BufferLock);

		// allocate the buffer
		CoreBufferInformation[i].BufferStartAddress = ExAllocatePoolWithTag(NonPagedPool, BufferSize, TAG);

		if (!CoreBufferInformation[i].BufferStartAddress)
		{
			Status = STATUS_INSUFFICIENT_RESOURCES;
			goto End;
		}

		// Zeroing the buffer
		RtlSecureZeroMemory(CoreBufferInformation[i].BufferStartAddress, BufferSize);

		// Set the end address
		CoreBufferInformation[i].BufferEndAddress = (UINT64)CoreBufferInformation[i].BufferStartAddress + BufferSize;
		
	}

	End :
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;

}

_Use_decl_annotations_ NTSTATUS EventClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	int CoreCount;

	// Deallocate every buffers
	CoreCount = KeQueryActiveProcessorCount(0);

	// de-allocate buffer for messages and initialize the core buffer information 
	for (int i = 0; i < CoreCount; i++)
	{
		// Free each core-specific buffers
		ExFreePoolWithTag(CoreBufferInformation[i].BufferStartAddress, TAG);
	}

	// de-allocate buffers for trace message and data messages
	ExFreePoolWithTag(CoreBufferInformation, TAG);

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;

}
int counter = 0;
_Use_decl_annotations_ NTSTATUS EventDispatchIoControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	counter++;

	PIO_STACK_LOCATION  IrpStack;
	PREGISTER_EVENT RegisterEvent;
	NTSTATUS    Status;

	IrpStack = IoGetCurrentIrpStackLocation(Irp);

	switch (IrpStack->Parameters.DeviceIoControl.IoControlCode)
	{
	case IOCTL_REGISTER_EVENT:

		// First validate the parameters.
		if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < SIZEOF_REGISTER_EVENT || Irp->AssociatedIrp.SystemBuffer == NULL) {
			Status = STATUS_INVALID_PARAMETER;
			break;
		}

		RegisterEvent = (PREGISTER_EVENT)Irp->AssociatedIrp.SystemBuffer;

		switch (RegisterEvent->Type) {
		case IRP_BASED:
			// change the following line
			SendMessageToQueue(0x85, 0, "Sina's answer is %s [%d] and %s - + 0x%x tttuuii", "Hello", counter, "Second Hello",0x1234);
			// end changes
			Status = RegisterIrpBasedNotification(DeviceObject, Irp);
			break;
		case EVENT_BASED:
			Status = RegisterEventBasedNotification(DeviceObject, Irp);
			break;
		default:
			ASSERTMSG("\tUnknow notification type from user-mode\n", FALSE);
			Status = STATUS_INVALID_PARAMETER;
			break;
		}
		break;

	default:
		ASSERT(FALSE);  // should never hit this
		Status = STATUS_NOT_IMPLEMENTED;
		break;
	}

	if (Status != STATUS_PENDING) {
		Irp->IoStatus.Status = Status;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}

	return Status;
}

_Use_decl_annotations_ VOID NotifyUsermodeCallback(PKDPC Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
	PNOTIFY_RECORD NotifyRecord;
	PIRP Irp;

	UNREFERENCED_PARAMETER(Dpc);
	UNREFERENCED_PARAMETER(SystemArgument1);
	UNREFERENCED_PARAMETER(SystemArgument2);

	NotifyRecord = DeferredContext;

	ASSERT(NotifyRecord != NULL); // can't be NULL
	_Analysis_assume_(NotifyRecord != NULL);

	switch (NotifyRecord->Type) 
	{

	case IRP_BASED:
		Irp = NotifyRecord->Message.PendingIrp;
		if (Irp != NULL) {

			PCHAR OutBuff; // pointer to output buffer
			ULONG InBuffLength; // Input buffer length
			ULONG OutBuffLength; // Output buffer length
			PIO_STACK_LOCATION IrpSp;

			IrpSp = IoGetCurrentIrpStackLocation(Irp);
			InBuffLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
			OutBuffLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

			if (!InBuffLength || !OutBuffLength)
			{
				Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
				IoCompleteRequest(Irp, IO_NO_INCREMENT);
				break;
			}

			// Check again that SystemBuffer is not null
			if (!Irp->AssociatedIrp.SystemBuffer)
			{
				// Buffer is invalid
				return;
			}

			OutBuff = Irp->AssociatedIrp.SystemBuffer;

			UINT32 Length = 0;

			// Read Buffer might be empty (nothing to send)
			if (!ReadBuffer(0, OutBuff, &Length))
			{
				// we have to return here as there is nothing to send here
				return;
			}
			
			Irp->IoStatus.Information = Length;


			Irp->IoStatus.Status = STATUS_SUCCESS;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
		}
		break;

	case EVENT_BASED:

		// Signal the Event created in user-mode.
		KeSetEvent(NotifyRecord->Message.Event, 0, FALSE);

		// Dereference the object as we are done with it.
		ObDereferenceObject(NotifyRecord->Message.Event);

		break;

	default:
		ASSERT(FALSE);
		break;
	}

	if (NotifyRecord != NULL) {
		ExFreePoolWithTag(NotifyRecord, TAG);
	}
}

_Use_decl_annotations_ NTSTATUS RegisterIrpBasedNotification(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	PNOTIFY_RECORD NotifyRecord;
	PIO_STACK_LOCATION IrpStack;
	KIRQL   OOldIrql;
	PREGISTER_EVENT RegisterEvent;

	// check if current core has another thread with pending IRP, if no then put the current thread to pending
	// otherwise return and complete thread with STATUS_SUCCESS as there is another thread waiting for message

	//int GetCurrentCoreIndex = KeGetCurrentProcessorNumberEx(0);
	int GetCurrentCoreIndex = 0;

	if (CoreBufferInformation[GetCurrentCoreIndex].NotifyRecored == NULL)
	{
		IrpStack = IoGetCurrentIrpStackLocation(Irp);
		RegisterEvent = (PREGISTER_EVENT)Irp->AssociatedIrp.SystemBuffer;

		// Allocate a record and save all the event context.
		NotifyRecord = ExAllocatePoolWithQuotaTag(NonPagedPool, sizeof(NOTIFY_RECORD), TAG);

		if (NULL == NotifyRecord) {
			return  STATUS_INSUFFICIENT_RESOURCES;
		}

		NotifyRecord->Type = IRP_BASED;
		NotifyRecord->Message.PendingIrp = Irp;

		KeInitializeDpc(&NotifyRecord->Dpc, // Dpc
			NotifyUsermodeCallback,     // DeferredRoutine
			NotifyRecord        // DeferredContext
		);

		IoMarkIrpPending(Irp);


		// Set the notify routine to the global structure
		CoreBufferInformation[GetCurrentCoreIndex].NotifyRecored = NotifyRecord;

		// check for new message
		if (CheckForNewMessage(GetCurrentCoreIndex))
		{
			// Insert dpc to queue
			KeInsertQueueDpc(&NotifyRecord->Dpc, NotifyRecord, NULL);
		}

		// We will return pending as we have marked the IRP pending.
		return STATUS_PENDING;
	}
	else
	{
		return STATUS_SUCCESS;
	}
}

_Use_decl_annotations_ NTSTATUS RegisterEventBasedNotification(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	PNOTIFY_RECORD NotifyRecord;
	NTSTATUS Status;
	PIO_STACK_LOCATION IrpStack;
	PREGISTER_EVENT RegisterEvent;
	KIRQL OldIrql;

	IrpStack = IoGetCurrentIrpStackLocation(Irp);
	RegisterEvent = (PREGISTER_EVENT)Irp->AssociatedIrp.SystemBuffer;

	// Allocate a record and save all the event context.
	NotifyRecord = ExAllocatePoolWithQuotaTag(NonPagedPool, sizeof(NOTIFY_RECORD), TAG);

	if (NULL == NotifyRecord) {
		return  STATUS_INSUFFICIENT_RESOURCES;
	}

	NotifyRecord->Type = EVENT_BASED;

	KeInitializeDpc(&NotifyRecord->Dpc, // Dpc
		NotifyUsermodeCallback,     // DeferredRoutine
		NotifyRecord        // DeferredContext
	);

	// Get the object pointer from the handle. Note we must be in the context of the process that created the handle.
	Status = ObReferenceObjectByHandle(RegisterEvent->hEvent,
		SYNCHRONIZE | EVENT_MODIFY_STATE,
		*ExEventObjectType,
		Irp->RequestorMode,
		&NotifyRecord->Message.Event,
		NULL
	);

	if (!NT_SUCCESS(Status)) {

		DebugPrint(("\tUnable to reference User-Mode Event object, Error = 0x%x\n", Status));
		ExFreePoolWithTag(NotifyRecord, TAG);
		return Status;
	}

	// Insert dpc to the queue
	KeInsertQueueDpc(&NotifyRecord->Dpc, NotifyRecord, NULL);

	return STATUS_SUCCESS;
}