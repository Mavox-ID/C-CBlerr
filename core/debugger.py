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

class GameDebugger: #типо игра хахахахахахах

    _COLORS = {
        'RESET': '\033[0m',
        'RED': '\033[91m',
        'YELLOW': '\033[93m',
        'GREEN': '\033[92m',
        'BLUE': '\033[94m',
        'CYAN': '\033[96m',
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
            print(f"Предупреждение: Не удалось записать данные в файл журнала: {e}")

    def _format_message(
        self,
        level: DebugLevel,
        message: str,
        include_time: bool = True
    ) -> Tuple[str, str]:

        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        elapsed = time.time() - self.start_time

        level_names = {
            DebugLevel.ERROR: 'ERROR',
            DebugLevel.WARNING: 'WARN',
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
            memory_info = {'note': 'psutil не установлен, информация о памяти недоступна, установи psutil'}

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

        self.log_error("Критическая ошибка произошла! ")

        self.log_error(f"Тип исключения: {context.exception_type}")
        self.log_error(f"Сообщение исключения: {context.exception_message}")
        self.log_error(f"Временная метка: {context.timestamp.isoformat()}")
        self.log_error(f"Затраченное время: {context.elapsed_time:.3f}с")

        self.log_error("\nСтек вызовов:")
        for i, frame in enumerate(context.stack_frames, 1):
            self.log_error(f"\n  Кадр {i}: {frame.function} ({frame.filename}:{frame.lineno})")
            if frame.code:
                self.log_error(f"    Код: {frame.code}")
            if frame.locals_dict:
                self.log_error("    Локальные переменные:")
                for var_name, var_value in frame.locals_dict.items():
                    self.log_error(f"      {var_name} = {var_value}")

        if context.memory_info:
            self.log_error(f"\nИнформация о памяти:")
            for key, value in context.memory_info.items():
                self.log_error(f"  {key}: {value}")

        self.log_error("\n" + "=" * 80)
        self.log_error(f"Тотальных ошибок: {self.error_count}")
        self.log_error(f"Всего предупреждений: {self.warning_count}")
        self.log_error(f"Файл журнала: {self.log_file.absolute()}")

    def watch_memory(self, address: int) -> None:

        self.memory_watches[address] = time.time()
        self.log_verbose(f"Просмотр памяти добавлен: 0x{address:x}")

    def get_summary(self) -> str:

        elapsed = time.time() - self.start_time
        summary_lines = [
            "",
            "|" + "Отчет по отладке".center(78) + "|",
            f"  Оставшееся время: {elapsed:.3f}s",
            f"  Ошибки:       {self.error_count}",
            f"  Предупреждения:     {self.warning_count}",
            f"  Файл журнала:     {self.log_file.absolute()}",
        ]
        return "\n".join(summary_lines)

    def __enter__(self):

        self.log_info("Начало отладочной сессии")
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):

        if exc_type is not None:
            self.critical_dump(exc_val)
        else:
            self.log_info("Отладочная сессия завершена успешно")
        return False

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
