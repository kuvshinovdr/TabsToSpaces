# TabsToSpaces v.1.0beta

A simple command line utility to convert tabs to spaces and change new-lines (to LF or to CRLF).

Command line parameters:

- `--help` to show informational message;
- `--width=n` or `-w:n` where `n` is an integer sets tab width in spaces to `n`;
- `--crlf` make all new-lines to be CR LF pairs;
- `--lf` make all new-lines to be single LF characters;
- `--trim` delete redundant spaces and tabs before new-lines (disabled by default);
- *other*: source file names to be converted (in-place).

File names (but not parent directory paths) may contain wildcard characters: `*` (matching any sequence of characters) and `?` (matching any single character). The pattern is not applied recursively to the nested directories (though this is planned to be added).

Default tab width is 4 spaces. Command line parameters are processed one by one and if you pass a file name and only then change the tab width, then your file will be processed using the default tab width setting. The same is true for CRLF/LF settings. Thus, all settings are applied only to the files that follow them.

Single CRs are left intact in any mode.

Disclaimer: this is a beta version and it is not well-tested.

The source code uses ISO C++23 and is intended to be cross-platform. Current builds are done in MSVC 2022 for Windows x64.
