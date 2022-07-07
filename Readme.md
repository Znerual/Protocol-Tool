# Introduction
The protocol-tool is a command-line tool that allows you to efficiently manage your notes. You can easily create, search and view your notes and protocols by 
using a handfull of command-line commands. At the same time, you can use your favorite markdown editor to create and edit your notes and view them in 
a variety of different output formats (if pandoc is installed, else only markdown is available). Currently, the formats html, docx, markdown, latex and 
pdf (if latex is installed), are available.

An overview over all implemented commands can be found inside the program by running the command `help`. Additionally, the command `help` can be combined with
a command name (in the form of `help command_name`) to show detailed options for that command. 

# Installation
The protocol-tool can be run in a portable, standalone mode or be installed. If you decide to use the portable mode, download the `.exe` and `para.conf` files 
and place them in the same directory. Alternatively, the program can be installed using the `.msi` file. Paths will be taken care of and 
also the option to autostart the program on startup exists. Additionally, shortcuts to the desktop and programs menu will be created.

Currently, only windows 64 bit is supported.

# Advanved Features
If the programm detects that pandoc is not installed it will ask you to install it. Although the tool itself is self sufficient and will work fine without pandoc, many
advances features (various ouput formats for opening and merging the notes) are not available without pandoc. If pandoc is installed on the system but the tool can not
find it, make sure pandoc can be started by the command line by running `pandoc -v` in the command line. Add pandoc to the path if this test fails.

# Editor
The notes itself are edited in the markdown language and the default editor of the system will be used by this tool to create and edit the notes. Some options are 
for instance Visual Studio Code, or Notepad++ with various addon like [MarkdownViewerPlus](https://github.com/nea/MarkdownViewerPlusPlus) or [MarkdownPlusPlus](https://github.com/Edditoria/markdown-plus-plus)
which both can edit and view markdown files.

# Data Backup
In order to backup your data, simply copy the folders `Files` and `Data` inside the `Notes` directory. If you additionally want to backup your settings and modes, also copy the 
`para.conf` file. In order to restore your data, simply move the files back to their original folder and either run the update command to refresh the file lists or restart the program
to also re-read the configuration settings. 

# Bug Reporting
Please open a new issue if you find a bug, including the `log.txt` file and the `para.conf` file. The log file can be found at the `TMP_PATH`, which lies inside the `Notes\Tmp` directory inside the 
installation folder, when installed with the .msi installer. The configuration file `para.conf` lies directly in the installation folder, when installed with the `.msi` installer and should lie in 
the same directory as the executable file for the portable installation.