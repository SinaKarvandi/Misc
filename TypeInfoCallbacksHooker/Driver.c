/*++

Module Name:

	driver.c

Abstract:

	This file contains the driver entry points and callbacks.

Environment:

	Kernel-mode Driver Framework

--*/

#include "driver.h"
#include "Header.h"

UINT64 CallbacksList;

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#endif


typedef unsigned int DWORD;


int OkayToCloseProcedure_Hook(
	PEPROCESS Process,
	DWORD DW,
	HANDLE Handle,
	KPROCESSOR_MODE PreviousMode)
{
	if (PreviousMode == KernelMode)
		DbgPrint("Attempt to close the handle : %x to a process opened by the kernel process : %s\n", Handle, (PUCHAR)Process + 0x450 /*+0x450 ImageFileName    : [15] UChar*/);
	else
		DbgPrint("Attempt to close the handle : %x to a process opened by the usermode process : %s\n", Handle, (PUCHAR)Process + 0x450/*+0x450 ImageFileName    : [15] UChar*/);
	return 1;
}

typedef int(*DumpProcedure)(PVOID Object, OBJECT_DUMP_CONTROL* DumpControl);
DumpProcedure _DumpProcedure;

int DumpProcedure_Hook(
	PVOID Object,
	OBJECT_DUMP_CONTROL* DumpControl)
{
	DbgPrint("[*] DumpProcedure called for object : 0x%llx \n", Object);
	return _DumpProcedure(Object, DumpControl);
}

typedef int(*OpenProcedure)(OB_OPEN_REASON OpenReason, CHAR AccessMode, PEPROCESS TargetProcess, PVOID Object, PULONG GrantedAccess, ULONG HandleCount);
OpenProcedure _OpenProcedure;

int OpenProcedure_Hook(
	OB_OPEN_REASON OpenReason,
	CHAR AccessMode,
	PEPROCESS TargetProcess,
	PVOID Object,
	PULONG GrantedAccess,
	ULONG HandleCount)
{
	DbgPrint("[*] OpenProcedure called for object : 0x%llx , TargetProcess : %s , Access Mode : %d , Granted Access : %x\n",
		Object, (PUCHAR)TargetProcess + 0x450, AccessMode, GrantedAccess);

	return _OpenProcedure(OpenReason, AccessMode, TargetProcess, Object, GrantedAccess, HandleCount);
}

typedef int(*CloseProcedure)(PEPROCESS Process, PVOID Object, ULONG ProcessHandleCount, ULONG SystemHandleCount);
CloseProcedure _CloseProcedure;

int CloseProcedure_Hook(
	PEPROCESS Process,
	PVOID Object,
	ULONG ProcessHandleCount,
	ULONG SystemHandleCount)
{
	DbgPrint("[*] CloseProcedure called for object : 0x%llx, Process : %s \n", Object, (PUCHAR)Process + 0x450);
	return _CloseProcedure(Process, Object, ProcessHandleCount, SystemHandleCount);
}

typedef int(*DeleteProcedure)(PVOID Object);
DeleteProcedure _DeleteProcedure;

int DeleteProcedure_Hook(
	PVOID Object)
{
	DbgPrint("[*] DeleteProcedure called for object : 0x%llx \n", Object);
	return _DeleteProcedure(Object);
}

typedef int(*ParseProcedure)(PVOID ParseObject, PVOID ObjectType, PACCESS_STATE AccessState, CHAR AccessMode, ULONG Attributes, UNICODE_STRING* CompleteName, UNICODE_STRING* RemainingName, PVOID Context, SECURITY_QUALITY_OF_SERVICE* SecurityQos, OB_EXTENDED_PARSE_PARAMETERS* ExtendedParameters, PVOID* Object);
ParseProcedure _ParseProcedure;

int ParseProcedure_Hook(
	PVOID ParseObject,
	PVOID ObjectType,
	PACCESS_STATE AccessState,
	CHAR AccessMode,
	ULONG Attributes,
	UNICODE_STRING* CompleteName,
	UNICODE_STRING* RemainingName,
	PVOID Context,
	SECURITY_QUALITY_OF_SERVICE* SecurityQos,
	OB_EXTENDED_PARSE_PARAMETERS* ExtendedParameters,
	PVOID* Object)
{
	DbgPrint("[*] ParseProcedure called for ParseObject : 0x%llx \n", ParseObject);
	return _ParseProcedure(ParseObject, ObjectType, AccessState, AccessMode, Attributes, CompleteName, RemainingName, Context, SecurityQos, ExtendedParameters, Object);
}


typedef int(*SecurityProcedure)(PVOID Object, SECURITY_OPERATION_CODE OperationCode, PULONG SecurityInformation, PVOID SecurityDescriptor, PULONG CapturedLength, PVOID* ObjectsSecurityDescriptor, POOL_TYPE PoolType, PGENERIC_MAPPING GenericMapping, CHAR Mode);
SecurityProcedure _SecurityProcedure;

int SecurityProcedure_Hook(
	PVOID Object,
	SECURITY_OPERATION_CODE OperationCode,
	PULONG SecurityInformation,
	PVOID SecurityDescriptor,
	PULONG CapturedLength,
	PVOID* ObjectsSecurityDescriptor,
	POOL_TYPE PoolType,
	PGENERIC_MAPPING GenericMapping,
	CHAR Mode)
{
	DbgPrint("[*] SecurityProcedure called for Object : 0x%llx \n", Object);
	return _SecurityProcedure(Object, OperationCode, SecurityInformation, SecurityDescriptor, CapturedLength, ObjectsSecurityDescriptor, PoolType, GenericMapping, Mode);
}

typedef int(*QueryNameProcedure)(PVOID Object, UCHAR HasObjectName, POBJECT_NAME_INFORMATION ObjectNameInfo, ULONG Length, PULONG* ReturnLength, CHAR Mode);
QueryNameProcedure _QueryNameProcedure;

int QueryNameProcedure_Hook(
	PVOID Object,
	UCHAR HasObjectName,
	POBJECT_NAME_INFORMATION ObjectNameInfo,
	ULONG Length,
	PULONG* ReturnLength,
	CHAR Mode)
{
	DbgPrint("[*] QueryNameProcedure called for object : 0x%llx \n", Object);
	return _QueryNameProcedure(Object, HasObjectName, ObjectNameInfo, Length, ReturnLength, Mode);
}


VOID DrvUnload(PDRIVER_OBJECT  DriverObject)
{
	UNICODE_STRING usDosDeviceName;


	PUCHAR ObjectType;
	//ObjectType = ObGetObjectType(PsGetCurrentProcess());
	ObjectType = (PUCHAR)*PsProcessType;
	if (*(INT64*)(ObjectType + 0xa8) == (INT64)OkayToCloseProcedure_Hook)
		*(INT64*)(ObjectType + 0xa8) = NULL;
	DbgPrint("[+] Hook Deleted for the Process Object\n");


	DbgPrint("[*] DrvUnload Called.");
	RtlInitUnicodeString(&usDosDeviceName, L"\\DosDevices\\TypeInfoCallbacksHooker");
	IoDeleteSymbolicLink(&usDosDeviceName);
	IoDeleteDevice(DriverObject->DeviceObject);

}
NTSTATUS DrvUnsupported(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	DbgPrint("[*] This function is not supported :( !");

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS
DriverEntry(
	_In_ PDRIVER_OBJECT  DriverObject,
	_In_ PUNICODE_STRING RegistryPath
)
{
	NTSTATUS NtStatus = STATUS_SUCCESS;
	UINT64 uiIndex = 0;
	PDEVICE_OBJECT pDeviceObject = NULL;
	UNICODE_STRING usDriverName, usDosDeviceName;

	DbgPrint("[*] DriverEntry Called.");

	RtlInitUnicodeString(&usDriverName, L"\\Device\\TypeInfoCallbacksHooker");

	RtlInitUnicodeString(&usDosDeviceName, L"\\DosDevices\\TypeInfoCallbacksHooker");

	NtStatus = IoCreateDevice(DriverObject, 0, &usDriverName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &pDeviceObject);



	if (NtStatus == STATUS_SUCCESS)
	{
		DbgPrint("[*] Setting Devices major functions.");

		for (uiIndex = 0; uiIndex < IRP_MJ_MAXIMUM_FUNCTION; uiIndex++)
			DriverObject->MajorFunction[uiIndex] = DrvUnsupported;

		DriverObject->DriverUnload = DrvUnload;
		IoCreateSymbolicLink(&usDosDeviceName, &usDriverName);
	}
	else {
		DbgPrint("[*] There was some errors in creating device.");
	}

	PUCHAR ProcessObjectType;
	DbgPrint("[*] Hooking The Process Object's OkayToCloseProcedure Callback\n");
	DbgPrint("[*] Every attempt to close a handle to a process will be displayed\n");


	/* Get the Process Object Type (OBJECT_TYPE) structure */

	// ProcessObjectType = ObGetObjectType(PsGetCurrentProcess());
	ProcessObjectType = (PUCHAR)*PsProcessType;
	DbgPrint("[*] Process Object Type Structure at : %p\n", ProcessObjectType);

	// Allocating a buffer to store the previous callbacks pointer (if exists)
	// 8 Callbacks * 8 Bytes for each pointer
	CallbacksList = ExAllocatePoolWithTag(NonPagedPool, 8 * 8, 0x41414141);



	/*****************************************************************************************************************/
	/* DumpProcedure_Hook */
	if (*(INT64*)(ProcessObjectType + 0x70) != NULL) {

		// Store the previous pointer
		*(INT64*)(CallbacksList + 0) = *(INT64*)(ProcessObjectType + 0x70);
		_DumpProcedure = *(INT64*)(ProcessObjectType + 0x70);

		// Save to pointer to new hook address
		*(INT64*)(ProcessObjectType + 0x70) = (INT64)DumpProcedure_Hook;

		DbgPrint("[*] DumpProcedure Hook Done !!\n");
	}
	else
	{
		DbgPrint("[*] DumpProcedure Hook Failed");
	}
	/*****************************************************************************************************************/
	/* OpenProcedure_Hook */
	if (*(INT64*)(ProcessObjectType + 0x78) != NULL) {

		// Store the previous pointer
		*(INT64*)(CallbacksList + 0x8) = *(INT64*)(ProcessObjectType + 0x78);
		_OpenProcedure = *(INT64*)(ProcessObjectType + 0x78);


		// Save to pointer to new hook address
		*(INT64*)(ProcessObjectType + 0x78) = (INT64)OpenProcedure_Hook;

		DbgPrint("[*] OpenProcedure Hook Done !!\n");
	}
	else
	{
		DbgPrint("[*] OpenProcedure Hook Failed");
	}
	/*****************************************************************************************************************/
	/* CloseProcedure_Hook */
	if (*(INT64*)(ProcessObjectType + 0x80) != NULL) {

		// Store the previous pointer
		*(INT64*)(CallbacksList + 0x10) = *(INT64*)(ProcessObjectType + 0x80);
		_CloseProcedure = *(INT64*)(ProcessObjectType + 0x80);


		// Save to pointer to new hook address
		*(INT64*)(ProcessObjectType + 0x80) = (INT64)CloseProcedure_Hook;

		DbgPrint("[*] CloseProcedure Hook Done !!\n");
	}
	else
	{
		DbgPrint("[*] CloseProcedure Hook Failed");
	}
	/*****************************************************************************************************************/
	/* DeleteProcedure_Hook */
	if (*(INT64*)(ProcessObjectType + 0x88) != NULL) {

		// Store the previous pointer
		*(INT64*)(CallbacksList + 0x18) = *(INT64*)(ProcessObjectType + 0x88);
		_DeleteProcedure = *(INT64*)(ProcessObjectType + 0x88);

		// Save to pointer to new hook address
		*(INT64*)(ProcessObjectType + 0x88) = (INT64)DeleteProcedure_Hook;

		DbgPrint("[*] DeleteProcedure Hook Done !!\n");
	}
	else
	{
		DbgPrint("[*] DeleteProcedure Hook Failed");
	}
	/*****************************************************************************************************************/
	/* ParseProcedure & ParseProcedureEx _Hook */
	if (*(INT64*)(ProcessObjectType + 0x90) != NULL) {

		// Store the previous pointer
		*(INT64*)(CallbacksList + 0x20) = *(INT64*)(ProcessObjectType + 0x90);
		_ParseProcedure = *(INT64*)(ProcessObjectType + 0x90);

		// Save to pointer to new hook address
		*(INT64*)(ProcessObjectType + 0x90) = (INT64)ParseProcedure_Hook;

		DbgPrint("[*] ParseProcedure Hook Done !!\n");
	}
	else
	{
		DbgPrint("[*] ParseProcedure Hook Failed");
	}
	/*****************************************************************************************************************/
	/* SecurityProcedure_Hook */
	if (*(INT64*)(ProcessObjectType + 0x98) != NULL) {

		// Store the previous pointer
		*(INT64*)(CallbacksList + 0x28) = *(INT64*)(ProcessObjectType + 0x98);
		_SecurityProcedure = *(INT64*)(ProcessObjectType + 0x98);

		// Save to pointer to new hook address
		*(INT64*)(ProcessObjectType + 0x98) = (INT64)SecurityProcedure_Hook;

		DbgPrint("[*] SecurityProcedure Hook Done !!\n");
	}
	else
	{
		DbgPrint("[*] SecurityProcedure Hook Failed");
	}
	/*****************************************************************************************************************/
	/* QueryNameProcedure_Hook */
	if (*(INT64*)(ProcessObjectType + 0xa0) != NULL) {

		// Store the previous pointer
		*(INT64*)(CallbacksList + 0x30) = *(INT64*)(ProcessObjectType + 0xa0);
		_QueryNameProcedure = *(INT64*)(ProcessObjectType + 0xa0);

		// Save to pointer to new hook address
		*(INT64*)(ProcessObjectType + 0xa0) = (INT64)QueryNameProcedure_Hook;

		DbgPrint("[*] QueryNameProcedure Hook Done !!\n");
	}
	else
	{
		DbgPrint("[*] QueryNameProcedure Hook Failed");
	}

	/*****************************************************************************************************************/
	/* Set the OkayToCloseProcedure function pointer from the OBJECT_TYPE_INITIALIZER structure to the hook function */
	if (*(INT64*)(ProcessObjectType + 0xa8) == NULL) {
		*(INT64*)(ProcessObjectType + 0xa8) = (INT64)OkayToCloseProcedure_Hook;
		DbgPrint("[*] OkayToCloseProcedure Hook Done !!\n");
	}
	else
	{
		DbgPrint("[*] OkayToCloseProcedure Hook Failed");
	}
	/*****************************************************************************************************************/


	return NtStatus;
}

