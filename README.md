# MDRipper
![Screenshot ripper 2023-01-06 212415](https://user-images.githubusercontent.com/59138188/211104555-468505eb-83bb-49e6-826c-de8a2d7086fb.png)

## Description
This is a Microsoft Windows program to extract Level 7 (admin) username and password data from an Ericsson/Aastra MD110 PABX.
MDRipper can work on files MD110 releases from BC8 to BC13 (Marketed as MX-One Telephony Switch, TSW)
It works using the data backup file for LIM 1 (L001) or a disk image file of the HDU, so physical access to the system is required.<br>

## Source code

This repository contains the source code files to build that application and example data files:

* `L001.bc10`       - A LIM 1 data file for the BC10 release.
* `L001.bc13`       - A LIM 1 data file for the BC13 release.
* `L001.horsham`    - A LIM 1 data file for the BC13 release.
