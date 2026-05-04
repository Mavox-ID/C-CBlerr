# CBlerr Compiler

CBlerr is a statically-compiled, C-targeting language with Python-inspired syntax.
This version is the fully rewritten **C** implementation (no Python runtime required).

## Project Layout

```
CBlerr/
├── src/
│   ├── cblerr.c               Compiler entry point
│   ├── bin/                   Build output (cblerr / cblerr.exe)
│   └── core/
│       ├── ast.h              AST node types & token definitions
│       ├── lexer.c/h          Tokenizer
│       ├── parser.c/h         Recursive-descent parser
│       ├── codegen.c/h        C code generator
│       ├── module_loader.c/h  Import resolver
│       └── debugger.c/h       Error reporting & suggestions
│       └── monomorphizer.c/h  Test Build
│       └── type_checker.c/h   Check Source code for errors with type ast.c/h
├── Makefile                   Linux build
└── build.bat                  Windows build (MinGW)
```

## Let's start

### Linux
```bash
# Install git
git clone https://github.com/Mavox-ID/C-CBlerr.git
```
Or just download zip in `https://github.com/Mavox-ID/C-CBlerr`.

### Windows
Download zip in `https://github.com/Mavox-ID/C-CBlerr`.

## Building the Compiler

### Linux
```bash
make          # produces src/bin/cblerr
```
Requires: `gcc` or `clang`.

### Windows (MinGW-w64)
```cmd
build.bat     :: produces src\bin\cblerr.exe
```
Requires MinGW-w64 with `gcc` in PATH.

## Using the Compiler

```bash
./CBlerr source.cbl               # compile → ./source (Linux) or source.exe (Win)
./CBlerr source.cbl -o myapp      # custom output name
./CBlerr source.cbl -c            # keep generated .c file
./CBlerr source.cbl --clang       # force Clang
./CBlerr source.cbl --stack-size 8M
```

## CBlerr Syntax Check

```cbl
# Variable declaration
x: int = 42
name: str = "hello"

# Function
def add(a: int, b: int) -> int:
    return a + b

# Extern C function
extern def printf(fmt: *void, ...) -> int

# Struct
struct Point:
    x: int
    y: int

# Control flow
if x > 0:
    print(x)
else:
    print(0)

while x > 0:
    x = x - 1

for i in 0..10:
    print(i)

# Types: int i8 i16 i32 i64 u8 u16 u32 u64 float f64 bool str void *T array<T>
# Operators: + - * / % ** and or not == != < > <= >= as sizeof
# Special: asm("...") endofcode
```
