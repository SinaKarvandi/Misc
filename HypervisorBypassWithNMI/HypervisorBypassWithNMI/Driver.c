

#include <ntddk.h>

BOOLEAN LoadNmiCallback();

VOID DrvUnload(PDRIVER_OBJECT DriverObject) { return STATUS_SUCCESS; }
NTSTATUS
DriverEntry(_In_ PDRIVER_OBJECT DriverObject,
            _In_ PUNICODE_STRING RegistryPath) {
  NTSTATUS status = STATUS_SUCCESS;

  DriverObject->DriverUnload = DrvUnload;


  //
  // Test in kernel
  //
  LoadNmiCallback();

  return status;
}
