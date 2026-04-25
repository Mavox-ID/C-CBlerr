# 🛠️ CBlerr Language (v5.1)

**CBlerr** is a low-level system language for people who hate bloatware. Combines the strictness of Rust, the simplicity of Python, and full control over hardware by generating clean C code.

## 🚀 New in v5.1
- **Ultra-Small Binaries**: Reached an executable size of **2 KB** (thanks to custom linking and UPX compression).
- **OpenGL Core**: Full support for rendering 3D graphics.
- **Localization**: Compilation logs are now fully localized to Russian.
- **Optimization**: All unnecessary dependencies have been stripped off, hardcore mode enabled.

## 📦 Features
- **Target OS**: Windows (x64)
- **Compiler**: Python-based transpiler with GCC.
- **Standard Library**: Minimalistic wrapper around the Win32 API and MSVCRT library.

## 📂 Project Structure (Coming in v5.2)
In future releases: file `.cblproj` for automated compilation processes.

```ini
name = MyProject
main_file = main.cbl
ui = 1
icon = icon.ico
```

## 🛠️ Build Guide
1. Clone this repository.
2. Run the compiler.
   `python build/build.py your_code.cbl`
3. Get your maximally optimized executable.

---
**Developer**: Tankman02
**Motto**: "Work it harder, make it better, do it faster, makes us stronger."
```