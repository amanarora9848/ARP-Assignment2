# ARP-Assignment2
Base repository for the **second ARP assignment**.
The project provides you with a base infrastructure for the implementation of the simulated vision system through shared memory, according to the requirements specified in the PDF file of the assignment.

The two processes involved in the simulation of the vision system, namely **processA** and **processB**, are implemented as simple *ncurses windows*. The development of the inter-process communication pipeline, that is the shared memory, is left to you.

As for the first assignment, you also find a **master** process already prepared for you, responsible of spawning the entire simulation.

Additionally, I have prepared a simple program called **circle.c**, which shows you the basic functionalities of the *libbitmap* library. Please, note that the **circle.c** process must not appear in your final project. It is simply meant to be a guide for you on how to use the bitmap library, therefore you will need to properly use portions of that code in **processA** and **processB** in order to develop your solution.

All credits to @SimoneMacci0 for development of the base repository for the assignment, at [SimoneMacci0/ARP-Assignment2](https://github.com/SimoneMacci0/ARP-Assignment2).

## *libbitmap* installation and usage
To work with the bitmap library, you need to follow these steps:
1. Download the source code from [this GitHub repo](https://github.com/draekko/libbitmap.git) in your file system.
2. Navigate to the root directory of the downloaded repo and run the configuration through command ```./configure```. Configuration might take a while.  While running, it prints some messages telling which features it is checking for.
3. Type ```make``` to compile the package.
4. Run ```make install``` to install the programs and any data files and documentation.
5. Upon completing the installation, check that the files have been properly installed by navigating to ```/usr/local/lib```, where you should find the ```libbmp.so``` shared library ready for use.
6. In order to properly compile programs which use the *libbitmap* library, you first need to notify the **linker** about the location of the shared library. To do that, you can simply add the following line at the end of your ```.bashrc``` file:      
```export LD_LIBRARY_PATH="/usr/local/lib:$LD_LIBRARY_PATH"```
### Using *libbitmap* in your code
Now that you have properly installed the library in your system, it's time to use it in your programs:
1. Include the library in your programs via ```#include <bmpfile.h>```. If you want to check the content of ```bmpfile.h``` to glimpse the functionalities of the library, navigate to ```/usr/local/include```, where the header file should be located.
2. Compile programs which use the *libbitmap* library by linking the shared library with the ```-lbmp``` command.     
Example for compiling **circle.c**: ```gcc src/circle.c -lbmp -lm -o bin/circle``` 

## Compiling and running **processA** and **processB**
The two processes are implemented as UIs through *ncurses* library, therefore you need to compile their source files by linking the shared library via ```-lncurses```. As for the first assignment, exploit the resize event of the windows to get out of situations in which the graphical elements do not properly spawn.

## Executing **circle.c**
This is a simple example of a program which uses the *libbitmap* library. It generates a 100x100 colored `.bmp` file with user-defined name, depicting a blue circle of given radius. When you execute it, pass the two arguments (file name and radius value) along. Execution example: ```./bin/circle out/test.bmp 20```.

## A note about the consoles:

The two consoles: Inspection and Command console will be controlling the movement of the hoist, as instructed. All the process logs, user inputs, etc., are written to the log files inside `logs/`. 

Apart from the already present **Stop** `S` and **Reset** `R` buttons, we have implemented an extra **exit** button `X`, which can be used to terminate all the processes and close the consoles.

![Command Console](images/command_console.png)
![Inspection Console](images/inspection_console.png)

## Branch Info:

- **main**: In this version, the `master` process also performs `watchdog` duties. Watchdog uses the log files information (mtime) to keep track of the inactivity of the processes
- **exp**: Experimental branch, where changes are made before merging to other branches. Right now it should be the same as **version2**.
- **version2**: In this version, `master` only spawns the processes and the watchdog is a separate process. The watchdog receives periodically "alive" messages from other processes and keeps track of their inactivity using the lack of such messages. 


## Programming Paradigms

- All Inter Process Communications are done using named pipes (FIFOs).
- Most processes also use the `select` function to monitor `pipes` and read only when data is avaialable in them.
- We use the `sigaction` POSIX function to perform all signal handling for all the pre-existing and user defined signals.


## Brief Explanations about the processes

- Master: 
	- Spawns the `command` console.
	- Spawns the motor processes `motorx` and `motorz`.
	- Spawns the `world` process.
	- Spawns the `inspection` console, which takes the pid's of the motor processes.
	- Performs `watchdog` duties: keeps track of inactivity time for every process looking at the `mtime` property of all log files.
	- Waits for the termination of the 'konsoles' and terminates the programs.

- Command Console:
	- Sends the user input for controlling motors to both motors, `motorx` and `motorz`.
	- Logs the status messages (user inputs) and any errors in the log file.

- Inspection Console:
	- Keeps track of the 3 buttons, `Stop`, `Reset` and `X` (EXIT) buttons.
	- Sends the coresponding signal to both motors and cmd.
	- Logs the button pressed (user input) and any errors in the log file.

- MotorX and MotorZ:
	- Set (or Resets) the desired velocities of the hoist (increasing or decreasing by a set buffer) received from `cmd` pipe.
	- Send the desired `x` and `z` positions to the world process using fifos.
	- logs the desired position and any errors in the log file.

- World:
	- Receives the `x` and `z` position of the hoist (from the motors)
	- Generates (simulates) a random error within a defined range (5%).
	- Sends generated positions to the inspection console.
	- Logs the received position and the generated one, as well as any errors, in the log file.


## Known Issues:

- After pressing the exit button (in the inspection console) "broken pipe" errors are produced in `motorx`, `motorz` and `world` which are actually recorded in their corresponding log files. This is most likely caused due to the long time between the termination of the consoles and these other processes, giving them enough time to try to read from closed pipes. This error is not present when processes are terminated due to inactivity for instance. These errors are handled through the handling of the SIGPIPE signal produced.
