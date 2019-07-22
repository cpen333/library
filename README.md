# Processes and Threads Library and Examples

Course library and examples for CPEN333 - System Software Engineering (University of British Columbia)

Includes a cross-platform header-only library for
- Process/thread synchronization
- Inter-process communication

Also contains usage examples that will be valuable for the labs.

Usage:
- Add the `include` directory to the list of "Include directories" for your compiler
- Include the desired headers in your program (see `examples` folder for examples)


## Documentation

The main library is heavily documented in-code using [Doxygen](http://www.stack.nl/~dimitri/doxygen/download.html).  A compiled version can be viewed here:
- https://cpen333.github.io/library/

## Building the Examples

The examples *should* compile and run on most platforms and with most IDEs.  I have provided `cmake` files to automate compilation.  On Windows, I prefer to use the MinGW compiler suite.

### Prerequisites:
- Git (https://git-scm.com/downloads)
- CMake 3.8.0 or higher (https://cmake.org/download/)
- On Windows, MinGW 5.0 or higher (https://sourceforge.net/projects/mingw-w64/)

### Download the repository:
Open a terminal (or command-prompt) and clone the repository
```
> git clone https://github.com/cpen333/library.git
> cd library
```
Otherwise, you can download the zip file, extract it somewhere, and navigate to that folder in the terminal.

### Compile the examples with CMake
Windows with MinGW:
```
> mkdir build && cd build
> cmake -G "MinGW Makefiles" ..
> mingw32-make
```
OSX/Linux:
```
> mkdir build && cd build
> cmake ..
> make
```
This should create a `bin` folder within the `build` directory that contains all the compiled examples.

### Generate projects for Visual Studio
On Windows, you can also generate Visual Studio solution and project files:
```
> mkdir build && cd build
> cmake -G "Visual Studio 15 2017 Win64" ..
```
(or if it fails, it should tell you which versions of Visual Studio you can generate for).  
You should then be able to go into any of the examples folders, and double-click on the desired "solution" file to open it.

To run one of the targets,
- Right-click on the desired project, and select `Debug > Start new instance`
- You may get a warning that the project is out of date, asking if you wish to build the project.  Click `Yes`.
- You may get a warning that there were "Deployment" errors, asking if you wish to continue.  Click `Yes`.

The project should then run.






