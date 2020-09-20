# Bomb Lab Walkthrough
by Kai Schuyler Gonzalez
## Setup

- untar bomb9.tar
    - ```tar xvf bomb9.tar```
- cd bomb9
- save strings in file to text file
    - `strings bomb > bomb9-str`
- save assembly code to text file
    - `objdump -d bomb > bomb9-assem`
- look at main contents
    - Found `<phase_1>` function call, along with `<phase_defused>`
    ```
        106d:	e8 9f 08 00 00       	callq  1911 <read_line>
        1072:	48 89 c7             	mov    %rax,%rdi
        1075:	e8 fa 00 00 00       	callq  1174 <phase_1>
        107a:	e8 d6 09 00 00       	callq  1a55 <phase_defused>
    ```
    - same function calls for phases 2, 3, 4, 5, 6
- Set breakpoints at each phase and ```<explode_bomb>``` to avoid any bomb explosions
___
## Phase 1
- Use `disas` command in gdb to look at `<phase_1>` assembly code: 
```
Dump of assembler code for function phase_1:
 => 0x0000555555555174 <+0>:     sub    $0x8,%rsp
    0x0000555555555178 <+4>:     lea    0x16d1(%rip),%rsi        # 0x555555556850
    0x000055555555517f <+11>:    callq  0x55555555568f <strings_not_equal>
    0x0000555555555184 <+16>:    test   %eax,%eax
    0x0000555555555186 <+18>:    jne    0x55555555518d <phase_1+25>
    0x0000555555555188 <+20>:    add    $0x8,%rsp
    0x000055555555518c <+24>:    retq   
    0x000055555555518d <+25>:    callq  0x555555555894 <explode_bomb>
    0x0000555555555192 <+30>:    jmp    0x555555555188 <phase_1+20>
End of assembler dump.
```
- `<strings_not_equal>` compares input string to a string stored in memory.
- run gdb with breakpoints set. 
    - Enter 'test string' for `<phase_1>` input
- Look at contents of `$rdi`
    - `(gdb) p $rdi`
    - $rdi contains input string
- use `si` command to step through assembly code until `<bomb_explode>` function call 
- print contents of `$rsi`
    - `$rsi` contains string 'Why make trillions when we could make... billions?`

### Solution: **Why make trillions when we could make... billions?**
___
## Phase 2
- Once again, `disas` phase_2
```
Dump of assembler code for function phase_2:
 => 0x0000555555555194 <+0>:     push   %rbp
    0x0000555555555195 <+1>:     push   %rbx
    0x0000555555555196 <+2>:     sub    $0x28,%rsp
    0x000055555555519a <+6>:     mov    %rsp,%rsi
    0x000055555555519d <+9>:     callq  0x5555555558d0 <read_six_numbers>
    0x00005555555551a2 <+14>:    cmpl   $0x0,(%rsp)
    0x00005555555551a6 <+18>:    jne    0x5555555551af <phase_2+27>
    0x00005555555551a8 <+20>:    cmpl   $0x1,0x4(%rsp)
    0x00005555555551ad <+25>:    je     0x5555555551b4 <phase_2+32>
    0x00005555555551af <+27>:    callq  0x555555555894 <explode_bomb>
    0x00005555555551b4 <+32>:    mov    %rsp,%rbx
    0x00005555555551b7 <+35>:    lea    0x10(%rbx),%rbp
    0x00005555555551bb <+39>:    jmp    0x5555555551c6 <phase_2+50>
    0x00005555555551bd <+41>:    add    $0x4,%rbx
    0x00005555555551c1 <+45>:    cmp    %rbp,%rbx
    0x00005555555551c4 <+48>:    je     0x5555555551d7 <phase_2+67>
    0x00005555555551c6 <+50>:    mov    0x4(%rbx),%eax
    0x00005555555551c9 <+53>:    add    (%rbx),%eax
    0x00005555555551cb <+55>:    cmp    %eax,0x8(%rbx)
    0x00005555555551ce <+58>:    je     0x5555555551bd <phase_2+41>
    0x00005555555551d0 <+60>:    callq  0x555555555894 <explode_bomb>
    0x00005555555551d5 <+65>:    jmp    0x5555555551bd <phase_2+41>
    0x00005555555551d7 <+67>:    add    $0x28,%rsp
    0x00005555555551db <+71>:    pop    %rbx
    0x00005555555551dc <+72>:    pop    %rbp
    0x00005555555551dd <+73>:    retq   
End of assembler dump.
```
- main thing going in this function is `<read_six_numbers>`
- this function takes input as 6 integers formatted as follows:
    - `a b c d e f`
- if first input is not 0, `<explode_bomb>` is called
    ```
    <+14>:    cmpl   $0x0,(%rsp)
    <+18>:    jne    0x5555555551af <phase_2+27>
    ```
- If second input is not 1, assembly continues on to `<explode_bomb>`
    ```
    <+20>:    cmpl   $0x1,0x4(%rsp)
    <+25>:    je     0x5555555551b4 <phase_2+32>
    <+27>:    callq  0x555555555894 <explode_bomb>
    ```
- The loop between lines `<+41>` and `<+58>` checks previous two inputs and sums them to compare to current input and breaks when `$rbp` and `$rbx` register contents are equal
### Solution: **0 1 1 2 3 5**
___
## Phase 3
- The assembly code of `<phase_3>` is kind of long, let's look at lines the important lines:
```
    <+19>:    lea    0x16b6(%rip),%rsi        # 0x5555555568ae
    <+36>:    cmp    $0x2,%eax
    <+39>:    jle    0x555555555226 <phase_3+72>
    ...
    <+41>:    cmpl   $0x7,0xc(%rsp)
    <+46>:    ja     0x555555555318 <phase_3+314>
    ...
    <+56>:    lea    0x16a3(%rip),%rdx        # 0x5555555568c0
    ...
    <+72>:    callq  0x555555555894 <explode_bomb>
    ...
    <+260>:   mov    $0x63,%eax
    <+265>:   cmpl   $0x37a,0x8(%rsp)
    <+273>:   je     0x555555555322 <phase_3+324>
    ...
    <+324>:   cmp    %al,0x7(%rsp)
    <+328>:   je     0x55555555532d <phase_3+335>
    <+330>:   callq  0x555555555894 <explode_bomb>
```
- `x /s 0x5555555568ae` outputs `0x5555555568ae: "%d %c %d"`
    - The input will have the format `int char int`
    - run gdb again with test string that follows this format
- first int must be greater than 2 to jump over `<explode_bomb>`
- keep stepping through assembly until line 260
    - third input is being compared to hex number `0x37a` which equals `890` in decimal
        - this must be the second integer.
- at line 325, the second input is being compared to register `$al`
    - `x/a $al` = `0x63`
    - `0x63` = decimal `99`
    - char 99 is `c`
- the solution must be `6 c 890`, test to make sure.
- Success!
    
### Solution: **6 c 890**
___
## Phase 4
- `x /s 0x555555556ae3` shows us that the string that the input is being compared to has format `int int`
- registers `$rsp` points to array that stores input integers, `rsi` stores format
- second input must be a 2
- A function call is made to `func4`. Let's look at it's assembly code:
    ```
    Dump of assembler code for function func4:
    =>  0x0000555555555332 <+0>:     mov    $0x0,%eax
        0x0000555555555337 <+5>:     test   %edi,%edi
        0x0000555555555339 <+7>:     jle    0x555555555342 <func4+16>
        0x000055555555533b <+9>:     mov    %esi,%eax
        0x000055555555533d <+11>:    cmp    $0x1,%edi
        0x0000555555555340 <+14>:    jne    0x555555555344 <func4+18>
        0x0000555555555342 <+16>:    repz retq 
        0x0000555555555344 <+18>:    push   %r12
        0x0000555555555346 <+20>:    push   %rbp
        0x0000555555555347 <+21>:    push   %rbx
        0x0000555555555348 <+22>:    mov    %esi,%r12d
        0x000055555555534b <+25>:    mov    %edi,%ebx
        0x000055555555534d <+27>:    lea    -0x1(%rdi),%edi
        0x0000555555555350 <+30>:    callq  0x555555555332 <func4>
        0x0000555555555355 <+35>:    lea    (%rax,%r12,1),%ebp
        0x0000555555555359 <+39>:    lea    -0x2(%rbx),%edi
        0x000055555555535c <+42>:    mov    %r12d,%esi
        0x000055555555535f <+45>:    callq  0x555555555332 <func4>
        0x0000555555555364 <+50>:    add    %ebp,%eax
        0x0000555555555366 <+52>:    pop    %rbx
        0x0000555555555367 <+53>:    pop    %rbp
        0x0000555555555368 <+54>:    pop    %r12
        0x000055555555536a <+56>:    retq   
    End of assembler dump.
    ```
    - this is a recursive function.
- Phase_4 calls this recursive function and compares first input integer to the result after the recursive function executes.
- After the recursive function, our inputs have to be equal to `108` and `2`
### Solution:  **108 2**
___
## Phase 5
```
Dump of assembler code for function phase_5:
 => 0x00005555555553be <+0>:     sub    $0x18,%rsp
    0x00005555555553c2 <+4>:     lea    0x8(%rsp),%rcx
    0x00005555555553c7 <+9>:     lea    0xc(%rsp),%rdx
    0x00005555555553cc <+14>:    lea    0x1710(%rip),%rsi        # 0x555555556ae3
    0x00005555555553d3 <+21>:    mov    $0x0,%eax
    0x00005555555553d8 <+26>:    callq  0x555555554e60 <__isoc99_sscanf@plt>
    0x00005555555553dd <+31>:    cmp    $0x1,%eax
    0x00005555555553e0 <+34>:    jle    0x55555555542f <phase_5+113>
    0x00005555555553e2 <+36>:    mov    0xc(%rsp),%eax
    0x00005555555553e6 <+40>:    and    $0xf,%eax
    0x00005555555553e9 <+43>:    mov    %eax,0xc(%rsp)
    0x00005555555553ed <+47>:    cmp    $0xf,%eax
    0x00005555555553f0 <+50>:    je     0x555555555425 <phase_5+103>
    0x00005555555553f2 <+52>:    mov    $0x0,%ecx
    0x00005555555553f7 <+57>:    mov    $0x0,%edx
    0x00005555555553fc <+62>:    lea    0x14dd(%rip),%rsi        # 0x5555555568e0 <array.3418>
    0x0000555555555403 <+69>:    add    $0x1,%edx
    0x0000555555555406 <+72>:    cltq   
    0x0000555555555408 <+74>:    mov    (%rsi,%rax,4),%eax
    0x000055555555540b <+77>:    add    %eax,%ecx
    0x000055555555540d <+79>:    cmp    $0xf,%eax
    0x0000555555555410 <+82>:    jne    0x555555555403 <phase_5+69>
    0x0000555555555412 <+84>:    movl   $0xf,0xc(%rsp)
    0x000055555555541a <+92>:    cmp    $0xf,%edx
    0x000055555555541d <+95>:    jne    0x555555555425 <phase_5+103>
    0x000055555555541f <+97>:    cmp    %ecx,0x8(%rsp)
    0x0000555555555423 <+101>:   je     0x55555555542a <phase_5+108>
    0x0000555555555425 <+103>:   callq  0x555555555894 <explode_bomb>
    0x000055555555542a <+108>:   add    $0x18,%rsp
    0x000055555555542e <+112>:   retq   
    0x000055555555542f <+113>:   callq  0x555555555894 <explode_bomb>
    0x0000555555555434 <+118>:   jmp    0x5555555553e2 <phase_5+36>
End of assembler dump.
```
- again, the assembly code is too long to pour over, time to look at the important bits.
- I solved this problem by translating assembly to `C` as well as I could.
- The `C` code looks roughly like this:
```c
void phase_5(string s) {
    int a, b;
    if (sscanf(s, "%d %d", &a, &b) <= 1)
        explode_bomb();
    a &= 15;
    if (a != 15) {
        int c = 0
        int d = 0;
        do {
            d++;
            a = array[a];
            c += a;
        } while (a != 15);
        if (d == 15 && c == b) 
            return;
    }
    explode_bomb();
}
```
- this time, it looks like the input will be a string containing two integers seperated by a space.
- `<phase_5>` references an array in line <+62>: 
    - `array[15] = [10,2,14,7,8,12,15,11,0,4,1,13,3,9,6]`
- `int a` stores the first integer value after sscanf is called, `int b` stores the second.
- The registers in assembly code that store the values are `$rsi` and `$rdx`
- sscanf returns number of fields succesfully converted and assigned. If input is 1 or less integers, the bomb will explode.
- the do-while loop must execute 15 times in order the the final if statement to be true.
- c must also equal second input integer for function to exit without a bomb explosion.
- I reverse engineered the solution from here.
### Solution: **5 115** 
___
## Phase 6

- Phase 6 takes in six integers seperated by spaces
- each integer has to be less than or equal to 6
- no integer should be the same as any other integer
- This phase has a node data structure. 
- I found the address to every node and the values stored in each.
- From the assembly code, I realized I had to sort the nodes from largest to smallest values. 
- Once I had them sorted, I had to enter the inverse of each node in the same order
    - For example, if the node was 4, i would enter 2, if it was 1, I would enter 5, and so on.

Node values: 
```
         Node Address    Node            Hex Value      Decimal
<node6>: 0x000001a7      0x00000006      0x55758660     423 
<node2>: 0x00000126      0x00000002      0x55758120     294
<node1>: 0x0000015c      0x00000001      0x55758670     348
<node5>: 0x0000011d      0x00000005      0x00000000     285
<node4>: 0x0000008d      0x00000004      0x55758630     141
<node3>: 0x00000236      0x00000003      0x55758640     566
```

### Solution: **4 1 6 5 2 3**