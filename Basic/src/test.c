int x = 10;  // .data
int arr[5] = {65,66,67,68,69}; // .data
int y;       // .bss
const int z = 0x4142; // .rodata

void _start() {
    y = x;   // dùng cả .data và .bss

    __asm__ (
        "mov $60, %%rax\n"
        "xor %%rdi, %%rdi\n"
        "syscall\n"
        :
        :
        : "%rax", "%rdi"
    );
}
