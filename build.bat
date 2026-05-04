@echo off
:: Produces: src\bin\CBlerr.exe
::
:: Requirements:
::   MinGW-w64  (gcc.exe in PATH)
::   UPX        (upx.exe in PATH or same folder <-> optional) :>
::
:: Usage:
::   build.bat           normal release build
::   build.bat clean     remove build artefacts
:: OPEN IN TERMINAL PLEASE?

setlocal EnableDelayedExpansion

set CC=gcc
set SRC_DIR=src
set CORE_DIR=src\core
set BIN_DIR=src\bin
set TARGET=%BIN_DIR%\CBlerr.exe

set CFLAGS=-std=c11 -Os -s ^
 -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers ^
 -Wno-stringop-truncation -Wno-format-truncation -Wno-misleading-indentation ^
 -ffunction-sections -fdata-sections ^
 -fno-ident ^
 -fno-asynchronous-unwind-tables ^
 -fno-unwind-tables ^
 -fomit-frame-pointer ^
 -fno-stack-protector ^
 -fmerge-all-constants ^
 -fno-math-errno ^
 -fvisibility=hidden ^
 -mno-stack-arg-probe

set LDFLAGS=-nostartfiles ^
 -Wl,--entry=CblerrStartup ^
 -Wl,--gc-sections ^
 -Wl,--strip-all ^
 -Wl,--build-id=none ^
 -Wl,--no-seh ^
 -Wl,--file-alignment=512 ^
 -Wl,--section-alignment=4096 ^
 -lopengl32 -lwinmm -lmsvcrt -lkernel32 -luser32 -lgdi32

set SRCS=%SRC_DIR%\cblerr.c ^
 %CORE_DIR%\lexer.c ^
 %CORE_DIR%\parser.c ^
 %CORE_DIR%\codegen.c ^
 %CORE_DIR%\module_loader.c ^
 %CORE_DIR%\debugger.c ^
 %CORE_DIR%\type_checker.c ^
 %CORE_DIR%\monomorphizer.c

if "%1"=="clean" goto :clean

:release
if not exist "%BIN_DIR%" mkdir "%BIN_DIR%"
echo [CBlerr] Compiling for Windows...

%CC% %CFLAGS% -I%SRC_DIR% %SRCS% -o %TARGET% %LDFLAGS%
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Compilation failed!
    exit /b 1
)
echo [CBlerr] Built: %TARGET%

for %%X in (upx.exe upx) do (
    where %%X >nul 2>&1
    if !ERRORLEVEL! equ 0 (
        for %%F in (%TARGET%) do set SIZE_RAW=%%~zF
        %%X --best --lzma --quiet %TARGET% >nul 2>&1
        for %%F in (%TARGET%) do set SIZE_UPX=%%~zF
        echo [CBlerr] UPX: !SIZE_RAW! bytes to !SIZE_UPX! bytes
        goto :upx_done
    )
)
echo [CBlerr] UPX not found in PATH - Skipping
:upx_done

echo [CBlerr] Done: %TARGET%
exit /b 0

:clean
echo [CBlerr] Cleaning...
if exist "%TARGET%" del /f /q "%TARGET%"
if exist "%TEMP%\cblerr_standalone" rd /s /q "%TEMP%\cblerr_standalone"
echo [CBlerr] Clean done.
exit /b 0
