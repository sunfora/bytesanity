#include <sys/mman.h>
#include <unistd.h>
#include <iostream>
#include <cstring>

extern "C" void print(int value) {
  std::cout << value << std::endl;
}

using byte = unsigned char;

byte set_the_stack[] = {
    0x55,                                                  //     push rbp                  0 bytes
    0x53,                                                  //     movabs rbx                1 bytes
    0x48, 0x89, 0xE5,                                      //     mov rbp, rsp              2 bytes
    0x48, 0x83, 0xE4, 0xF0,                                //     and rsp, -16              5 bytes
                                                           //
    0x48, 0xBB,                                            //     movabs rbx,               9 bytes
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00   //     [64 bit pointer]         11 bytes
};

byte load_and_check[] = {
    0xBF,                                                  //     mov edi,                          0  bytes
          0x00, 0x00, 0x00, 0x00,                          //     [32 bit int]                      1  bytes
    0x81, 0xFF,                                            //     cmp edi                           5  bytes
          0x00, 0x00, 0x00, 0x00,                          //     [32 bit int]                      7  bytes
    0x7C, 0x00                                             //     jl  +offset (copy + body - 6)    11  bytes
};

byte copy[] = {
   0x89, 0xF8,                                             // mov eax, edi (save)
   0x48, 0x8D, 0x35,                                       // lea rsi
          0x00, 0x00, 0x00, 0x00,                          // [displacement = -7 - 2 -load_and_check]
   0x48, 0x8D, 0x3D,                                       // lea rdi
         0x00, 0x00, 0x00, 0x00,                           // [displacement = body + copy - lea - lea - mov // -7 -7 - 2]
   0xB9,                                                   // mov ecx
         0x00, 0x00, 0x00, 0x00,                           // [32 imm = body + copy + load_and_check]
   0xF3, 0xA4,                                             // rep movsb (copy the code after itself)
   0x83, 0x2D,                                             // sub
         0x00, 0x00, 0x00, 0x00,                           // [memory = rip + body + 2 + 1]
         0x05,                                             //  5
   0x89, 0xC7                                              // mov edi, eax (restore)
};

byte body[] = {                                            //                          offset: 
    0xFF, 0xD3,                                            //     call print(edi)          
    0xEB, 0x06,                                            //     jmp +6
    0x48, 0x89, 0xEC,                                      //     mov rsp, rbp            
    0x5B,                                                  //     pop rbx
    0x5D,                                                  //     pop rbp                
    0xC3                                                   //     ret                   
};                                                         //                          

void load(void* vmem) {
    byte* mem = (byte*) vmem;
    size_t offset = 0;

    // load set the stack
    std::memcpy(mem + offset, set_the_stack, sizeof(set_the_stack));
    offset += sizeof(set_the_stack);
    // patch print address
    auto print_address = &print;
    offset -= sizeof(print_address);
    std::memcpy(mem + offset, &print_address, sizeof(print_address));
    offset += sizeof(print_address);


    // load and patch load_and_check
    int initial_value = 100;
    int end_value = -100;
    byte jump_size = sizeof(copy) + sizeof(body) - 6; // 32
    // load load_and_check
    std::memcpy(mem + offset, load_and_check, sizeof(load_and_check));
    // patch initial edi value
    std::memcpy(mem + offset + 1, &initial_value, sizeof(initial_value));
    std::memcpy(mem + offset + 3 + sizeof(initial_value), &end_value, sizeof(end_value));
    offset += sizeof(load_and_check);
    // patch jump relative
    offset -= sizeof(jump_size);
    std::memcpy(mem + offset, &jump_size, sizeof(jump_size));
    offset += sizeof(jump_size);

    // load and patch the copy
    int disp_rsi = -((int) sizeof(load_and_check)) - 7 - 2;
    int disp_rdi = sizeof(copy) + sizeof(body) - 14 - 2;
    int code_size = sizeof(load_and_check) + sizeof(copy) + sizeof(body);
    int edi_pos = sizeof(body) + 2 + 1;

    std::memcpy(mem + offset, copy, sizeof(copy));
    offset += 2;
    offset += 3;
    std::memcpy(mem + offset, &disp_rsi, sizeof(disp_rsi));
    offset += sizeof(disp_rsi);
    offset += 3;
    std::memcpy(mem + offset, &disp_rdi, sizeof(disp_rdi));
    offset += sizeof(disp_rdi);
    offset += 1;
    std::memcpy(mem + offset, &code_size, sizeof(code_size));
    offset += sizeof(code_size);
    offset += 4;
    std::memcpy(mem + offset, &edi_pos, sizeof(edi_pos));
    offset += sizeof(edi_pos) + 2 + 1;
    std::memcpy(mem + offset, body, sizeof(body));
}

template<typename T>
T* get(void* mem, size_t offset) {
  auto bytes = (byte *) mem;
  return (T*) (bytes + offset);
}


int main() {
    // Get system page size
    long page_size = sysconf(_SC_PAGESIZE);

    // Allocate a page of executable memory
    void* exec_mem = mmap(
        nullptr,              // Let the OS choose the address
        page_size,            // One page
        PROT_READ | PROT_WRITE | PROT_EXEC, // Permissions: RWX
        MAP_PRIVATE | MAP_ANONYMOUS,        // Not backed by a file
        -1,                   // No file descriptor
        0                     // Offset
    );

    if (exec_mem == MAP_FAILED) {
        perror("mmap failed");
        return 1;
    }
    
    load(exec_mem);
    
    auto func = (void(*)()) exec_mem;
    
    func();
    func();
    return 0;
}
