# CBlerr Compiler

CBlerr is a statically-compiled, C-targeting language with Python-inspired syntax.
This version is the fully rewritten **C** implementation (no Python runtime required).

## Project Layout

```
CBlerr/
├── src/
│   ├── cblerr.c               Compiler entry point
│   ├── bin/                   Build output (CBlerr / CBlerr.exe)
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
make          # produces src/bin/CBlerr
```
Requires: `gcc` or `clang` && `UPX` (If you have and need a small size binary file CBlerr size)

### Windows (MinGW-w64)
```cmd
build.bat     :: produces src\bin\CBlerr.exe
```
Requires MinGW-w64 with `gcc` in PATH && `UPX` (If you have and need a small size binary file CBlerr size)

## Using the Compiler

```bash
./CBlerr source.cbl               # compile >GOT? ./source (Linux) or source.exe (Win)
./CBlerr source.cbl -o myapp      # custom output name
./CBlerr source.cbl -c            # keep generated .c file
./CBlerr source.cbl --clang       # force Clang
./CBlerr source.cbl --stack-size 8M
```

## Run a first app in CBlerr!

### CBlerr Hello World!

```cbl
# Hello World in CBlerr (Writen in C)

def main() -> int:
    printf("Hello World!");
    return 0
```
