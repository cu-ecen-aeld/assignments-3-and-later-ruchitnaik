# Faulty Kernal OOPS Analysis

Following is the result for ` echo "hello_world" > /dev/fault ` command when ran in qemu instance  
```
Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000
Mem abort info:
  ESR = 0x96000046
  EC = 0x25: DABT (current EL), IL = 32 bits
  SET = 0, FnV = 0
  EA = 0, S1PTW = 0
Data abort info:
  ISV = 0, ISS = 0x00000046
  CM = 0, WnR = 1
user pgtable: 4k pages, 39-bit VAs, pgdp=000000004206d000
[0000000000000000] pgd=0000000041fea003, p4d=0000000041fea003, pud=0000000041fea003, pmd=0000000000000000
Internal error: Oops: 96000046 [#1] SMP
Modules linked in: scull(O) faulty(O) hello(O)
CPU: 0 PID: 152 Comm: sh Tainted: G           O      5.10.7 #1
Hardware name: linux,dummy-virt (DT)
pstate: 80000005 (Nzcv daif -PAN -UAO -TCO BTYPE=--)
pc : faulty_write+0x10/0x20 [faulty]
lr : vfs_write+0xc0/0x290
sp : ffffffc010bc3db0
x29: ffffffc010bc3db0 x28: ffffff8001ff2580 
x27: 0000000000000000 x26: 0000000000000000 
x25: 0000000000000000 x24: 0000000000000000 
x23: 0000000000000000 x22: ffffffc010bc3e30 
x21: 00000000004c9940 x20: ffffff8001fa5f00 
x19: 0000000000000012 x18: 0000000000000000 
x17: 0000000000000000 x16: 0000000000000000 
x15: 0000000000000000 x14: 0000000000000000 
x13: 0000000000000000 x12: 0000000000000000 
x11: 0000000000000000 x10: 0000000000000000 
x9 : 0000000000000000 x8 : 0000000000000000 
x7 : 0000000000000000 x6 : 0000000000000000 
x5 : ffffff80020177b8 x4 : ffffffc008675000 
x3 : ffffffc010bc3e30 x2 : 0000000000000012 
x1 : 0000000000000000 x0 : 0000000000000000 
Call trace:
 faulty_write+0x10/0x20 [faulty]
 ksys_write+0x6c/0x100
 __arm64_sys_write+0x1c/0x30
 el0_svc_common.constprop.0+0x9c/0x1c0
 do_el0_svc+0x70/0x90
 el0_svc+0x14/0x20
 el0_sync_handler+0xb0/0xc0
 el0_sync+0x174/0x180
Code: d2800001 d2800000 d503233f d50323bf (b900003f) 
---[ end trace ba8f304b1aa96b1b ]---
```  
## Analysis  
Essentially the result give us the debug prints show where the fault has occured.  
```
Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000
```
In the above block the first line shows the reason for occurance of the fault. Here, the fault has occured becuase of dereferening of the null pointer, addressing a virtual address.  
```
Mem abort info:
  ESR = 0x96000046
  EC = 0x25: DABT (current EL), IL = 32 bits
  SET = 0, FnV = 0
  EA = 0, S1PTW = 0
Data abort info:
  ISV = 0, ISS = 0x00000046
  CM = 0, WnR = 1
user pgtable: 4k pages, 39-bit VAs, pgdp=000000004206d000
[0000000000000000] pgd=0000000041fea003, p4d=0000000041fea003, pud=0000000041fea003, pmd=0000000000000000
Internal error: Oops: 96000046 [#1] SMP
Modules linked in: scull(O) faulty(O) hello(O)
CPU: 0 PID: 152 Comm: sh Tainted: G           O      5.10.7 #1
Hardware name: linux,dummy-virt (DT)
pstate: 80000005 (Nzcv daif -PAN -UAO -TCO BTYPE=--)
```
The above block displays the discreption about the mem fault occured. The discreption is specific to the arhchitecure we are builing for. It contains all the details related to memory mapping, modules linked, hardware info etc.  
``` 
pc : faulty_write+0x10/0x20 [faulty] 
```
The above line shows the program counter location which caused mem fault. Evidently the fault occured during the execution of ``` faulty_write() ``` function. It also displays the details about the offset of the function. Hence, we know that we are dereference a NULL pointer from a specfic address offset in faulty_write function. That's where we need to look to solve the fault.  
Analysing the source code for the faulty function we understand that the issue lies in the [line](https://github.com/cu-ecen-aeld/assignment-7-ruchitnaik/blob/6f71da038cb54c17f15339274c7b06e83cdf08c3/misc-modules/faulty.c#L53). It can be seen in the source code below.
```
ssize_t faulty_write (struct file *filp, const char __user *buf, size_t count,
		loff_t *pos)
{
	/* make a simple fault by dereferencing a NULL pointer */
	*(int *)0 = 0;
	return 0;
}
```