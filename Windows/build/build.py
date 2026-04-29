import sys
import subprocess
import os
from pathlib import Path

def main():
    current_dir = Path(__file__).parent.resolve()
    target_script = current_dir / "cblerr.py" # CBlerr Compilator file for code start (TODO)

    command = [sys.executable, str(target_script)] + sys.argv[1:]

    try:
        result = subprocess.run(command, check=False)
        sys.exit(result.returncode)
    except KeyboardInterrupt:
        print("\nThe build was aborted by the user.")
        sys.exit(130)
    except Exception as e:
        print(f"Failed to run compiler: {e}.")
        sys.exit(1)

if __name__ == "__main__":
    main()
