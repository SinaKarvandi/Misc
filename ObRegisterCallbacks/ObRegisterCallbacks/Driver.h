/*++

Module Name:

    driver.h

Abstract:

    This file contains the driver definitions.

Environment:

    Kernel-mode Driver Framework

--*/

#include <ntddk.h>
#include <wdf.h>
#include <initguid.h>

// Define funcs
VOID DrvUnload(PDRIVER_OBJECT  DriverObject);

OB_PREOP_CALLBACK_STATUS
PreOperationCallback(
	_In_ PVOID RegistrationContext,
	_Inout_ POB_PRE_OPERATION_INFORMATION PreInfo
);
VOID
PostOperationCallback(
	_In_ PVOID RegistrationContext,
	_In_ POB_POST_OPERATION_INFORMATION PostInfo
);

typedef struct _TD_CALLBACK_PARAMETERS {
	ACCESS_MASK AccessBitsToClear;
	ACCESS_MASK AccessBitsToSet;
}
TD_CALLBACK_PARAMETERS, *PTD_CALLBACK_PARAMETERS;

// TD_CALLBACK_REGISTRATION

typedef struct _TD_CALLBACK_REGISTRATION {

	// Handle returned by ObRegisterCallbacks.
	PVOID RegistrationHandle;

	// If not NULL, filter only requests to open/duplicate handles to this
	// process (or one of its threads).

	PVOID TargetProcess;
	HANDLE TargetProcessId;


	// Currently each TD_CALLBACK_REGISTRATION has at most one process and one
	// thread callback. That is, we can't register more than one callback for
	// the same object type with a single ObRegisterCallbacks call.

	TD_CALLBACK_PARAMETERS ProcessParams;
	TD_CALLBACK_PARAMETERS ThreadParams;

	ULONG RegistrationId;        // Index in the global TdCallbacks array.

}
TD_CALLBACK_REGISTRATION, *PTD_CALLBACK_REGISTRATION;

