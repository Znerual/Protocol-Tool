# Installation
This programm is a portable, standalone, command line tool that does not need to be installed. Just place the ProtocolTool.exe and the para.conf in the same directory and you 
are good to go. Currently, only a windows version is available.

# Advanved Features
If the programm detects that pandoc is not installed it will ask you to install it. Although the tool itself is self sufficient and will work fine without pandoc, many
advances features (various ouput formats for opening and merging the notes) are not available without pandoc. If pandoc is installed on the system but the tool can not
find it, make sure pandoc can be started by the command line by running `pandoc -v` in the command line. Add pandoc to the path if this test fails.

# Editor
The notes itself are edited in the markdown language and the default editor of the system will be used by this tool to create and edit the notes. Some options are 
for instance Visual Studio Code, or Notepad++ with various addon like [MarkdownViewerPlus](https://github.com/nea/MarkdownViewerPlusPlus) or [MarkdownPlusPlus](https://github.com/Edditoria/markdown-plus-plus)
which both can edit and view markdown files.

# Bug Reporting
Please open a new issue if you find a bug, also include the log.txt file which can be found at the `TMP_PATH` (usually the TMP_PATH lies directly beneath the base_path in the folder TmpEnvironment)