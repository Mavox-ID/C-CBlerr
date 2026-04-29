import sys
import os
import inspect
import traceback
import time
from enum import IntEnum
from pathlib import Path
from datetime import datetime
from typing import Optional, Dict, Any, List, Tuple
from dataclasses import dataclass
from io import StringIO

ANSI_RESET = "\033[0m"
ANSI_RED = "\033[31m"
ANSI_BLUE = "\033[34m"
ANSI_YELLOW = "\033[33m"

def _detect_color_support() -> bool:
    if os.environ.get('NO_COLOR'):
        return False
    try:
        if not (hasattr(sys.stdout, 'isatty') and sys.stdout.isatty()) and not (hasattr(sys.stderr, 'isatty') and sys.stderr.isatty()):
            return False
    except Exception:
        pass
    if os.name == 'nt':
        try:
            import ctypes
            kernel32 = ctypes.windll.kernel32
            STD_OUTPUT_HANDLE = -11
            STD_ERROR_HANDLE = -12
            handle_out = kernel32.GetStdHandle(STD_OUTPUT_HANDLE)
            handle_err = kernel32.GetStdHandle(STD_ERROR_HANDLE)
            mode = ctypes.c_uint32()
            if kernel32.GetConsoleMode(handle_out, ctypes.byref(mode)) == 0:
                if kernel32.GetConsoleMode(handle_err, ctypes.byref(mode)) == 0:
                    return False
            ENABLE_VT_PROCESSING = 0x0004
            try:
                kernel32.SetConsoleMode(handle_out, mode.value | ENABLE_VT_PROCESSING)
            except Exception:
                try:
                    kernel32.SetConsoleMode(handle_err, mode.value | ENABLE_VT_PROCESSING)
                except Exception:
                    pass
            return True
        except Exception:
            return False
    return True

COLORS_SUPPORTED = _detect_color_support()

def color_red(text: str) -> str:
    if not COLORS_SUPPORTED:
        return text
    return f"{ANSI_RED}{text}{ANSI_RESET}"

def color_blue(text: str) -> str:
    if not COLORS_SUPPORTED:
        return text
    return f"{ANSI_BLUE}{text}{ANSI_RESET}"

def color_yellow(text: str) -> str:
    if not COLORS_SUPPORTED:
        return text
    return f"{ANSI_YELLOW}{text}{ANSI_RESET}"

def _levenshtein_distance(s1: str, s2: str) -> int:
    if len(s1) < len(s2):
        return _levenshtein_distance(s2, s1)
    if len(s2) == 0:
        return len(s1)
    
    previous_row = range(len(s2) + 1)
    for i, c1 in enumerate(s1):
        current_row = [i + 1]
        for j, c2 in enumerate(s2):
            insertions = previous_row[j + 1] + 1
            deletions = current_row[j] + 1
            substitutions = previous_row[j] + (c1 != c2)
            current_row.append(min(insertions, deletions, substitutions))
        previous_row = current_row
    
    return previous_row[-1]

def _token_similarity_score(unknown: str, candidate: str) -> float:
    unknown_lower = unknown.lower()
    candidate_lower = candidate.lower()
    
    if unknown_lower == candidate_lower:
        return 0.0
    
    distance = _levenshtein_distance(unknown_lower, candidate_lower)
    max_len = max(len(unknown_lower), len(candidate_lower))
    
    if max_len == 0:
        return float('inf')
    
    base_similarity = distance / max_len
    
    length_diff = abs(len(unknown_lower) - len(candidate_lower))
    length_penalty = length_diff * 0.1
    
    if unknown_lower[0] == candidate_lower[0]:
        base_similarity *= 0.85
    
    common_prefix = 0
    for i in range(min(len(unknown_lower), len(candidate_lower))):
        if unknown_lower[i] == candidate_lower[i]:
            common_prefix += 1
        else:
            break
    
    if common_prefix > 0:
        base_similarity *= (1.0 - common_prefix / max_len * 0.3)
    
    final_score = base_similarity + length_penalty
    return final_score

def _find_closest_match(identifier: str, candidates: List[str], max_distance: int = 3) -> Optional[str]:
    closest = None
    best_distance = max_distance + 1
    best_score = float('inf')
    
    id_lower = identifier.lower()
    id_first_char = id_lower[0] if id_lower else ''
    
    candidates_by_priority = []
    
    for candidate in candidates:
        if len(candidate) > 0 and len(identifier) > 0:
            cand_lower = candidate.lower()
            dist = _levenshtein_distance(id_lower, cand_lower)
            
            if dist <= max_distance:
                cand_first_char = cand_lower[0]
                starts_same = (cand_first_char == id_first_char)
                length_diff = abs(len(id_lower) - len(candidate))
                
                priority = (dist, not starts_same, length_diff, len(candidate))
                candidates_by_priority.append((priority, candidate))
    
    if candidates_by_priority:
        candidates_by_priority.sort()
        best = candidates_by_priority[0]
        return best[1]
    
    return None

KNOWN_KEYWORDS = [
    'def', 'return', 'endofcode', 'if', 'else', 'elif', 'while', 'for', 'break', 'continue',
    'struct', 'enum', 'match', 'case', 'default', 'import', 'from', 'module', 'as',
    'int', 'str', 'bool', 'float', 'void', 'u8', 'u16', 'u32', 'u64', 'i8', 'i16', 'i32', 'i64', 'f64',
    'and', 'or', 'not', 'in', 'asm', 'comptime', 'let', 'const', 'extern', 'packed', 'inline',
    'true', 'false', 'null', 'sizeof', 'print', 'printf', 'sprintf', 'scanf', 'malloc', 'calloc', 'realloc',
    'free', 'memcpy', 'memmove', 'memset', 'strlen', 'strcpy', 'strcat', 'strcmp', 'puts', 'putchar', 'getchar',
    'exit', 'abort', 'rand', 'srand', 'sin', 'cos', 'tan', 'sqrt', 'pow', 'abs', 'floor', 'ceil',
    'fabs', 'fmod', 'log', 'exp', 'atan2', 'memchr', 'memmem', 'strncpy', 'strncat', 'strncmp',
    'strstr', 'strchr', 'strrchr', 'strtok', 'atoi', 'atof', 'strtol', 'strtod', 'snprintf',
    'vprintf', 'fprintf', 'fscanf', 'fopen', 'fclose', 'fread', 'fwrite', 'fseek', 'ftell',
    'rewind', 'fflush', 'remove', 'rename', 'tmpfile', 'time', 'clock', 'difftime', 'mktime',
    'getenv', 'system', 'qsort', 'bsearch', 'atexit', 'signal', 'raise',
    'PostQuitMessage', 'CreateWindowExA', 'GetDC', 'ReleaseDC', 'ShowWindow', 'GetConsoleWindow',
    'SetConsoleMode', 'GetConsoleMode', 'GetStdHandle', 'WriteConsole', 'ReadConsole',
    'GetAsyncKeyState', 'Sleep', 'GetCursorPos', 'ScreenToClient', 'GetMessageA', 'DispatchMessageA',
    'PeekMessageA', 'TranslateMessage', 'MessageBoxA', 'wWinMain', 'WinMain', 'DllMain',
    'ExitProcess', 'GetModuleHandleA', 'GetProcAddress', 'LoadLibraryA', 'FreeLibrary',
    'Beep', 'PlaySoundA', 'sndPlaySoundA', 'timeGetTime', 'GetTickCount',
    'OpenFileA', 'CreateFileA', 'ReadFile', 'WriteFile', 'CloseHandle',
    'HeapAlloc', 'HeapFree', 'GetProcessHeap'
]

class DebugLevel(IntEnum):

    NONE = 0
    ERROR = 1
    WARNING = 2
    INFO = 3
    VERBOSE = 4
    TRACE = 5

@dataclass
class StackFrame:

    filename: str
    function: str
    lineno: int
    code: str
    locals_dict: Dict[str, Any]

@dataclass
class CrashContext:

    timestamp: datetime
    exception_type: str
    exception_message: str
    stack_frames: List[StackFrame]
    memory_info: Dict[str, Any]
    elapsed_time: float

class GameDebugger:

    _COLORS = {
        'RESET': '\033[0m',
        'RED': '\033[31m',
        'YELLOW': '\033[33m',
        'GREEN': '\033[32m',
        'BLUE': '\033[34m',
        'CYAN': '\033[36m',
        'GRAY': '\033[90m',
    }

    def __init__(
        self,
        debug_level: DebugLevel = DebugLevel.INFO,
        log_file: str = "debug.log",
        use_colors: bool = True,
        max_log_size: int = 10 * 1024 * 1024
    ) -> None:

        self.debug_level = debug_level
        self.log_file = Path(log_file)
        self.use_colors = use_colors and sys.stdout.isatty()
        self.max_log_size = max_log_size

        self.start_time = time.time()
        self.error_count = 0
        self.warning_count = 0
        self.memory_watches: Dict[int, Any] = {}

        self.log_file.parent.mkdir(parents=True, exist_ok=True)

    def _colorize(self, text: str, color: str) -> str:

        if not self.use_colors or color not in self._COLORS:
            return text
        return f"{self._COLORS[color]}{text}{self._COLORS['RESET']}"

    def _rotate_log_if_needed(self) -> None:

        if self.log_file.exists() and self.log_file.stat().st_size > self.max_log_size:
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            backup_name = self.log_file.stem + f"_{timestamp}.log"
            backup_path = self.log_file.parent / backup_name
            self.log_file.rename(backup_path)

    def _write_log(self, message: str) -> None:

        self._rotate_log_if_needed()
        try:
            with open(self.log_file, 'a', encoding='utf-8') as f:
                f.write(message + '\n')
        except IOError as e:
            print(f"Warning: Failed to write data to the log file: {e}")

    def _format_message(
        self,
        level: DebugLevel,
        message: str,
        include_time: bool = True
    ) -> Tuple[str, str]:

        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        elapsed = time.time() - self.start_time

        level_names = {
            DebugLevel.ERROR: 'Error',
            DebugLevel.WARNING: 'Warn',
            DebugLevel.INFO: 'INFO',
            DebugLevel.VERBOSE: 'VERB',
            DebugLevel.TRACE: 'TRACE',
        }

        level_name = level_names.get(level, 'UNKN')

        if include_time:
            prefix = f"[{timestamp}] [{level_name}] [{elapsed:7.3f}s]"
        else:
            prefix = f"[{level_name}]"

        file_msg = f"{prefix} {message}"

        color_map = {
            DebugLevel.ERROR: 'RED',
            DebugLevel.WARNING: 'YELLOW',
            DebugLevel.INFO: 'GREEN',
            DebugLevel.VERBOSE: 'CYAN',
            DebugLevel.TRACE: 'GRAY',
        }

        color = color_map.get(level, 'RESET')
        console_msg = self._colorize(prefix, color) + f" {message}"

        return console_msg, file_msg

    def log_error(self, message: str) -> None:

        if self.debug_level >= DebugLevel.ERROR:
            console_msg, file_msg = self._format_message(DebugLevel.ERROR, message)
            print(console_msg, file=sys.stderr)
            self._write_log(file_msg)
            self.error_count += 1

    def log_warning(self, message: str) -> None:

        if self.debug_level >= DebugLevel.WARNING:
            console_msg, file_msg = self._format_message(DebugLevel.WARNING, message)
            print(console_msg, file=sys.stderr)
            self._write_log(file_msg)
            self.warning_count += 1

    def log_info(self, message: str) -> None:

        if self.debug_level >= DebugLevel.INFO:
            console_msg, file_msg = self._format_message(DebugLevel.INFO, message)
            print(console_msg)
            self._write_log(file_msg)

    def log_verbose(self, message: str) -> None:

        if self.debug_level >= DebugLevel.VERBOSE:
            console_msg, file_msg = self._format_message(DebugLevel.VERBOSE, message)
            print(console_msg)
            self._write_log(file_msg)

    def log_trace(self, message: str) -> None:

        if self.debug_level >= DebugLevel.TRACE:
            console_msg, file_msg = self._format_message(DebugLevel.TRACE, message)
            print(console_msg)
            self._write_log(file_msg)

    def capture_crash_context(self, exc: Exception) -> CrashContext:

        tb = exc.__traceback__
        stack_frames: List[StackFrame] = []

        while tb is not None:
            frame_info = inspect.getframeinfo(tb.tb_frame)
            local_vars: Dict[str, Any] = {}

            try:
                for var_name, var_value in tb.tb_frame.f_locals.items():
                    try:
                        var_repr = repr(var_value)
                        if len(var_repr) > 500:
                            var_repr = var_repr[:500] + "..."
                        local_vars[var_name] = var_repr
                    except Exception:
                        local_vars[var_name] = "<unable to represent>"
            except Exception:
                pass

            stack_frames.append(StackFrame(
                filename=frame_info.filename,
                function=frame_info.function,
                lineno=frame_info.lineno,
                code=frame_info.code_context[0].strip() if frame_info.code_context else "",
                locals_dict=local_vars
            ))

            tb = tb.tb_next

        try:
            import psutil
            process = psutil.Process(os.getpid())
            memory_info = {
                'rss_mb': process.memory_info().rss / (1024 * 1024),
                'vms_mb': process.memory_info().vms / (1024 * 1024),
            }
        except ImportError:
            memory_info = {'note': 'psutil not found'}

        return CrashContext(
            timestamp=datetime.now(),
            exception_type=type(exc).__name__,
            exception_message=str(exc),
            stack_frames=stack_frames,
            memory_info=memory_info,
            elapsed_time=time.time() - self.start_time
        )

    def critical_dump(self, exc: Exception) -> None:

        context = self.capture_crash_context(exc)

        self.log_error("Critical Error! ")

        self.log_error(f"Exception Type: {context.exception_type}")
        self.log_error(f"Exception Message: {context.exception_message}")
        self.log_error(f"Timestamp: {context.timestamp.isoformat()}")
        self.log_error(f"Elapsed Time: {context.elapsed_time: .3f}s")

        self.log_error("\nStack call:")
        for i, frame in enumerate(context.stack_frames, 1):
            self.log_error(f"\n  Frame {i}: {frame.function} ({frame.filename}:{frame.lineno})")
            if frame.code:
                self.log_error(f"    Code: {frame.code}")
            if frame.locals_dict:
                self.log_error("    Local variable:")
                for var_name, var_value in frame.locals_dict.items():
                    self.log_error(f"      {var_name} = {var_value}")

        if context.memory_info:
            self.log_error(f"\nMemory information:")
            for key, value in context.memory_info.items():
                self.log_error(f"  {key}: {value}")

        self.log_error("\n" + "=" * 80)
        self.log_error(f"Total errors: {self.error_count}")
        self.log_error(f"Total warnings: {self.warning_count}")
        self.log_error(f"Log file: {self.log_file.absolute()}")

    def watch_memory(self, address: int) -> None:

        self.memory_watches[address] = time.time()
        self.log_verbose(f"Memory watch added: 0x{address:x}")

    def get_summary(self) -> str:

        elapsed = time.time() - self.start_time
        summary_lines = [
            "",
            "|" + "Debug Report".center(78) + "|",
            f"  Elapsed time: {elapsed:.3f}s",
            f"  Errors:       {self.error_count}",
            f"  Warnings:     {self.warning_count}",
            f"  Log file:     {self.log_file.absolute()}",
        ]
        return "\n".join(summary_lines)

    def __enter__(self):

        self.log_info("Debug session started")
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):

        if exc_type is not None:
            self.critical_dump(exc_val)
        else:
            self.log_info("Debug session completed successfully")
        return False

    def display_syntax_error(self, exc: Exception, source: Optional[str] = None, filename: Optional[str] = None) -> None:

        msg = str(exc) if exc is not None else "Error"

        if self.use_colors and COLORS_SUPPORTED:
            print(color_red("[ERROR]") + f" {msg}", file=sys.stderr)
        else:
            print(f"[ERROR] {msg}", file=sys.stderr)

        lineno = None
        start_col = None
        end_col = None

        if isinstance(exc, SyntaxError):
            lineno = getattr(exc, 'lineno', None)
            offset = getattr(exc, 'offset', None)
            text_attr = getattr(exc, 'text', None)
            if offset:
                try:
                    start_col = int(offset) - 1
                    end_col = start_col + 1
                except Exception:
                    start_col = None
            if text_attr and not source and isinstance(text_attr, str):
                source = text_attr

        if lineno is None:
            try:
                import re
                m = re.search(r'at line (\d+)', msg)
                if not m:
                    m = re.search(r'line (\d+)', msg)
                if m:
                    lineno = int(m.group(1))
            except Exception:
                lineno = None

        if source and lineno:
            lines = source.splitlines()
            if 1 <= lineno <= len(lines):
                src_line = lines[lineno - 1].rstrip('\n')
                print(src_line, file=sys.stderr)

                if start_col is None:
                    try:
                        import re
                        m = re.search(r"'([^']+)'|\"([^\"]+)\"", msg)
                        token = None
                        if m:
                            token = m.group(1) or m.group(2)
                        if token:
                            idx = src_line.find(token)
                            if idx != -1:
                                start_col = idx
                                end_col = idx + len(token)
                        if start_col is None:
                            idx = 0
                            while idx < len(src_line) and src_line[idx].isspace():
                                idx += 1
                            start_col = idx
                            end_col = idx + 1
                    except Exception:
                        start_col = 0
                        end_col = 1

                if start_col is None:
                    start_col = 0
                if end_col is None or end_col <= start_col:
                    end_col = start_col + 1

                if start_col < 0:
                    start_col = 0
                if start_col > len(src_line):
                    start_col = len(src_line)
                if end_col > len(src_line):
                    end_col = len(src_line)

                spaces = ' ' * start_col
                carets = '^' * max(1, end_col - start_col)
                if self.use_colors and COLORS_SUPPORTED:
                    print(spaces + color_blue(carets), file=sys.stderr)
                else:
                    print(f"{spaces}{carets}", file=sys.stderr)

        try:
            import re
            reason = None 
            fix = None
            hint = None
            
            if re.search(r'Unknown|Unexpected token', msg, flags=re.I):
                reason = 'Invalid or unexpected code fragment'
            
            m_unknown = re.search(r"'([^']+)'", msg)
            if m_unknown:
                unknown_token = m_unknown.group(1)
                closest = _find_closest_match(unknown_token, KNOWN_KEYWORDS, max_distance=3)
                if closest:
                    hint = f'Did you mean: "{closest}"'
            
            if 'endofcode' in (source or '') and 'return 0' in (source or ''):
                fix = 'Use a valid keyword'
            
            if reason:
                if self.use_colors and COLORS_SUPPORTED:
                    print(color_yellow("[REASON]") + f" {reason}", file=sys.stderr)
                else:
                    print(f"[REASON] {reason}", file=sys.stderr)
        except Exception:
            pass

_global_debugger: Optional[GameDebugger] = None

def get_debugger() -> GameDebugger:

    global _global_debugger
    if _global_debugger is None:
        _global_debugger = GameDebugger(debug_level=DebugLevel.INFO)
    return _global_debugger

def init_debugger(
    debug_level: DebugLevel = DebugLevel.INFO,
    log_file: str = "debug.log"
) -> GameDebugger:

    global _global_debugger
    _global_debugger = GameDebugger(debug_level=debug_level, log_file=log_file)
    return _global_debugger
