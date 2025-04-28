# TabsToSpaces

A simple command line utility to convert tabs to spaces and change new-lines (to LF or to CRLF).

Command line parameters:

- `--help` to show informational message (v.0.9);
- `--width=n` or `-w:n` where `n` is an integer sets tab width in spaces to `n` (v.0.9);
- `--crlf` make all new-lines to be CR LF pairs (v.1.0);
- `--lf` make all new-lines to be single LF characters (v.1.0);
- *other*: source file names to be converted (in-place).

Default tab width is 4 spaces. Command line parameters are processed one by one and if you pass a file name and only then change the tab width, then your file will be processed using the default tab width setting. The same is true for CRLF/LF settings. Thus, all settings are applied only to the files that follow them.

