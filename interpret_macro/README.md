# Declarative memory views and writes in C

About a year ago I decided that I need to create a true low-level lisp.  
And although I made a design document it is yet to be done.

One idea in a language I had is declarative and local views over memory.  
So that you can effectively manipulate the memory without writing ugly code which shifts numbers on different offsets.

For example consider some data pointer and  
the random task to return B and R and packed as 16 bit integer.
```
(reg data (((byte R) _ (byte B)))
  (let stack ((int16 br))
    [br (B R)]))
```

Or idk reverse R G and B (that is actually easier).
```
(reg data (((byte R) (byte G) (byte B)) color)
    [color (B G R)])
```

But until I have done such a language one need to work in C or C++.  
Where such stuff is not so trivial. But it turns out I can achieve almost the same level of declarativeness.

Lo and behold

```c
#define interpret(type_name, ptr, ...) \
    struct __attribute__((packed, may_alias)) type_name { __VA_ARGS__ } *type_name = \
    (struct type_name *)(ptr)
```

It is actually pretty portable (GCC, LLVM support).  
The effect of packed can be achieved in MSVC and may\_alias is the default state of Microsoft's C compiler.

So the final version is:
```c
#if defined(_MSC_VER)
    // MSVC
    #define interpret(type_name, ptr, ...) \
        __pragma(pack(push, 1))            \
        struct type_name { __VA_ARGS__ };  \
        __pragma(pack(pop))                \
        struct type_name *type_name = (struct type_name *)(ptr)
#else
    // GCC, Clang
    #define interpret(type_name, ptr, ...) \
        struct __attribute__((packed, may_alias)) type_name { __VA_ARGS__ }; \
        struct type_name *type_name = (struct type_name *)(ptr)
#endif
```

Now what it really does? Well, it defines a type and assigns whatever pointer you have passed as the new struct.

There might be two problems with this (which we solve with the declarations):
1. **Padding**
     
   As you know in C and C++ if you define a struct there are certain alignment rules.  
   The compiler makes your code faster.  
   By making memory accesses more machine friendly.  
   Thus padding your structures so that they match machine word boundaries (when individual fields are accessed).
     
   We do not like this. Since we want to view memory explicitly here byte by byte.  
   Because it is a memory viewing / parsing happening here. We read from raw memory (maybe from file or net packet).  
2. **Aliasing**
     
   Again when it comes to raw memory manip C effectievely says it is UB unless you do it with something like `char*`.  
   You have effective type of the memory region and compiler is free to think pointers are not aliased.  
   The way we want them to actually alias.
     
   So we need to disable it and make it work much more like `char *` and may\_alias exactly does it for us.

```c
interpret(color, data, 
  uint8_t R, G, B; 
);
uint16_t rb;
{
  interpret(view_rb, &rb, 
    uint8_t R, B;
  );
  *view_rb = (struct view_rb){color->R, color->B};
}
// rb is 0xdefa 😛
printf("rb = 0x%x\n", rb);
```

Here as you see we define 16 bit integer and then  
we have been able to pack two color bytes into it declaratively.

```c
uint16_t set_br(void* data) {
    uint16_t br;
    interpret(color, data, 
      uint8_t R, _, B; 
    );
    interpret(view_br, &br, 
      uint8_t B, R;
    );
    *view_br = (struct view_br){
      color->B, color->R
    };
    return br;
}

uint16_t set_br_normal(void* data) {
    uint8_t R, G, B; 
    uint8_t* bytes = data;
    R = bytes[0];
    B = bytes[2];
    uint16_t br = 0;
    br = (uint16_t)((R << 8) | B); 
    return br;
}

```

### x86-64 gcc 15.2

I actually really like this one it is very short and concise.  
Exactly what I would have written if I were like writing it manually.

I mean `set_br` function. Just take the byte, just put it into appropriate place.

```asm
set_br:
        movzx   eax, BYTE PTR [rdi+2]
        mov     ah, BYTE PTR [rdi]
        ret
set_br_normal:
        movzx   eax, BYTE PTR [rdi]
        movzx   edx, BYTE PTR [rdi+2]
        sal     eax, 8
        or      eax, edx
        ret
```

### x64 msvc v19

MSVC on x64 basically makes them the same.  
There is no benefit in readability or speed.

Though on lower optimization levels.  
The declarative version may be slightly shorter.

```asm
data$ = 8
set_br  PROC      
        movzx   eax, BYTE PTR [rcx]
        movzx   ecx, BYTE PTR [rcx+2]
        shl     ax, 8
        or      ax, cx
        ret     0
set_br  ENDP

data$ = 8
set_br_normal PROC 
        movzx   eax, BYTE PTR [rcx]
        movzx   ecx, BYTE PTR [rcx+2]
        shl     ax, 8
        or      ax, cx
        ret     0
set_br_normal ENDP
```

### x86-64 clang 22

And so does clang

```asm
set_br:
        movzx   ecx, byte ptr [rdi + 2]
        movzx   eax, byte ptr [rdi]
        shl     eax, 8
        or      eax, ecx
        ret

set_br_normal:
        movzx   ecx, byte ptr [rdi + 2]
        movzx   eax, byte ptr [rdi]
        shl     eax, 8
        or      eax, ecx
        ret
```

## TODO

1. Maybe add some actual real world examples where such thing might be needed.  
That would be hard though, because I do not work in embeded systems or anywhere near network stack.  
I do not even do protocols.

2. What about bit fields?  
   Seems good I guess.
