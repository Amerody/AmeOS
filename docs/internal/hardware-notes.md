# Hardware Notes {#hardware_notes}

Some notes I took from the hardware functionality or ARM architecture

## Exception Vector Tables
The ARM exception vector table defines where the CPU jumps when an exception occurs. It contains 16 entries (4 exception types × 4 sources):

Offset | Exception Source              | Exception Type
-------|--------------------------------|-------------------
0x000  | Current EL with SP0 (unused)  | Synchronous
0x080  |                               | IRQ
0x100  |                               | FIQ
0x180  |                               | SError
0x200  | Current EL with SPx (kernel)  | Synchronous
0x280  |                               | IRQ ← We use this
0x300  |                               | FIQ
0x380  |                               | SError
0x400  | Lower EL (AArch64)            | Synchronous
0x480  |                               | IRQ
0x500  |                               | FIQ
0x580  |                               | SError
0x600  | Lower EL (AArch32)            | Synchronous
0x680  |                               | IRQ
0x700  |                               | FIQ
<<<<<<< HEAD
0x780  |                               | SError
=======
0x780  |                               | SError
Each entry must be 128 bytes (0x80) apart.
>>>>>>> homeserver/Dev
