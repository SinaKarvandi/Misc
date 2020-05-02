#include "HypervisorBypass.h"
#include <ntddk.h>

//
// Global Variables
//

PVOID NmiRegPointer;

BOOLEAN
XxxNmiCallback(IN PVOID Context, IN BOOLEAN Handled) {

  UINT64 VmcsLink = 0;
  UINT64 GuestRip = 0;

  __vmx_vmread(VMCS_LINK_POINTER, &VmcsLink);

  if (VmcsLink != 0) {
    __vmx_vmread(GUEST_RIP, &GuestRip);
    if (GuestRip != NULL) {
      *(UINT64 *)Context = GuestRip;
    }
  }
  return TRUE;
}

ULONG_PTR IpiBroadcastOnCore(ULONG_PTR context) {
  ULONG CurrentCore = KeGetCurrentProcessorIndex();
  for (size_t i = 0; i < 100; i++) {

    //
    // The 0th core is responsible to inject NMI on each core
    //
    if (CurrentCore != 0) {
      int Info[4] = {0};
      __cpuidex(Info, 0, 0);
    } else {

      //
      // In XAPIC mode the ICR is addressed as two 32-bit registers, ICR_LOW
      // (FFE0 0300H) and ICR_HIGH (FFE0 0310H). In x2APIC mode, the ICR uses
      // MSR 830H.
      //

      //
      // System software can place the local APIC in the x2APIC mode by setting
      // the x2APIC mode enable bit(bit 10) in the IA32_APIC_BASE MSR at MSR
      // address 01BH.
      //

      //
      // Read Apic MSR
      //

      UINT64 ICR = __readmsr(MSR_x2APIC_ICR);

      ICR |= ((4 << 8) | (1 << 14) | (3 << 18));

      //
      // Apply the NMI
      //
      __writemsr(MSR_x2APIC_ICR, ICR);

      /*send nmi in x2apic mode.. in xapic mode map lapic in and
                   use mmio to send nmi. lapic is in MSR IA32_APIC_BASE (
                   0x1B ) the default address is 0xFEE00000*/
    }
  }

  return 0;
}

//
// If the NMI Exiting control is 0 and an NMI arrives while in VMX-non-root
// mode, the NMI is delivered to the guest via the guest's IDT. If the NMI
// Exiting control is 1, an NMI causes a VM exit.[Intel SDM, volume 3,
// section 24.6.1, table 24 - 5]
//
BOOLEAN LoadNmiCallback() {

  PVOID BufferAddress;

  BufferAddress = ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, 0x41424344);

  if (BufferAddress == NULL) {
    return FALSE;
  }
  RtlZeroMemory(BufferAddress, PAGE_SIZE);

  DbgPrint("Enabling nmi callback\n");
  NmiRegPointer = KeRegisterNmiCallback(XxxNmiCallback, BufferAddress);

  if (!NmiRegPointer) {
    return FALSE;
  }

  KeIpiGenericCall(IpiBroadcastOnCore, NULL);
  DbgBreakPoint();
  return TRUE;
}

VOID UnloadNmiCallback() {
  if (NmiRegPointer)
    KeDeregisterNmiCallback(NmiRegPointer);
  return;
}
