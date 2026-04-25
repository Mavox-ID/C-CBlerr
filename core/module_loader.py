from pathlib import Path
from typing import Dict, Set, List
from core.lexer import tokenize
from core.flux_parser import parse
from core.flux_ast import Program, ImportStmt, FromImportStmt, FunctionDef, StructDef, GlobalVariable

class ImportError(Exception):
    pass

def _resolve_module_path(module_name: str, base_dir: Path) -> Path:
    p = Path(module_name)
    if not p.is_absolute():
        candidate = (base_dir / module_name)
    else:
        candidate = p
    if candidate.exists():
        return candidate.resolve()
    if not candidate.suffix:
        cand2 = candidate.with_suffix('.cbl')
        if cand2.exists():
            return cand2.resolve()
    raise ImportError(f"Импортируемый файл не найден: {module_name} (from {base_dir})")


def inline_imports(program: Program, source_path: str | Path, cache: Dict[Path, Program] | None = None,
                   included: Set[Path] | None = None, stack: List[Path] | None = None) -> Program:
    if cache is None:
        cache = {}
    if included is None:
        included = set()
    if stack is None:
        stack = []

    src_path = Path(source_path).resolve()
    base_dir = src_path.parent

    funcs_names = {f.name for f in program.functions}
    structs_names = {s.name for s in program.structs}
    globals_names = {g.name for g in program.global_vars}

    imports = list(program.imports) if getattr(program, 'imports', None) else []

    for imp in imports:
        if imp is None:
            continue
        if isinstance(imp, tuple):
            continue

        if isinstance(imp, ImportStmt):
            module_name = imp.module_name
            mod_path = _resolve_module_path(module_name, base_dir)
            if mod_path in stack:
                path_chain = ' -> '.join(str(p) for p in stack + [mod_path])
                raise ImportError(f"Обнаружен циклический импорт: {path_chain}")

            if mod_path not in cache:
                src = Path(mod_path).read_text(encoding='utf-8')
                tokens = tokenize(src, str(mod_path))
                imported_prog = parse(tokens)
                cache[mod_path] = imported_prog
                inline_imports(imported_prog, mod_path, cache, included, stack + [mod_path])
            else:
                imported_prog = cache[mod_path]

            if mod_path in included:
                continue

            for s in imported_prog.structs:
                if s.name in structs_names or s.name in funcs_names or s.name in globals_names:
                    raise ImportError(f"При импорте обнаруживается повторяющийся символ '{s.name}' из {mod_path}")
                program.structs.append(s)
                structs_names.add(s.name)

            for f in imported_prog.functions:
                if f.name in funcs_names or f.name in structs_names or f.name in globals_names:
                    raise ImportError(f"При импорте обнаруживается повторяющийся символ '{f.name}' из {mod_path}")
                program.functions.append(f)
                funcs_names.add(f.name)

            for g in imported_prog.global_vars:
                if g.name in globals_names or g.name in funcs_names or g.name in structs_names:
                    raise ImportError(f"При импорте обнаруживается повторяющийся символ '{g.name}' из {mod_path}")
                program.global_vars.append(g)
                globals_names.add(g.name)

            included.add(mod_path)

        elif isinstance(imp, FromImportStmt):
            module_name = imp.module_name
            mod_path = _resolve_module_path(module_name, base_dir)
            if mod_path in stack:
                path_chain = ' -> '.join(str(p) for p in stack + [mod_path])
                raise ImportError(f"Обнаружен циклический импорт: {path_chain}")

            if mod_path not in cache:
                src = Path(mod_path).read_text(encoding='utf-8')
                tokens = tokenize(src, str(mod_path))
                imported_prog = parse(tokens)
                cache[mod_path] = imported_prog
                inline_imports(imported_prog, mod_path, cache, included, stack + [mod_path])
            else:
                imported_prog = cache[mod_path]

            for item in imp.items or []:
                found = False
                for f in imported_prog.functions:
                    if f.name == item:
                        if item in funcs_names or item in structs_names or item in globals_names:
                            raise ImportError(f"При импорте обнаруживается повторяющийся символ '{item}' из {mod_path}")
                        program.functions.append(f)
                        funcs_names.add(item)
                        found = True
                        break
                if found:
                    continue
                for s in imported_prog.structs:
                    if s.name == item:
                        if item in structs_names or item in funcs_names or item in globals_names:
                            raise ImportError(f"При импорте обнаруживается повторяющийся символ '{item}' из {mod_path}")
                        program.structs.append(s)
                        structs_names.add(item)
                        found = True
                        break
                if found:
                    continue
                for g in imported_prog.global_vars:
                    if g.name == item:
                        if item in globals_names or item in funcs_names or item in structs_names:
                            raise ImportError(f"При импорте обнаруживается повторяющийся символ '{item}' из {mod_path}")
                        program.global_vars.append(g)
                        globals_names.add(item)
                        found = True
                        break
                if not found:
                    raise ImportError(f"Элемент '{item}' не найден в модуле {mod_path}")

        else:
            continue

    program.imports = []
    return program
