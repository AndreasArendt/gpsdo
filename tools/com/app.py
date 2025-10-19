try:
    # normal package import (when run as package)
    from .reader import main
except Exception:
    # fallback when running this file directly as a script
    import sys
    import os

    # project root is three levels up from this file: d:\Projekte
    pkg_root = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
    if pkg_root not in sys.path:
        sys.path.insert(0, pkg_root)

    from gpsdo.tools.com.reader import main


if __name__ == "__main__":
    # set VID/PID here (strings accepted in hex or decimal form)
    VID = "0x0483"   # example vendor id
    PID = "0x5740"   # example product id

    # Build argv for reader.main; this overrides command-line args
    argv = ["--vid", VID]
    if PID:
        argv += ["--pid", PID]

    # call main with the preset vid/pid
    main(argv)