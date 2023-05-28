# 操作系统实验报告

完成实验：

* lab1
* lab2
* lab3



## Lab 1

所有文件修改见：[commit: finish lab 1](https://github.com/GJCav/xv6-labs/commit/828d7625083699fa939d021ebd1b723b14d36264)



### Boot xv6

略过



### sleep

非常简单，代码实现如下：

```c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: sleep <ticks>\n");
        exit(0);
    }

    int ticks = atoi(argv[1]);
    sleep(ticks);
    exit(0);
}
```

唯一的坑在于需要手动在 `Makefile` 中添加这个应用程序，即在 Makefile 的 `UPROGS=\` 段中添加这个程序，即：

```Makefile
UPROGS=\
	...
	$U/_sleep
```

后续的其他小实验同理。





### pingpong

使用 fork 函数创建一个子进程，父子进程之间通过 pipe 通信。父进程向子进程发送一个字节的数据，子进程再把这个字节发回父进程。

代码比较简单，只要注意 pipe 是一个半双工的通道，父子相互通信需要创建两个 pipe。

代码见：[pingpong.c](https://github.com/GJCav/xv6-labs/blob/master/Lab1_Xv6%20and%20Unix%20utilities/xv6_for_Lab1/user/pingpong.c)



### primes

这个作业采用 CSP 模型实现一个简单的素数筛子。CSP模型解释见[Bell Labs and CSP Threads (swtch.com)](https://swtch.com/~rsc/thread/)。核心结构为：

![img](.typora/sieve.gif)

上图中每一个方框都代表一个进程，每个进程负责筛掉能被某个素数整除的数字，并且把不能筛掉的数字传递给右侧的进程。

进程的创建可使用 fork，进程之间的通信使用 pipe，但在解除掉整个算法程序时需要比较小心的处理进程结束顺序。最后 fork 出的进程需要最先结束，父进程不能在子进程结束前结束，所以需要用到 wait 函数等待子进程结束。

实现代码见：[primes.c](https://github.com/GJCav/xv6-labs/blob/master/Lab1_Xv6%20and%20Unix%20utilities/xv6_for_Lab1/user/primes.c)



### find

仿照并实现 linux 中的 `find` 指令。

实现代码见：[find.c at](https://github.com/GJCav/xv6-labs/blob/master/Lab1_Xv6%20and%20Unix%20utilities/xv6_for_Lab1/user/find.c)

实验要点：

* 对文件系统的处理需要使用 `fstat` 函数，通过返回的 `stat` 结构体判断该文件类型（普通文件/文件夹）
* 如果是文件夹，需要使用 `dirent` 结构体读取文件夹所有子文件名称。递归处理这些子文件
* 递归处理时，需要拼接路径，代码实现了 `join` 函数拼接路径，在静态区开辟一个缓冲区存放拼接结果。



### xargs

仿照并实现 linux 中的 xargs 指令。

作业实现中只要求传递 1 个参数，即 `xargs -n 1 xxxxx` 的效果。我在实现时稍稍扩展了一些，实现效果如下：

如果文件 `args` 内容为：

```txt
a bddd c
1 1
```

那么指令 `cat args | xargs echo hello` 的效果是执行下面两个指令：

* `echo hello a bddd c`
* `echo hello 1 1`

虽然功能没有 linux 中 `xargs` 那么丰富，但已经能够处理变长参数。

实现代码见：[xargs.c](https://github.com/GJCav/xv6-labs/blob/master/Lab1_Xv6%20and%20Unix%20utilities/xv6_for_Lab1/user/xargs.c)

实验要点：

1. 包装 `exec`，系统调用中给出的 exec 指令会使用当前进程启动命令，不太方便，所以对其进行包装：
   ```c
   void exec_block(const char* path, char** argv) {
       int pid = fork();
       if (pid == 0) {
           exec(path, argv); // echo 1 2 3
           exit(0);
       } else {
           wait(0);
       }
   }
   ```

   先 fork 出子进程调用 exec 然后让父进程等待子进程结束。但这里还有另一个坑，在win或者linux中调用 exec 函数会自动把 path 指定的程序路径添加到 argv 的开头，但 xv6 不会做这件事，和现在主流系统的处理方式稍有差别，需要留个心眼。

2. 变长`argv` 传递：声明 `main` 函数时，我们能拿到 `argc` 指明 `argv` 的数量，但调用 `exec` 时却没有指定 `argv` 的数量，而是采用类似 null-terminated string 的方法，假设要传递 n 个参数，那么应该使 `argv[n] = 0` 表示参数结束。



## Lab2: system call

所有文件修改见：[commit: finish lab2](https://github.com/GJCav/xv6-labs/commit/9a1034f815617842ea8beb25ab046a8a16172c6a)



### Using gdb

1. 问题：which function called syscall?
   * `usertrap()`
2. 问题：What is the value of p->trapframe->a7 and what does that value represent? 
   * `a7=0x7` 表示 SYS_exec
3. 问题：What was the previous mode that the CPU was in?
   * User Mode
4. 问题：Write down the assembly instruction the kernel is panicing at. Which register corresponds to the varialable `num`?
   * `lw a3,0(zero) # 0 <_entry-0x80000000>`
   * `a3`
5. 问题：Why does the kernel crash? Hint: look at figure 3-3 in the text; is address 0 mapped in the kernel address space? Is that confirmed by the value in `scause` above?
   * 地址 0 没有被映射到内核空间中，但强行在访问这个地址，所以 panic
   * `$scause = 13`，表明这是一个 load page fault
6. 问题：What is the name of the binary that was running when the kernel paniced? What is its process id (`pid`)?
   * name = initcode
   * pid = 1



### System call tracing

实验要求添加一个系统调用 `trace` ，按照 mask 跟踪所有对 mask 指定的系统函数的调用。且子进程应该继承父进程的 mask。

系统调用的流程比较复杂，梳理如下：

* 每个系统调用都有一个用户区接口（user-space stub）

* 这些接口定义在`user/user.h`中

* 但这些接口实现时需要使用特殊的汇编指令 `ecall` ，所以必须使用汇编来实现这些函数。所有汇编代码实现在 `usys.S` 文件中，因为写汇编太繁琐，所以使用一个 perl 脚本 `usys.pl` 来生成 `usys.S`，我们只需要修改 `usys.pl` 即可

* 在 `usys.S` 中调用系统调用的基本格式如下：
  ```assembly
  fork:
   li a7, SYS_fork
   ecall
   ret
  ```

  需要把 systemcall 的编码放入 a7 寄存器，然后使用 `ecall` 指令。这些系统调用的编码在 `kernel/syscall.h` 文件中定义

* `ecall` 执行后，会调用 `kernel/syscall.c` 中的 `syscall(void)` 函数。`syscall.c` 文件会维护一个从系统调用编码到系统过程（`sysproc.c`）的映射，然后通过寄存器 a7 的值找到这个系统过程函数，然后执行这个函数。所有的系统过程都实现在 `kernel/sysproc.c` 文件中。

理清了这个过程，我们需要修改下面这些文件：

* `user/user.h`：定义 `trace(void)` 的原型
* `user/usys.pl`：添加一个 `entry("trace")`
* `kernel/syscall.h`：定义 `SYS_trace` 的值
* `kernel/sysproc.c`：接收用户的 mask 并保存在进程结构体 `proc` 中
* `kernel/syscall.c`：检查进程的 mask，如果要调用的函数在 mask 中，那么输出跟踪消息

除了疏通系统调用的整个流程，还需要修改进程结构体来保存 mask 并且在 fork 时传递到子进程：

* `proc.h`：给`proc` 结构体末尾添加一个 `uint64 trace_mask`
* `proc.c`：在 `fork` 函数中把父进程的 mask 赋值给子进程

详细文件修改见：[commit: finish lab2](https://github.com/GJCav/xv6-labs/commit/9a1034f815617842ea8beb25ab046a8a16172c6a)



### sysinfo

添加一个系统调用函数，用来打印一些系统信息。

重点在于统计空闲内存和进程数量，系统调用流程同上个小实验。

在 `proc.c` 中添加一个函数 `proc_count` 统计进程数量，代码见：[proc_count](https://github.com/GJCav/xv6-labs/commit/9a1034f815617842ea8beb25ab046a8a16172c6a#diff-cd53148a0258a75f08eed9898d8bd26315b0f2781c576872e16aed0fad338d09R690)

* 利用 `proc` 数组统计状态不是 `UNUSED` 的进程

在 `kalloc.c` 中添加函数 `free_mem` 统计空闲内存大小，代码见：[free_mem](https://github.com/GJCav/xv6-labs/commit/9a1034f815617842ea8beb25ab046a8a16172c6a#diff-ca8af731e4ba3e3f8fd8bdb38a8bae71b2fa1bdc4434e985bf072cfabbfcaaccR85)

* 直接利用 `kmem.freelist` 结构统计空闲块大小，然后乘上 512 得到空闲内存大小

为了让 `sysproc.c` 中也能调用上面两个函数，修改 `defs.h` 文件，添加这两个函数的原型





## Lab3: pgtables

### Speed up system calls

在`proc.h` 中的 `proc` 结构体添加一项 `struct usyscall *usyscall;`  来保存地址信息。

修改 `proc.c` 文件

* `allocproc` 函数中添加初始化 `usyscall` 的相关代码
* `freeproc` 函数中释放 `usyscall`
* `proc_pagetable` 函数中添加内存映射
* `proc_freepagetable` 函数中取消这个内存映射
* 这里代码比较零碎，详细修改见 [proc.c](https://github.com/GJCav/xv6-labs/commit/00bea8e6ba145902939cc41f14d9c789356a2d42#diff-e46295d53ffe75186e152cb085d036dad17d8502368ab4f3a208e2a92db72922R135)



问题：Which other xv6 system call(s) could be made faster using this shared page? Explain how.

* 可以加速fork()，通过在struct usyscall中添加一个parent数据，以供child们需要的时候在用户态直接使用USYSCALL页面调用，而不用切换到内核态。同样的思路可以加速 fstat。



### Print a page table

主要代码添加在 `kernel/vm.c` 中，代码见：[vmprint](https://github.com/GJCav/xv6-labs/commit/00bea8e6ba145902939cc41f14d9c789356a2d42#diff-0b29bf87c0929b28b0397fd73cc0694a9172b295e447f3da4e2eac359f651dfcR292)

* 偷个小懒，使用一个静态变量 deep 来储存深度，可能在并发时出现问题。但能通过测试，所以没有进一步改进。



### Detect which pages have been accessed

代码量不大，只要理解页表结构和提示中各种函数的用法不难写出：

```c
int
sys_pgaccess(void)
{
  uint64 vaddr;
  int max_num;
  uint64 mask_addr;

  argaddr(0, &vaddr);
  argint(1, &max_num);
  argaddr(2, &mask_addr);

  uint64 mask = 0;
  struct proc *p = myproc();
  for(int i = 0;i < max_num;i++) {
    pte_t *pte = walk(p->pagetable, vaddr + i * PGSIZE, 0);
    if (pte == 0) panic("sys_pgaccess: page not exist");
    if (PTE_FLAGS(*pte) & PTE_A) {
      mask |= (1L << i);
    }
    *pte = ((*pte) & PTE_A) ^ (*pte); // set PTE_A to 0
  }
  if (copyout(p->pagetable, mask_addr, (char*)&mask, sizeof(mask)) < 0) {
    panic("sys_pgaccess: copyout error");
  }
  return 0;
}
```

