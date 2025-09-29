# Replica 

replica is small x86-64 bytecode program which 

1. prints the number
2. writes its own code further into memory to execute
3. decreases the number inside the new instruction!
4. ???
5. PROFIT

---

### What's going on?

Okay this whole project was done on evening on very boring day.
It is a single C++ file which works kind of like a linker/loader to the very low level code.

For byte code, like the thing that your compiler produces, and then what is fed your machine.
And the machine consumes it and executes. Instruction by instruction.

I never had done this before by myself. 
I only had seen something like that done by Casey Muratori in his Handmade Hero series.
The function he did was kind of really simple. Something akin to

```c++
int five() {
  return 5;
}
```

He compiled it, then in the debugger took the bytes out.
Wrote the thing into the array. Allocated the chunk of memory on windows using simple kernel call.
Basically wrote the function by address, got the pointer. And called it.
And checked in the debugger that yep it returns 5.

Very great. Let's move on.

---

So... that was interesting. Because the thing he did is kinda what the JIT compiler does.
But he did it like by hand. 

So I looked at that thing and decided... That... 
Why not try something insane like that?

I mean I was always a fan of lisp. Self modifying code and things like that.
So I decided that I would do something which does something with its own code.
But does it in way simple enough so that I don't be bothered in very delicate details.

### Okay so what can it be?

You know the first idea that came to my mind is that... 
Let's just load something into the register and print it. 
And then somehow maybe patch the instruction itself. That loads into that register.
Change the immediate value right when you load it into the register.

That would be the easiest thing to do.
I am talking about 
```asm
mov edi, 17
```

It is probably encoded that way: `[instruction] [number]`.
And so the only thing left is to patch that number by calculating the offset and maybe loop it around.

But then I thought that... Writing the loop and patching the value in the instruction is not fun.
It is tedious and stupid for what you can do just by changing the edi itself with one instruction.

So that what I do should be funnier somehow.
And then an idea came to my mind that instead of jumping back and doing things in a style of the loop.
I can do what would have done the programmer in the functional world. 

I can create an iteration that copies itself and runs new iteration.
Kind of like the interpreter which is right inside the definition of the program.
It doesn't stop right there. It copies itself and then it runs itself over and over again.

And the final version of the program in the memory is actually the depiction of the execution.
That is kinda... weird to think about.

And it is very lispy in style.
That's a true pure functional programming! :D

### Okay

In order to do that insanity we need to solve a lot of problems if we are not assembly and bytecode programmers.
Because I am not.

1. How do we even create a function in x86-64?
2. How do we execute the body? How do we return anything and what we need to know?
3. How do we copy the whole body of a function further down the road? And so that we don't go mad in the process?
4. How do we check condition and leave instead of executing the last time?

You can also have these two questions:
- How do we even store something like that in C++?
- Okay but how do we execute it?

Well, if you are not familiar with that, then basically computer memory page can be marked as executable.
By default we do not have it like that, but if you ask the operating system it gives you the memory which is executable.
And thus we can then write instructions we wrote as pure bytes, take and address of that thing and cast it to function pointer.
Call it in C++ or C code.

The compiler will do the dirty work of calculating everything to leave us there alone in the memory page.
So that we can step debug this insane code.

In order to store the code I just used byte array. It was pretty convenient actually. Because you can just ask sizeof of it.
And here you go - here is your size of the block of code.

And so that we do not go mad calculating it all by ourselves, I just divided the program into logically connected blocks, simple units of work.
So that it would be pretty easy to reason about and understand.

> By the way, it was actually very funny to discover that many instructions are encoded really nicely and if you think about it for longer.
I guess I can even memorize what is going on there. As I have memorized 256 combinations of ASCII characters on windows alt + number input method.
When I was in highschool. But later in life, I completely lost that knowledge. Because it was utterly useless.

### 1. How do we even create a function in x86-64?
### 2. How do we execute the body? How do we return anything and what we need to know?
### 3. How do we copy the whole body of a function further down the road? And so that we don't go mad in the process?
### 4. How do we check condition and leave instead of executing the last time?
