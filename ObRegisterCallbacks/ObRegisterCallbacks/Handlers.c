

#include <ntddk.h>
#include <wdf.h>
#include <initguid.h>
#include "Driver.h"


OB_PREOP_CALLBACK_STATUS
PreOperationCallback(
	_In_ PVOID RegistrationContext,
	_Inout_ POB_PRE_OPERATION_INFORMATION PreInfo
)
{

	ACCESS_MASK AccessBitsToClear = 0;
	ACCESS_MASK AccessBitsToSet = 0;
	ACCESS_MASK InitialDesiredAccess = 0;
	ACCESS_MASK OriginalDesiredAccess = 0;


	PACCESS_MASK DesiredAccess = NULL;

	LPCWSTR ObjectTypeName = NULL;
	LPCWSTR OperationName = NULL;


	ASSERT(PreInfo->CallContext == NULL);

	
	if (PreInfo->ObjectType == *PsProcessType) {

		// Process that tries to open/duplicate the current process handle (process itself)

		if (PreInfo->Object == PsGetCurrentProcess()) {
			DbgPrintEx(
				DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL,
				"PreOperationCallback: ignore process open/duplicate from the protected process itself\n");
		}

		ObjectTypeName = L"PsProcessType";
		AccessBitsToClear = 0;
		AccessBitsToSet = 0;
	}



	else if (PreInfo->ObjectType == *PsThreadType) {
		HANDLE ProcessIdOfTargetThread = PsGetThreadProcessId((PETHREAD)PreInfo->Object);

		
		// Threads which try to open/duplicate from the protected process itself
		if (ProcessIdOfTargetThread == PsGetCurrentProcessId()) {
			DbgPrintEx(
				DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL,
				"PreOperationCallback: ignore thread open/duplicate from the protected process itself\n");
		}

		ObjectTypeName = L"PsThreadType";
		AccessBitsToClear = 0;
		AccessBitsToSet = 0;
	}
	else {
		DbgPrintEx(
			DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
			"PreOperationCallback: unexpected object type\n");

		// Breakpoint if any !
		DbgBreakPoint();
		goto Exit;
	}

	switch (PreInfo->Operation) {
	case OB_OPERATION_HANDLE_CREATE:
		DesiredAccess = &PreInfo->Parameters->CreateHandleInformation.DesiredAccess;
		OriginalDesiredAccess = PreInfo->Parameters->CreateHandleInformation.OriginalDesiredAccess;

		OperationName = L"OB_OPERATION_HANDLE_CREATE";
		break;

	case OB_OPERATION_HANDLE_DUPLICATE:
		DesiredAccess = &PreInfo->Parameters->DuplicateHandleInformation.DesiredAccess;
		OriginalDesiredAccess = PreInfo->Parameters->DuplicateHandleInformation.OriginalDesiredAccess;

		OperationName = L"OB_OPERATION_HANDLE_DUPLICATE";
		break;

	default:
		ASSERT(FALSE);
		break;
	}

	InitialDesiredAccess = *DesiredAccess;

	// Filter only if request made outside of the kernel
	if (PreInfo->KernelHandle != 1) {
		*DesiredAccess &= ~AccessBitsToClear;
		*DesiredAccess |= AccessBitsToSet;
	}

	DbgPrint(
		"PreOperationCallback ,\n"
		"    Client Id:    %p:%p\n"
		"    Object:       %p\n"
		"    Type:         %ls\n"
		"    Operation:    %ls (KernelHandle=%d)\n"
		"    OriginalDesiredAccess: 0x%x\n"
		"    DesiredAccess (in):    0x%x\n"
		"    DesiredAccess (out):   0x%x\n"
		"    =========================================================\n",
		PsGetCurrentProcessId(),
		PsGetCurrentThreadId(),
		PreInfo->Object,
		ObjectTypeName,
		OperationName,
		PreInfo->KernelHandle,
		OriginalDesiredAccess,
		InitialDesiredAccess,
		*DesiredAccess
	);

Exit:

	return OB_PREOP_SUCCESS;
}

VOID
PostOperationCallback(
	_In_ PVOID RegistrationContext,
	_In_ POB_POST_OPERATION_INFORMATION PostInfo
)
{ }
