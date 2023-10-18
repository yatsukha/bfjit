# BFJIT

C++ optimizing Brainf\*ck compiler, implemented using LLVM Intermediate Representation set of libraries.

Features:
 * read from file or STDIN
 * full LLVM optimization passes (O3)
 * debug information
 * optional output of both original IR and the optimized version
 * cell size agnostic implementation, the default is 32 bit
 * `,` operator with EOF support, allowing proper STDIN handling
 * bound checks for `>` and `<`

## Building

Dependencies:
 * `fmtlib` version 10
 * `LLVM` library, reasonably recent
 * compiler that supports C++23

```
$ mkdir release_build && cd release_build
$ LDFLAGS="<optional LDFLAGS for your LLVM distribution>" \
    cmake .. \
    -DCMAKE_BUILD_TYPE=Release \ # everything after this is optional
    -DCMAKE_C_COMPILER=<optional clang> \
    -DCMAKE_CXX_COMPILER=<optional clang++> \
    -DLLVM_DIR=<where to look for custom LLVM lib> \
    -DCUSTOM_CXX_LINKER=<optional link flag to pass to compiler for a custom linker>

# example for MacOS using homebrew and commercial version of the mold linker
$ LDFLAGS="-L$HOMEBREW_PREFIX/opt/llvm/lib/c++ -Wl,-rpath,$HOMEBREW_PREFIX/opt/llvm/lib/c++" \
    cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=/opt/homebrew/opt/llvm/bin/clang \
    -DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++ \
    -DLLVM_DIR=/opt/homebrew/opt/llvm/lib/cmake/llvm \
    -DCUSTOM_CXX_LINKER="--ld-path=/usr/local/bin/ld64.sold"

# compile, this will output a bfjit binary
$ make -j <number of logical cores>
```

## Usage

```
Brainf*ck JIT interpreter.
Usage: ./bfjit [OPTIONS] [file.bf]

Positionals:
  file.bf TEXT                Optional BF file to interpret, otherwise read from stdin until ^D (EOF).

Options:
  -h,--help                   Print this help message and exit
  -v,--verbose                Print intermediate results, as well as resulting optimized LLVM IR.
  -n,--no-exec                Dont execute, just print the optimized LLVM IR.
```

There are three programs - a hello world, cat, and fibonacci in the `example/` subdirectory.

## Examples of LLVM IR output


`example/cat.bf`:
```
,     # read char
[     # if not zero (EOF)
  .   # print it out
  ,   # read the next char
]     # jump to matching [ if not zero
```

<details>
    <summary>Unoptimized output:</summary>

    ; ModuleID = 'global_module'
    source_filename = "global_module"

    define i32 @jit_main(i32 %0, ptr %1) !dbg !2 {

    ;;;;;; ENTRY - memory allocation and cell pointer setup

    entry:
      %memory_base = alloca ptr, align 8
      %memory_ptr = alloca ptr, align 8
      %memory_limit = alloca ptr, align 8
      %2 = tail call ptr @malloc(i64 1073741824)
      store ptr %2, ptr %memory_base, align 8
      store ptr %2, ptr %memory_ptr, align 8
      call void @llvm.memset.p0.i64(ptr align 1 %2, i8 0, i64 1073741824, i1 false)
      %3 = load ptr, ptr %memory_base, align 8
      %4 = ptrtoint ptr %3 to i64
      %5 = add nuw nsw i64 %4, 8589934592
      %6 = inttoptr i64 %5 to ptr
      store ptr %6, ptr %memory_limit, align 8

    ;;;;;; get first char

      %7 = call i32 @getchar(), !dbg !9

    ;;;;;; check for EOF

      %8 = icmp eq i32 %7, -1, !dbg !9
      br i1 %8, label %if, label %else, !dbg !9


    if:                                               ; preds = %entry
      %9 = load ptr, ptr %memory_ptr, align 8, !dbg !9
      store i32 0, ptr %9, align 4, !dbg !9
      br label %merge, !dbg !9

    else:                                             ; preds = %entry
      %10 = load ptr, ptr %memory_ptr, align 8, !dbg !9
      store i32 %7, ptr %10, align 4, !dbg !9
      br label %merge, !dbg !9

    merge:                                            ; preds = %else, %if
      %11 = load ptr, ptr %memory_ptr, align 8, !dbg !10
      %12 = load i32, ptr %11, align 4, !dbg !10

    ;;;;;; check for loop entry

      %13 = icmp ne i32 %12, 0, !dbg !10
      br i1 %13, label %loop, label %exit, !dbg !10

    ;;;;;; loop body

    loop:                                             ; preds = %merge3, %merge
      %14 = load ptr, ptr %memory_ptr, align 8, !dbg !11
      %15 = load i32, ptr %14, align 4, !dbg !11
      %16 = call i32 @putchar(i32 %15), !dbg !11
      %17 = call i32 @getchar(), !dbg !12
      %18 = icmp eq i32 %17, -1, !dbg !12

    ;;;;;; check for EOF

      br i1 %18, label %if1, label %else2, !dbg !12

    ;;;;;; successful exit

    exit:                                             ; preds = %merge3, %merge
      ret i32 0, !dbg !12

    if1:                                              ; preds = %loop
      %19 = load ptr, ptr %memory_ptr, align 8, !dbg !12
      store i32 0, ptr %19, align 4, !dbg !12
      br label %merge3, !dbg !12

    else2:                                            ; preds = %loop
      %20 = load ptr, ptr %memory_ptr, align 8, !dbg !12
      store i32 %17, ptr %20, align 4, !dbg !12
      br label %merge3, !dbg !12

    merge3:                                           ; preds = %else2, %if1
      %21 = load ptr, ptr %memory_ptr, align 8, !dbg !12
      %22 = load i32, ptr %21, align 4, !dbg !12

    ;;;;;; check if the loop should continue

      %23 = icmp ne i32 %22, 0, !dbg !12
      br i1 %23, label %loop, label %exit, !dbg !12
    }

    ;;;;;; extern functions used

    declare noalias ptr @malloc(i64)

    ; Function Attrs: nocallback nofree nounwind willreturn memory(argmem: write)
    declare void @llvm.memset.p0.i64(ptr nocapture writeonly, i8, i64, i1 immarg) #0

    declare i32 @getchar()

    declare i32 @putchar(i32)

    attributes #0 = { nocallback nofree nounwind willreturn memory(argmem: write) }

    ;;;;;; debug information, in this case this refers to the oneline ",[.,]" version of the program


    !llvm.dbg.cu = !{!0}

    !0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "BFJIT", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug)
    !1 = !DIFile(filename: "cat.bf", directory: "../example")
    !2 = distinct !DISubprogram(name: "jit_main", linkageName: "jit_main", scope: null, file: !1, type: !3, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0)
    !3 = !DISubroutineType(types: !4)
    !4 = !{!5, !5, !6}
    !5 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
    !6 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !7, size: 64)
    !7 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !8, size: 64)
    !8 = !DIBasicType(name: "char", size: 8, encoding: DW_ATE_signed_char)
    !9 = !DILocation(line: 1, scope: !2)
    !10 = !DILocation(line: 1, column: 1, scope: !2)
    !11 = !DILocation(line: 1, column: 2, scope: !2)
    !12 = !DILocation(line: 1, column: 3, scope: !2)
</details>

<details>
    <summary>Optimized version yields a more sensible loop (noise removed):</summary>

    define i32 @jit_main(i32 %0, ptr nocapture readnone %1) local_unnamed_addr #0 {
    entry:
      %2 = tail call i32 @getchar()
      %3 = add i32 %2, 1
      %.not = icmp ult i32 %3, 2
      br i1 %.not, label %exit, label %loop

    loop:                                             ; preds = %entry, %loop
      %.1314 = phi i32 [ %5, %loop ], [ %2, %entry ]
      %4 = tail call i32 @putchar(i32 %.1314)
      %5 = tail call i32 @getchar()
      %6 = add i32 %5, 1
      %.not12 = icmp ult i32 %6, 2
      br i1 %.not12, label %exit, label %loop

    exit:                                             ; preds = %loop, %entry
      ret i32 0
    }
</details>

