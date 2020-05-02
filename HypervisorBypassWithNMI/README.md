This file contains a possible bypass to Hypervisors that virtualize an already running system using NMI.

The POC is derived from : https://www.unknowncheats.me/forum/c-and-c-/390593-vm-escape-via-nmi.html#post2772568

When you're in VMX Root, an attacker can send an NMI and that NMI will be serviced in VMX ROOT. If you use a same IDT for vmx root and vmx non root then you probably have this problem.
