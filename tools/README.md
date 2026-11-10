# Download scripts

## download-dos-tools.sh

Tools are written to `DOSBox`. Every download is checked against its SHA-256 before it is extracted.

| Target | Destination | Name | Download |
| --- | --- | --- | --- |
| `asmedit` | `aedtinst` | ASM Edit 1.82b | [aedt182b.zip](http://www.o-love.net/asmedit/) |
| `bc` | `bcsetup` | Borland C++ 3.1 and Application Frameworks | [Borland CPP 3.1 and Application Frameworks (1992) (3.5-1.44mb).7z](https://winworldpc.com/product/borland-c/30) |
| `fprot` | `fprot` | F-PROT 3.16f | [f-prot.zip](https://www.dosdays.co.uk/topics/antivirus_utilities.php) |
| `nc` | `ncsetup` | Norton Commander 5.51 | [Norton Commander 5.51 (3.5).7z](https://winworldpc.com/product/norton-commander/55x) |
| `tasm` | `tasminst` | Borland Turbo Assembler 4.0 | [Borland Turbo Assembler 4.0 (3.5-1.44mb).7z](https://winworldpc.com/product/turbo-assembler/4x) |
| `tbav` | `tbavinst` | Thunderbyte Anti-Virus 8.03 | [tbdos803.zip](https://www.dosdays.co.uk/topics/antivirus_utilities.php) |
| `tp` | `tpsetup` | Borland Turbo Pascal 7.01 | [Borland Turbo Pascal 7.01 (3.5-720k).7z](https://winworldpc.com/product/turbo-pascal/7x) |

## install-dos-tools.py

Tools are installed to `DOSBox`, the DOSBox C: drive, the same way as their DOS `INSTALL.EXE` would, without running DOS. The setup files are checked before installing; only the versions downloaded by `download-dos-tools.sh` are supported. If an installation fails or is interrupted, the partly installed directory is removed.

| Target | Source | Destination | Name | Installer options |
| --- | --- | --- | --- | --- |
| `asmedit` | `aedtinst` | `ASMEDIT` | ASM Edit 1.82b | `C:\ASMEDIT` |
| `bc` | `bcsetup` | `BORLANDC` | Borland C++ 3.1 and Application Frameworks | `C:\BORLANDC`, Windows capability: No |
| `tasm` | `tasminst` | `TASM`, `WINDOWS` | Borland Turbo Assembler 4.0 | `C:\TASM`, 16-bit command line tools, Turbo Debugger for DOS |
| `tp` | `tpsetup` | `TP` | Borland Turbo Pascal 7.01 | `C:\TP` |

With `--dosbox`, the real `INSTALL.EXE` is run in DOSBox Staging instead, and the installer options are typed with `AUTOTYPE`. Both ways give the same result.

| Option | Description |
| --- | --- |
| `--dosbox` | run the real `INSTALL.EXE` in DOSBox Staging; don't type into the DOSBox window |
| `--headless` | run DOSBox without a window |
| `--slowdown N` | make the pauses between typed keys N times longer, for slow machines |
