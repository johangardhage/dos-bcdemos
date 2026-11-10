#!/usr/bin/env python3
"""Installs the DOS tools downloaded by download-dos-tools.sh into DOSBox,
producing the same result as running each tool's DOS INSTALL.EXE with the
options below. A given directory is the C: drive. Otherwise the C: drive
is the DOSBox directory.

  asmedit DOSBox/aedtinst -> DOSBox/ASMEDIT   (C:\\ASMEDIT)
  bc      DOSBox/bcsetup  -> DOSBox/BORLANDC  (C:\\BORLANDC, Windows capability: No)
  tasm    DOSBox/tasminst -> DOSBox/TASM      (C:\\TASM, Main + 16-bit command line + TD for DOS)
  tp      DOSBox/tpsetup  -> DOSBox/TP        (C:\\TP)

By default the installation is done natively (no DOS needed). With --dosbox,
each tool's real INSTALL.EXE is run in DOSBox Staging and driven by typing
the keys with AUTOTYPE. The DOSBox window shows the installer (don't type
into it); --headless runs DOSBox without a window. On slow machines,
--slowdown N makes all pauses between the typed keys N times longer.

Only the versions downloaded by download-dos-tools.sh are supported; the
setup files are checked before installing. If an installation fails or is
interrupted, the partly installed directory is removed.

Requires: unzip for bc and tp; 7zz (or 7z) for tasm; --dosbox: dosbox-staging
"""

import argparse
import datetime
import glob
import hashlib
import os
import random
import re
import shutil
import signal
import struct
import subprocess
import sys
import tempfile
import time
import zipfile


class Error(Exception):
    """A failure reported as 'error: <message>' and exit status 1."""


def die(message):
    raise Error(message)


def run_main(main):
    """Runs main() and turns errors into an error message and exit status."""
    # keep progress and error messages in order when the output is piped
    sys.stdout.reconfigure(line_buffering=True)
    try:
        sys.exit(main())
    except Error as e:
        print(f"error: {e}", file=sys.stderr)
        sys.exit(1)
    except KeyboardInterrupt:
        sys.exit(130)


def target_parser(description, epilog):
    """Returns an argument parser for '[directory] <target>...' command lines."""
    return argparse.ArgumentParser(
        usage="%(prog)s [options] [directory] <target>...",
        description=description,
        epilog=epilog,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )


def split_directory(argv, known):
    """Returns (directory, argv) after removing a leading destination directory.

    The first positional that is not a known target is the directory when
    another positional follows, or when it looks like a path. A lone path is
    removed so the caller prints help. Options stay in place.
    """
    positionals = []
    i = 1
    while i < len(argv):
        arg = argv[i]
        if arg == "--slowdown":
            i += 2
            continue
        if arg.startswith("-"):
            i += 1
            continue
        positionals.append(i)
        i += 1
    if not positionals:
        return None, argv
    first_i = positionals[0]
    first = argv[first_i]
    if first in known:
        return None, argv
    looks_like_dir = "/" in first or first in (".", "..")
    if len(positionals) == 1 and not looks_like_dir:
        return None, argv
    directory = first.rstrip("/") or first
    return directory, argv[:first_i] + argv[first_i + 1:]


def parse_targets(parser, all_targets, aliases=None, argv=None):
    """Parses the command line. Prints the help and exits if no target is given.

    all_targets lists the valid targets in order, aliases maps extra names
    (like 'all') to lists of targets. argv includes the program name.
    Returns (args, targets).
    """
    parser.add_argument("targets", nargs="*", metavar="target", help=argparse.SUPPRESS)
    args = parser.parse_args(None if argv is None else argv[1:])
    if not args.targets:
        parser.print_help()
        sys.exit(0)
    aliases = aliases or {}
    targets = []
    for t in args.targets:
        if t in aliases:
            targets += aliases[t]
        elif t in all_targets:
            targets.append(t)
        else:
            parser.print_help(sys.stderr)
            parser.exit(1, f"error: unknown target '{t}'\n")
    return args, targets


def need(program, hint=None):
    """Checks that a program is installed and returns its path."""
    path = shutil.which(program)
    if not path:
        die(f"{program} not found" + (f" ({hint})" if hint else ""))
    return path


def seven_zip():
    """Returns the 7-Zip command (7zz or 7z)."""
    for program in ("7zz", "7z"):
        if shutil.which(program):
            return program
    die("7zz/7z not found (install 7zip / sevenzip)")


def extract_7z(archive, dest, members=None):
    """Extracts any archive 7-Zip understands (7z, LhA, FAT disk images).

    members limits the extraction to the given names.
    """
    os.makedirs(dest, exist_ok=True)
    result = subprocess.run([seven_zip(), "x", "-y", f"-o{dest}", str(archive)]
                            + list(members or []),
                            stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
    if result.returncode != 0:
        die(f"extracting {os.path.basename(archive)} failed: {result.stderr.strip()}")


def extract_zip(archive, dest, members=None):
    """Extracts a zip archive with unzip, keeping the file timestamps.

    members limits the extraction to the given names. unzip decompresses the
    old PKZIP 1.x methods (Shrink, Implode) that most of these archives use,
    keeps the local DOS times as they are, and ignores the bogus attributes
    some old DOS zips have (7-Zip extracts those as broken symlinks).
    """
    name = os.path.basename(archive)
    if members is not None:
        try:
            with zipfile.ZipFile(archive) as z:
                missing = set(members) - set(z.namelist())
        except zipfile.BadZipFile as e:
            die(f"{name}: {e}")
        if missing:
            die(f"{', '.join(sorted(missing))} not found in {name}")
    os.makedirs(dest, exist_ok=True)
    # unzip takes member names as wildcard patterns; escape the wildcards
    patterns = [re.sub(r"([*?\[\]\\])", r"\\\1", m) for m in members or []]
    result = subprocess.run(["unzip", "-qq", "-o", str(archive), *patterns, "-d", dest],
                            stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
    if result.returncode != 0:
        die(f"extracting {name} failed: {result.stderr.strip()}")


class WorkDir:
    """A temporary directory, removed when the with block ends."""

    def __enter__(self):
        self.path = tempfile.mkdtemp()
        return self.path

    def __exit__(self, *exc):
        shutil.rmtree(self.path, ignore_errors=True)


def dos_datetime_to_mtime(date, tm):
    """Converts a DOS date and time word to a local timestamp."""
    t = datetime.datetime(1980 + (date >> 9), (date >> 5) & 15, date & 31,
                          tm >> 11, (tm >> 5) & 63, (tm & 31) * 2)
    return time.mktime(t.timetuple())

# target: (setup directory, install directory, name)
TOOLS = {
    "asmedit": ("aedtinst", "ASMEDIT", "ASM Edit 1.82b"),
    "bc": ("bcsetup", "BORLANDC", "Borland C++ 3.1 and Application Frameworks"),
    "tasm": ("tasminst", "TASM", "Borland Turbo Assembler 4.0"),
    "tp": ("tpsetup", "TP", "Borland Turbo Pascal 7.01"),
}

# Setup files of the supported versions (as downloaded by download-dos-tools.sh).
# The native installation depends on their exact contents, the --dosbox key
# sequences on the installer screens.
KNOWN_FILES = {
    "asmedit": {
        "INSTALL.EXE": "f2ac4c7789a8f97371ccb8b618fa40277d4475eb7135a55f43a9d915466766b9",
        "Asmedit.lib": "b2cdbd6f1bf24d500fc5701e07aaf362f8dd1a848a259047ca696de30ce05780",
    },
    "bc": {
        "INSTALL.EXE": "ebbfc9bb387b9a278fcabe78a4cb0545d5b46bf5ee0aafa4acfc63083ce3d265",
        "FILELIST.DOC": "3eaf729f5ddb718b76d12c6f7247c971019131c300aed44cb3ee4250ffa3346a",
    },
    "tasm": {
        "INSTALL.EXE": "46d7d2ea9a4f40ff9bda62edcaaa3e93d0b3fc1f90c3892832bc8915b5a9aeb4",
        "FILEINFO.INI": "e8549de6971ebe8b80a60ae5679aacfd89183c9ed9f30faec036ab36cc89b41a",
    },
    "tp": {
        "INSTALL.EXE": "d442f65760001f9a7781aab420d22f364aa71cafe34e6c8ea17ac09797347961",
        "TURBO.ZIP": "201c6341c1a98aa73c30d400e82c5df69b743a35cbf5939276bac96896845b67",
        "TPX.ZIP": "a8ab24113027b202ad140bde52e2286532f225d908e62921d26b2c082b1d57fa",
    },
}


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def check_version(t, src):
    for name, expected in KNOWN_FILES[t].items():
        path = os.path.join(src, name)
        if not os.path.isfile(path):
            die(f"{t}: {path} not found")
        if sha256(path) != expected:
            die(f"{t}: {path} is not the expected version "
                f"(re-download with download-dos-tools.sh {t})")


def dos_text(path, lines, eol_last=True):
    """Writes DOS text lines (CRLF separated). The last line gets a CRLF only
    if eol_last is set, matching what the installers write.
    """
    text = "".join(line + "\r\n" for line in lines)
    if not eol_last:
        text = text[:-2]
    with open(path, "wb") as f:
        f.write(text.encode("ascii"))


def patch_pstring(path, offset, string):
    """Writes a Turbo Pascal string (length byte + characters) at offset."""
    with open(path, "r+b") as f:
        f.seek(offset)
        f.write(bytes([len(string)]) + string.encode("ascii"))


def copy_files(src, names, dest_dir):
    """Copies files keeping their timestamps, like cp -p."""
    for name in names:
        shutil.copy2(os.path.join(src, name), os.path.join(dest_dir, name))


# ---------------------------------------------------------------------------
# Borland C++ 3.1
#
# Files are ZIP archives, or ZIP archives split over several disks (.CA1,
# .CA2, ...). Each part starts with a 5-byte header (disk count and offset)
# which is stripped before joining the parts.
# ---------------------------------------------------------------------------

# (archive, destination, only these files)
BC_ARCHIVES = [
    # Base
    ("BC", "BIN"), ("BIN", "BIN"), ("BINSRC", "BIN"), ("CMDLINE", "BIN"), ("IDE", "BIN"),
    ("INCLUDE", "INCLUDE"), ("BGI", "BGI"), ("DOC", "DOC"), ("EXAMPLES", "EXAMPLES"),
    # Libraries (all memory models)
    ("XLIB", "LIB"), ("SLIB", "LIB"), ("MLIB", "LIB"), ("CLIB", "LIB"), ("LLIB", "LIB"),
    ("HLIB", "LIB"), ("STARTUP", "LIB/STARTUP"),
    # Run-time library source
    ("BATSRC", "CRTL"), ("CLIBSRC", "CRTL/CLIB"), ("CLIBSRC1", "CRTL/CLIB"),
    ("CLIBSRC2", "CRTL/CLIB"), ("CLIBSRC3", "CRTL/CLIB"), ("EMUSRC", "CRTL/EMU"),
    ("IOSTRSRC", "CRTL/IOSTREAM"), ("IOSTRSR1", "CRTL/IOSTREAM"), ("MATHSRC", "CRTL/MATH"),
    ("RTLINSRC", "CRTL/RTLINC"), ("WINLBSRC", "CRTL/WINLIB"), ("INDEPSRC", "CRTL/WINLIB/INDEP"),
    # Class library
    ("CLASSBLD", "CLASSLIB"), ("CLASSEXM", "CLASSLIB/EXAMPLES"), ("CLASSINC", "CLASSLIB/INCLUDE"),
    ("CLASSLIB", "CLASSLIB/LIB"), ("CLASSSRC", "CLASSLIB/SOURCE"),
    # Turbo Vision
    ("TVDEMOS", "TVISION/DEMOS"), ("TVDOCDEM", "TVISION/DOCDEMOS"), ("TVHELP", "TVISION/HELP"),
    ("TVINC", "TVISION/INCLUDE"), ("TVLIB", "TVISION/LIB"), ("TVSRC", "TVISION/SOURCE"),
    # Tools: Turbo Assembler, Turbo Debugger, Turbo Profiler, Resource Workshop
    ("TASM", "BIN"), ("TD", "BIN"), ("TDUTIL", "BIN"), ("TPROF", "BIN"), ("RW", "BIN"),
    ("RWDOC", "DOC"), ("WHELP1", "BIN"), ("WIN31BIN", "BIN"), ("WBIN", "BIN", ["TOOLHELP.DLL"]),
    ("WIN31DEV", "REDIST"),
    # Examples
    ("TASMEXMP", "EXAMPLES/TASM"), ("WHEREIS", "EXAMPLES/TASM/WHEREIS"),
    ("FILT", "EXAMPLES/TASM/FILT"), ("HEAP", "EXAMPLES/TASM/HEAP"),
    ("QUEUE", "EXAMPLES/TASM/QUEUE"), ("TCALC", "EXAMPLES/TCALC"), ("TDEXMPL", "EXAMPLES/TD"),
    ("TDWEXMPL", "EXAMPLES/TD"), ("TFEXMPL", "EXAMPLES/TPROF"), ("RWCDEMO", "EXAMPLES/WORKSHOP"),
    ("WEXAMPLE", "EXAMPLES/WIN30"), ("CHELP31", "EXAMPLES/WIN31/CHELP"),
    ("CLIOLE", "EXAMPLES/WIN31/CLIOLE"), ("CMDLG31", "EXAMPLES/WIN31/CMDLG"),
    ("DDEML31", "EXAMPLES/WIN31/DDEML"), ("DRAG", "EXAMPLES/WIN31/DRAGDROP"),
    ("SOUNDER", "EXAMPLES/WIN31/SOUNDER"), ("TRUETYPE", "EXAMPLES/WIN31/TRUETYPE"),
    # ObjectWindows Library
    ("OWL", "OWL"), ("OWLDOC", "OWL/DOC"), ("OWLINC", "OWL/INCLUDE"), ("OWLLIB", "OWL/LIB"),
    ("OWLSRC", "OWL/SOURCE"), ("DLOBJ", "OWL/OBJECT/DL"), ("WLOBJ", "OWL/OBJECT/WL"),
    ("WMOBJ", "OWL/OBJECT/WM"), ("WSOBJ", "OWL/OBJECT/WS"), ("OWLDEMOS", "OWL/EXAMPLES"),
    ("CALC", "OWL/EXAMPLES/CALC"), ("CHECKERS", "OWL/EXAMPLES/CHECKERS"),
    ("CHESS", "OWL/EXAMPLES/CHESS"), ("CURSOR", "OWL/EXAMPLES/CURSOR"),
    ("FILEAPP", "OWL/EXAMPLES/FILEAPP"), ("GDIDEMO", "OWL/EXAMPLES/GDIDEMO"),
    ("MFILEAPP", "OWL/EXAMPLES/MFILEAPP"), ("OWLPRINT", "OWL/EXAMPLES/OWLPRINT"),
    ("PAINT", "OWL/EXAMPLES/PAINT"), ("PROGTALK", "OWL/EXAMPLES/PROGTALK"),
    ("STEPS", "OWL/EXAMPLES/STEPS"), ("SWAT", "OWL/EXAMPLES/SWAT"),
    ("SYSINFO", "OWL/EXAMPLES/SYSINFO"), ("TTT", "OWL/EXAMPLES/TTT"),
    ("ACLOCK", "OWL/EXAMPL31/ACLOCK"), ("CMDLGAP", "OWL/EXAMPL31/CMDLGAP"),
    ("DDEML", "OWL/EXAMPL31/DDEML"), ("DRAGDROP", "OWL/EXAMPL31/DRAGDROP"),
    ("OLE", "OWL/EXAMPL31/OLECLNT"), ("OLESRVR", "OWL/EXAMPL31/OLESRVR"),
    ("OWLHELP", "OWL/EXAMPL31/OWLHELP"), ("OWLPEN", "OWL/EXAMPL31/OWLPEN"),
    ("SCRNSAVE", "OWL/EXAMPL31/SCRNSAVE"), ("SHELLAPI", "OWL/EXAMPL31/SHELLAPI"),
    ("SOUNDPLY", "OWL/EXAMPL31/SOUNDPLY"), ("TTFONT", "OWL/EXAMPL31/TTFONT"),
]


def bc_zip(src, name, work):
    """Returns the zip for an archive, joining split .CA? parts if needed."""
    path = os.path.join(src, f"{name}.ZIP")
    if os.path.isfile(path):
        return path
    parts = sorted(glob.glob(os.path.join(glob.escape(src), f"{name}.CA?")))
    if not parts:
        die(f"{name}.ZIP not found in {src}")
    path = os.path.join(work, f"{name}.zip")
    with open(path, "wb") as out:
        for part in parts:
            with open(part, "rb") as f:
                out.write(f.read()[5:])
    return path


def install_bc(src, dest, work):
    print(f"Installing Borland C++ 3.1 to {dest}")
    for name, directory, *members in BC_ARCHIVES:
        print(f"  {name} -> {directory}")
        extract_zip(bc_zip(src, name, work), os.path.join(dest, directory),
                    members[0] if members else None)
    os.makedirs(os.path.join(dest, "TVISION", "OBJECT"))

    # The installer moves the sys/*.h headers into their own directory
    os.makedirs(os.path.join(dest, "INCLUDE", "SYS"))
    for h in ("LOCKING.H", "STAT.H", "TIMEB.H", "TYPES.H"):
        os.rename(os.path.join(dest, "INCLUDE", h), os.path.join(dest, "INCLUDE", "SYS", h))

    # Files copied from the installation disk
    copy_files(src, ["README", "README.COM", "FILELIST.DOC"], dest)
    copy_files(src, ["UNZIP.EXE"], os.path.join(dest, "BIN"))

    # Configuration files written by the installer
    print("  configuration files")
    bin_dir = os.path.join(dest, "BIN")
    dos_text(os.path.join(bin_dir, "TURBOC.CFG"),
             [r"-IC:\BORLANDC\INCLUDE", r"-LC:\BORLANDC\LIB"])
    dos_text(os.path.join(bin_dir, "TLINK.CFG"), [r"-LC:\BORLANDC\LIB"])
    dos_text(os.path.join(bin_dir, "THELP.CFG"),
             [r"/fC:\BORLANDC\BIN\TCHELP.TCH", r"/fC:\BORLANDC\BIN\TVCHELP.TCH",
              r"/fC:\BORLANDC\BIN\OWLHELP.TCH", r"/fC:\BORLANDC\BIN\TASM.TAH"], eol_last=False)
    for examples in ("EXAMPLES", "EXAMPL31"):
        dos_text(os.path.join(dest, "OWL", examples, "BCROOT.INC"), [r"BCROOT=C:\BORLANDC"],
                 eol_last=False)
    with open(os.path.join(dest, "REDIST", "PENWIN.INI"), "ab") as f:
        f.write(b"\r\nRecognizer=C:\\BORLANDC\\REDIST\\MARS.DLL\r\n\r\n[mars]\r\n"
                b"msdb=C:\\BORLANDC\\REDIST\\MARS.MOB")


# ---------------------------------------------------------------------------
# Turbo Pascal 7.01 (TFLOPPY.ZIP, TURBO.EXE for floppy-only systems, is not
# installed)
# ---------------------------------------------------------------------------
TP_ARCHIVES = [
    ("TURBO", "BIN"), ("TPX", "BIN"), ("TPC", "BIN"), ("TPL", "BIN"), ("HELP", "BIN"),
    ("DPMISYS", "BIN"), ("UTILS", "BIN"), ("BGIBIN", "BGI"), ("BGI", "UNITS"), ("UNITS", "UNITS"),
    ("TURBO3", "UNITS"), ("INTRFACE", "DOC"), ("ONLINE", "DOC"), ("SOURCE", "SOURCE"),
    ("DEMOS", "EXAMPLES"), ("BGIDEMOS", "EXAMPLES/BGI"), ("BREAKOUT", "EXAMPLES/BREAKOUT"),
    ("DOCDEMO", "EXAMPLES/DOCDEMOS"), ("TVDEBUG", "EXAMPLES/TVDEBUG"),
    ("TVDEMOS", "EXAMPLES/TVDEMO"), ("TVFM", "EXAMPLES/TVFM"),
]


def install_tp(src, dest, work):
    print(f"Installing Turbo Pascal 7.01 to {dest}")
    for name, directory in TP_ARCHIVES:
        print(f"  {name} -> {directory}")
        extract_zip(os.path.join(src, f"{name}.ZIP"), os.path.join(dest, directory))

    # Files copied from the installation disk
    copy_files(src, ["README", "README.COM", "FILELIST.DOC", "UNZIP.EXE"], dest)

    # Configuration written by the installer: unit directory and help files.
    # TURBO.TP and TPX.TP get the unit directory and the Turbo Vision help
    # file patched in as Pascal strings.
    print("  configuration files")
    bin_dir = os.path.join(dest, "BIN")
    dos_text(os.path.join(bin_dir, "TPC.CFG"), [r"/UC:\TP\UNITS"], eol_last=False)
    dos_text(os.path.join(bin_dir, "THELP.CFG"), [r"/fC:\TP\BIN\TURBO.TPH",
                                                  r"/fC:\TP\BIN\TVISION.TPH"])
    for name, units, help_file in (("TURBO.TP", 819, 3485), ("TPX.TP", 821, 3487)):
        patch_pstring(os.path.join(bin_dir, name), units, r"C:\TP\UNITS")
        patch_pstring(os.path.join(bin_dir, name), help_file, r"C:\TP\BIN\TVISION.TPH")


# ---------------------------------------------------------------------------
# Turbo Assembler 4.0
#
# .PAK files are LHA archives. The groups are listed in FILEINFO.INI:
# MAIN, Cmd16 and TDDOS are installed (16-bit tools for DOS).
# ---------------------------------------------------------------------------
TASM_PAKS = [
    ("FILELIST", ""), ("GROUPS", "BIN"),                                       # MAIN
    ("CMD16", "BIN"), ("CMDLINE", "BIN"), ("CMDINC", "INCLUDE"), ("TA16LIB", "LIB"),  # Cmd16
    ("TDDOS", "BIN"),                                                          # TDDOS
]


def install_tasm(src, dest, work):
    print(f"Installing Turbo Assembler 4.0 to {dest}")
    for name, directory in TASM_PAKS:
        print(f"  {name} -> {directory or '.'}")
        extract_7z(os.path.join(src, f"{name}.PAK"), os.path.join(dest, directory))

    # Files copied from the installation disk
    copy_files(src, ["TSM_INST.TXT", "README.TXT", "README.COM", "UNPAK.EXE"], dest)

    print("  configuration files")
    dos_text(os.path.join(dest, "BIN", "TLINK.CFG"), [r"-LC:\TASM\LIB"])

    # Program group for Windows (GROUPS.EXE is run by Windows to create it).
    # The installer writes the group header and the MAIN entries before it
    # creates C:\WINDOWS, so without an existing C:\WINDOWS only the TDDOS
    # entries end up in GROUPS.B$$.
    print("  WINDOWS program group")
    windows = os.path.join(os.path.dirname(dest), "WINDOWS")
    groups = []
    if os.path.isdir(windows):
        groups = ["[group1]", "caption=Borland Tasm 4.0", "file=TASM40.GRP", "[icons1]",
                  r"TASM.HLP=C:\TASM\BIN,Tasm Reference, HELP.ICO"]
    groups += [r"TD.PIF=C:\TASM\BIN,Turbo Debugger for DOS, TDDOS.ICO",
               r"TDINST.EXE=C:\TASM\BIN,TD Configuration, TDOSINST.ICO"]
    os.makedirs(windows, exist_ok=True)
    copy_files(os.path.join(dest, "BIN"), ["GROUPS.EXE", "HELP.ICO", "TDDOS.ICO", "TDOSINST.ICO"],
               windows)
    dos_text(os.path.join(windows, "GROUPS.B$$"), groups)


# ---------------------------------------------------------------------------
# ASM Edit 1.82b
#
# Asmedit.lib is ASM Edit's own archive format, unpacked here the same way as
# INSTALL.EXE does it:
#
#   entry:  name    string[13] (Pascal string, 14 bytes)
#           size    longint
#           time    word (DOS)
#           date    word (DOS)
#           attr    byte ($10 = directory, name '..' = leave directory)
#           unused  4 bytes
#   file data follows as blocks until size bytes are produced:
#           method  byte (0 = stored, 1 = compressed)
#           length  word
#           data    length bytes
#
# The first entry is the archive header ('O Love & Tels'), the last is 'EOL'.
#
# Compressed blocks are a static Huffman code (tree built from a fixed
# frequency table, symbol 256 = 8 bit literal) followed by a byte code
# expansion (dictionary words and character runs). The tables are from
# INSTALL.EXE.
#
# INSTALL.EXE also stamps the install date into ASMSHELL.EXE.
# ---------------------------------------------------------------------------
FREQ = [
    528, 29831, 306, 23852, 5168, 2617, 3659, 1086, 1819, 938,
    822, 795, 843, 24629, 668, 643, 388, 297, 206, 176,
    235, 109, 150, 533, 311, 363, 487, 105, 434, 483,
    197, 293, 109537, 512, 352, 1281, 74, 209, 400, 2594,
    5902, 5437, 218, 2097, 4378, 3521, 4812, 1625, 7598, 6185,
    4401, 3708, 2860, 2883, 2974, 1853, 3167, 941, 3012, 670,
    1057, 4447, 765, 1706, 52, 4911, 2817, 4704, 4747, 4167,
    5687, 1460, 1371, 4886, 685, 462, 2922, 3973, 2952, 3968,
    4338, 486, 2517, 6531, 5666, 1654, 1093, 1110, 1889, 661,
    640, 684, 120, 215, 145, 185, 224, 17194, 5566, 11024,
    10989, 32203, 9743, 5326, 6758, 20524, 324, 2769, 10328, 6750,
    17403, 19602, 6379, 589, 17910, 18787, 21758, 8623, 3509, 2853,
    1656, 3283, 450, 19, 173, 37, 39, 22, 39, 22,
    33, 19, 16, 120, 19, 26, 22, 55, 22, 49,
    29, 17, 30, 69, 9, 41, 69, 10, 24, 25,
    23, 34, 22, 52, 23, 28, 32, 16, 17, 26,
    12, 16, 15, 20, 31, 15, 28, 28, 16, 13,
    23, 29, 23, 26, 15, 32, 22, 45, 16, 4717,
    88, 12, 17, 26, 24, 23, 98, 29, 40, 21,
    25, 142, 729, 533, 129, 17, 1515, 26, 19, 37,
    18, 26, 23, 24, 18, 39, 19, 51, 19, 13,
    27, 25, 10, 26, 16, 20, 22, 142, 154, 1132,
    81, 15, 30, 1172, 22, 27, 34, 39, 15, 19,
    21, 22, 14, 19, 27, 26, 34, 22, 30, 22,
    20, 39, 87, 30, 19, 24, 23, 21, 38, 28,
    18, 25, 29, 26, 1839, 3729, 1,
]

DICT = [
    b'286', b'386', b'486', b'808x', b'Architecture', b'Bytes', b'Bit',
    b'Character', b'Control', b'Clocks', b'CPU', b'Description',
    b'Descriptor', b'Debug', b'Exceptions', b'Flags', b'FPU ', b'Horizontal',
    b'Interrupt', b'Jump', b'Management', b'Modifies ', b'Memory', b'Not',
    b'Operations', b'Operands', b'Register', b'Segment', b'Size', b'Set',
    b'Terminology', b'This', b'The', b'Usage:', b'Vertical', b'automatically',
    b'assembling', b'available', b'address', b'and', b'all', b'are',
    b'breakpoint', b'byte', b'bits', b'calculate', b'character',
    b'coprocessor', b'comparison', b'condition', b'consists', b'contains',
    b'control', b'code', b'can', b'documented', b'destination', b'decrement',
    b'ditor', b'data', b'error', b'for', b'from', b'file', b'generated',
    b'hardware ', b'information', b'interrupt', b'implement', b'immediate',
    b'increment', b'invalid', b'index', b'jump', b'line', b'multiple',
    b'mode', b'memory', b'mem16', b'nstruction', b'not', b'none',
    b'operation', b'osition', b'operand', b'only', b'processor', b'program',
    b'pointer', b'register', b'reserved', b'return', b'round', b'reg16',
    b'reg32', b'software', b'segment', b'select', b'ssemble', b'set',
    b'see below', b'see:', b'the', b'this', b'window', b'with', b'will',
    b'you', b'your', b'\xb37\xb36\xb35\xb34\xb33\xb32\xb3',
    b'\xc5\xc4\xc4\xc4\xc4\xc4\xc4', b' \xb3 \xb3 \xb3 ', b'\xc3\xc4\xc4',
    b'\xc4\xc4\xb4',
]

RUN_CHARS = {3: 0x20, 4: 0xC4, 5: 0xDC}   # runs of space, '─' and '▄'


def build_tree():
    """Builds the Huffman tree exactly like INSTALL.EXE: repeatedly joins the
    two active nodes with the lowest frequency (first found wins), lowest is
    the left child. Nodes are [freq, left, right, symbol, active], leaves have
    left = None.
    """
    nodes = [[f, None, None, sym, True] for sym, f in enumerate(FREQ) if f]
    active = len(nodes)
    while active > 1:
        node = [0, None, None, None, True]
        for side in (1, 2):
            best = None
            for i, n in enumerate(nodes):
                if n[4] and (best is None or n[0] < nodes[best][0]):
                    best = i
            nodes[best][4] = False
            node[side] = best
            node[0] += nodes[best][0]
        nodes.append(node)
        active -= 1
    return nodes


def huffman(nodes, data):
    if data[0] == 0xFF:
        count, pos = (data[1] << 8) | data[2], 3
    else:
        count, pos = data[0], 1
    bits = int.from_bytes(data[pos:] + b"\0\0", "big")
    nbits = (len(data) - pos + 2) * 8
    bp = 0
    out = bytearray()
    root = len(nodes) - 1
    while len(out) < count:
        i = root
        while nodes[i][1] is not None:
            bit = (bits >> (nbits - 1 - bp)) & 1
            bp += 1
            i = nodes[i][2] if bit else nodes[i][1]
        sym = nodes[i][3]
        if sym == 256:
            sym = (bits >> (nbits - 8 - bp)) & 0xFF
            bp += 8
        out.append(sym)
    return out


def expand(data):
    out = bytearray()
    i = 0
    while i < len(data):
        c = data[i]
        if c == 1:                      # dictionary word
            out += DICT[data[i + 1]]
            i += 2
        elif c == 2:                    # escaped byte
            out.append(data[i + 1] + 1)
            i += 2
        elif c in RUN_CHARS:            # run of space, '─' or '▄'
            out += bytes([RUN_CHARS[c]]) * data[i + 1]
            i += 2
        elif c == 6:                    # run of any character
            out += bytes([data[i + 2]]) * data[i + 1]
            i += 3
        else:
            out.append(c)
            i += 1
    return out


def stamp_shell(path):
    """Install date stamp at end-0x419: 1018 random bytes (0..252), the date
    twice (high byte first) and a checksum of all bytes + 3.
    """
    t = time.localtime()
    date = ((t.tm_year - 1980) * 12 + t.tm_mon) * 31 + t.tm_mday
    hi, lo = date >> 8 & 0xFF, date & 0xFF
    stamp = bytes(random.randrange(0xFD) for _ in range(0x3FA)) + bytes([hi, lo, hi, lo])
    check = (sum(stamp) + 3) & 0xFFFF
    stamp += bytes([check >> 8, check & 0xFF])
    with open(path, "r+b") as f:
        f.seek(-0x419, 2)
        f.write(stamp)


def install_asmedit(src, dest, work):
    print(f"Installing ASM Edit 1.82b to {dest}")
    os.makedirs(dest)
    nodes = build_tree()
    with open(os.path.join(src, "Asmedit.lib"), "rb") as f:
        lib = f.read()
    path = []
    pos = 0
    first = True
    while pos < len(lib):
        name = lib[pos + 1:pos + 1 + lib[pos]].decode("latin-1")
        size, tm, date, attr = struct.unpack("<IHHB", lib[pos + 14:pos + 23])
        pos += 27
        if first:
            if name != "O Love & Tels":
                die("Asmedit.lib: bad header")
            first = False
            continue
        if name == "EOL":
            break
        if name == "..":
            path.pop()
            continue
        if attr & 0x10:
            path.append(name)
            os.makedirs(os.path.join(dest, *path), exist_ok=True)
            continue
        out = bytearray()
        while len(out) < size:
            method = lib[pos]
            length = struct.unpack("<H", lib[pos + 1:pos + 3])[0]
            block = lib[pos + 3:pos + 3 + length]
            pos += 3 + length
            out += expand(huffman(nodes, block)) if method == 1 else block
        if len(out) != size:
            die(f"Asmedit.lib: {name}: bad size")
        file = os.path.join(dest, *path, name)
        with open(file, "wb") as f:
            f.write(out)
        print("  " + "\\".join(path + [name]))
        if name == "ASMSHELL.EXE" and not path:
            stamp_shell(file)
        else:
            mtime = dos_datetime_to_mtime(date, tm)
            os.utime(file, (mtime, mtime))


NATIVE = {"asmedit": install_asmedit, "bc": install_bc, "tasm": install_tasm, "tp": install_tp}

# ---------------------------------------------------------------------------
# --dosbox: run the real INSTALL.EXE in DOSBox Staging
#
# The DOSBox directory is mounted as C:, INSTALL.EXE is started from the
# setup directory and the keys below are typed with AUTOTYPE, after WAIT
# seconds and then one key every PACE seconds (',' = one extra PACE).
# DOSBox exits when INSTALL.EXE quits. The Borland installers also open a
# README viewer, and DOSBox waits until that quits too.
#
# The installers flush the keyboard while busy, so the pauses after starting
# the installation are generous. The Borland sequences end by closing the
# README viewer with Esc several times; Esc after that is harmless.
# ---------------------------------------------------------------------------
DOSBOX_TIMEOUT = 300
README = "enter , , , , esc , , esc , , esc , , esc"

# target: (wait, pace, keys)
DOSBOX_KEYS = {
    # Welcome OK, File|Begin install, target C:\ASMEDIT OK, create path OK,
    # install successful OK, File|Exit
    "asmedit": (2, 2, "enter f10 enter enter enter enter , , , , , enter f10 enter down enter"),
    # Welcome, source drive C, source path \BCSETUP, then
    # Windows Options... -> WINDOWS Capability: No, Start (F9)
    "bc": (2, 2, "enter c enter enter down down down down enter enter esc f9 "
                 ", , , , , , , , , , " + README),
    # Welcome, source drive C, source path \TASMINST, then set 32-bit tools,
    # TD for Windows, TD for Win32, Examples and Documentation to No, Start (F9)
    "tasm": (2, 1, "enter c enter enter down down down enter down enter "
                   "down down enter down enter down enter f9 , , , , , , , , , , "
                   + README + " , , , , esc"),
    # Welcome, source path \TPSETUP, Hard Drive (the menu ignores the first
    # Enter), Start (F9)
    "tp": (2, 2, "enter enter enter enter f9 , , , , , , " + README),
}

# Files that must exist after a successful installation
DOSBOX_CHECK = {
    "asmedit": ["ASMEDIT/ASMEDIT.EXE", "ASMEDIT/ASMSHELL.EXE", "ASMEDIT/ASMEDIT.HLP"],
    "bc": ["BORLANDC/BIN/BC.EXE", "BORLANDC/BIN/TURBOC.CFG", "BORLANDC/BIN/THELP.CFG"],
    "tasm": ["TASM/BIN/TASM.EXE", "TASM/BIN/TLINK.CFG", "WINDOWS/GROUPS.B$$"],
    "tp": ["TP/BIN/TURBO.EXE", "TP/BIN/TPC.CFG", "TP/BIN/THELP.CFG"],
}


def install_dosbox(t, dosbox, args, work):
    need("dosbox-staging")
    src = TOOLS[t][0]
    wait, pace, keys = DOSBOX_KEYS[t]
    # AUTOTYPE allows at most 30 s wait and 10 s pace
    wait = min(wait * args.slowdown, 30)
    pace = min(pace * args.slowdown, 10)
    timeout = DOSBOX_TIMEOUT * args.slowdown

    env = dict(os.environ)
    if args.headless:
        print(f"Running {src}\\INSTALL.EXE in DOSBox (headless)", flush=True)
        env["SDL_VIDEODRIVER"] = "dummy"
    else:
        print(f"Running {src}\\INSTALL.EXE in DOSBox (don't type into the window)", flush=True)
    command = [
        "dosbox-staging", "--noprimaryconf", "--nolocalconf",
        "--set", "cpu_cycles=max", "--set", "nosound=true",
        "-c", f'mount c "{dosbox}"',
        "-c", "c:",
        "-c", f"cd \\{src}",
        "-c", f"autotype -w {wait:g} -p {pace:g} {keys}",
        "-c", "install.exe",
        "-c", "exit",
    ]
    log_path = os.path.join(work, f"dosbox-{t}.log")
    with open(log_path, "w") as log:
        process = subprocess.Popen(command, stdout=log, stderr=subprocess.STDOUT, env=env)
        try:
            status = process.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            die(f"{t}: DOSBox did not exit within {timeout:g}s, the installer is probably "
                f"waiting for input (try --slowdown 2 on a slow machine)")
        finally:
            if process.poll() is None:
                process.kill()
                process.wait()
    if status != 0:
        with open(log_path) as log:
            sys.stderr.write(log.read())
        die(f"{t}: dosbox-staging failed")

    for f in DOSBOX_CHECK[t]:
        if not os.path.isfile(os.path.join(dosbox, f)):
            die(f"{t}: installation incomplete, {os.path.join(dosbox, f)} is missing "
                f"(try --slowdown 2 on a slow machine)")


def main():
    parser = target_parser(
        "Installs the DOS tools downloaded by download-dos-tools.sh into DOSBox.",
        "A given directory is the C: drive. Otherwise tools are installed "
        "from and to DOSBox.\n\nTargets:\n  all         every tool below\n"
        + "".join(f"  {t:<11} {name:<43} ({src} -> {dest})\n"
                  for t, (src, dest, name) in TOOLS.items()))
    parser.add_argument("--dosbox", action="store_true",
                        help="run the real INSTALL.EXE in DOSBox Staging instead")
    parser.add_argument("--headless", action="store_true", help="run DOSBox without a window")
    parser.add_argument("--slowdown", type=float, default=1, metavar="N",
                        help="make the pauses between typed keys N times longer (e.g. 2)")
    directory, argv = split_directory(sys.argv, set(TOOLS) | {"all"})
    args, targets = parse_targets(parser, list(TOOLS), {"all": list(TOOLS)}, argv)
    if args.slowdown < 1:
        die("--slowdown must be a number >= 1")
    if not args.dosbox and (args.headless or args.slowdown != 1):
        die("--headless and --slowdown only apply with --dosbox")
    if not args.dosbox and {"bc", "tp"} & set(targets):
        need("unzip")
    if not args.dosbox and "tasm" in targets:
        seven_zip()

    # SIGTERM cleans up like Ctrl-C
    signal.signal(signal.SIGTERM, lambda *_: sys.exit(143))

    dosbox = directory or os.path.join(os.getcwd(), "DOSBox")
    for t in targets:
        src_name, dest_name, _ = TOOLS[t]
        src = os.path.join(dosbox, src_name)
        dest = os.path.join(dosbox, dest_name)
        if os.path.exists(dest):
            print(f"Skipping {t}: {dest} already exists")
            continue
        if not os.path.isdir(src):
            die(f"{src} not found (run download-dos-tools.sh {t} first)")
        check_version(t, src)

        # Remove what this installation creates if it doesn't complete
        cleanup = [dest]
        windows = os.path.join(dosbox, "WINDOWS")
        if t == "tasm" and not os.path.exists(windows):
            cleanup.append(windows)
        try:
            with WorkDir() as work:
                if args.dosbox:
                    install_dosbox(t, dosbox, args, work)
                else:
                    NATIVE[t](src, dest, work)
        except BaseException:
            for d in cleanup:
                if os.path.exists(d):
                    print(f"Removing incomplete installation: {d}", file=sys.stderr)
                    shutil.rmtree(d)
            raise
        print(f"Done: {dest}")


if __name__ == "__main__":
    run_main(main)
