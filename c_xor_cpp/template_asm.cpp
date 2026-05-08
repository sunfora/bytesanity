template<int Count>
struct AsmLoop {
    static __attribute__((always_inline)) inline void unroll() {
        asm volatile(
            ".pushsection .rodata" "\n"
            ".L_cursed_str%=: .ascii \"C ^ C++ = asm\\n\"" "\n"
            ".popsection" "\n"
            
            "mov $1, %%rax" "\n"
            "mov $1, %%rdi" "\n"
            "lea .L_cursed_str%=(%%rip), %%rsi" "\n"
            "mov $14, %%rdx" "\n"
            "syscall"
            : : : "rax", "rdi", "rsi", "rdx", "rcx", "r11", "memory"
        );
        AsmLoop<Count - 1>::unroll();
    }
};

template<>
struct AsmLoop<0> {
    static __attribute__((always_inline)) inline void unroll() {}
};

template<int ExitCode>
[[noreturn]] void _start_template() {
    asm volatile(
        ".global _start \n"
        "_start:\n\t"
    );
    
    AsmLoop<3>::unroll();

    asm volatile(
        "mov $60, %%rax \n\t"
        "mov %0, %%rdi \n\t"
        "syscall \n\t"
        : : "i" (ExitCode) : "rax", "rdi", "rcx", "r11"
    );
    __builtin_unreachable(); 
}

template void _start_template<42>();
