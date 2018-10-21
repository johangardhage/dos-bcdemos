# dos-bcdemos

Retro programming in Borland C++ 3.1

![Screenshot](/screenshots/bc.png "Borland C++ IDE")

## Prerequisites

To build and run the Borland C++ demos, you must first install the following tools:

- [DOSBox](https://www.dosbox.com/download.php)
- [Borland C++](https://winworldpc.com/product/borland-c/30)

### Install DOSBox

#### openSUSE

`$ sudo zypper install dosbox mtools p7zip-full`

#### Ubuntu

`$ sudo apt install dosbox mtools p7zip-full`

#### Configuration

When starting `dosbox` the first time, the configuration file `~/.dosbox/dosbox-0.74-2.conf` will be generated

### Install Borland C++

1. Download `Borland C++ 3.1 & Application Frameworks (3.5).7z`

1. Create a directory which will contain the DOS C: drive
   ```
   $ mkdir ~/DOSBox
   ```

1. Extract the downloaded Borland C++ archive
   ```
   $ 7z x "Borland C++ 3.1 & Application Frameworks (3.5).7z"
   ```

1. Extract the Borland C++ disk images
   ```
   $ cd "Borland C++ 3.1 & Application Frameworks (3.5)"/
   $ mkdir bcsetup
   $ for i in *.img; do echo $i; mcopy -m -i $i :: bcsetup; done
   ```

1. Move the extracted files to the DOS C: drive
   ```
   $ mv bcsetup ~/DOSBox/
   ```

1. Configure DOSBox

   Edit `~/.dosbox/dosbox-0.74-2.conf` and add the following autoexec options
   ```
   [autoexec]
   mount c ~/DOSBox
   path %PATH%;C:\BORLANDC\BIN
   c:
   ```

1. Start `dosbox` and execute the Borland C++ installation program
   ```
   $ dosbox
   C:\> cd bcsetup
   C:\BCSETUP> install.exe
   ```
   In the installation program, select the following options
   ```
   Enter the SOURCE drive to use: C
   Enter the SOURCE Path: \BCSETUP
   Windows Options... WINDOWS Capability: No
   Start Installation
   ```

## Build demos

Link the `dos-bcdemos` git repository to the DOS C: drive
```
$ ln -s ~/git/github/dos-bcdemos ~/DOSBox/bcdemos
```

#### Build demos from DOS terminal

1. Execute build script
   ```
   C:\BCDEMOS> buildall.bat
   ```
   The demos will be located in the `C:\BCDEMOS\BUILD` directory

1. Run demo
   ```
   C:\BCDEMOS> build\demo01.exe
   ```

#### Build demos from Borland C++ IDE

1. Start Borland C++ IDE
   ```
   C:\BCDEMOS> bc
   ```

1. Configure Borland C++

   Press `ALT+O` for options

   Select `Directories` and type in the following directories
   ```
   Include Directories: C:\BORLANDC\INCLUDE;C:\BCDEMOS\SRC
   Library Directories: C:\BORLANDC\LIB
   Output Directory: C:\BCDEMOS\BUILD
   ```

   Select `Debugger` and the following options
   ```
   Program Heap Size: 512 K Bytes
   ```

   Select `Environment`, `Preferences` and the following options
   ```
   [ ] Auto-Save
   ```

   Select `Environment`, `Editor` and the following options
   ```
   [ ] Create backup files
   ```

   Select `Save`

1. Open demo source file

   Press `F3` to open file

1. Build demo source file

   Press `F9` to build file

1. Run demo

   Press `ALT+R` and `R` to run demo

## License

Licensed under MIT license. See [LICENSE](LICENSE) for more information.

## Authors

* Johan Gardhage
* Original code written by [Ben Houston](https://github.com/bhouston/3DMaskDemo1997)

## Screenshots

![Screenshot](/screenshots/demo01.png "Flat shading")
![Screenshot](/screenshots/demo02.png "Gouraud shading")
![Screenshot](/screenshots/demo03.png "Phong shading")
![Screenshot](/screenshots/demo04.png "Environment shading")
![Screenshot](/screenshots/demo05.png "Transparent motionblur environment shading")
![Screenshot](/screenshots/demo06.png "Texture mapping")
![Screenshot](/screenshots/demo07.png "Flat shaded texture mapping")
![Screenshot](/screenshots/demo08.png "Gouraud shaded texture mapping")
![Screenshot](/screenshots/demo09.png "Phong shaded texture mapping")
![Screenshot](/screenshots/demo10.png "Phong shaded texture bump mapping")
