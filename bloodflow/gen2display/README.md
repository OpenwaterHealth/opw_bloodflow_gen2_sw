# Overview
This software is a QT project meant to run on the Openwater Bloodflow Gen2 system. It compiles to an executable that may be run on an x86 system with a TDA4 emulator or on the TDA4 itself in a console. In the console, this application connects to a C++ application over sockets and exchanges information about button presses, user input, and data feedback. 

## Features
The UI application gives the user the ability to:
- Start and stop scans for patients
- Check contact between the patient's forehead and the patient applied component of the device
- Check the quality of the data recorded
- Manage data recordings on the device's internal storage
- Transfer data recordings to a USB flash drive
- Install software updates delivered via flash drive

# User Documentation
User Guide:
https://docs.google.com/document/d/1KtrLJ_kjrIWFhRTz3GcrUFAJ7LChcWwhtGG9w_1jLhs/edit

Software Update Instructions:
https://docs.google.com/document/d/1q8FWDact6j7lssCdk108j4u4Z8-eMeLwroqNSnHM9Sg/edit

## Configuring QT Creator to Cross Compile for TDA4

1. Install QT Creator v5.12. Skip this if you already have the Reach VM running with it preinstalled.

2. Open QT creator and open the .pro file in this folder. You should be able to hit play and have things run.
- It will hang if not interfaced with by a mock openwater app so run tda_mock_sockets.py in the mock / folder to really get to a full interface.

3. Now it's time to make your system build for the TDA4. Download the two .sh files from the following link.

OpenwaterTeam/Software/Build-Tools/arago-2021.09-aarch64-linux-tisdk.sh
OpenwaterTeam/Software/Build-Tools/arago-2021.09-toolchain-2021.09.sh

https://drive.google.com/drive/folders/10CAqhpgBUYyCzu29gBcMZuqHv6r5uI6a?usp=sharing

4. Give them both run permissions:
chmod +x arago-2021.09-aarch64-linux-tisdk.sh
chmod +x arago-2021.09-toolchain-2021.09.sh

5. Run the first installer:
./arago-2021.09-aarch64-linux-tisdk.sh -D -y

6. Run the second installer:
./arago-2021.09-toolchain-2021.09.sh  -D
When it asks where to install, tell it:
/usr/local/arago-2021.09

7. Configure QT Creator to cross compile using the newly installed SDK
a) In QT, open "Manage Kits" from the sidebar of the Projects pane (screencap 1)
b) In the "Qt Versions" tab, hit "Add" and add the qmake from the SDK we just installed. It will be at:
/usr/local/arago-2021.09/sysroots/x86_64-arago-linux/usr/bin/qmake
c) In the "Compilers" tab, hit "Add" and add a GCC > C compiler. Name it "TDA4 GCC" and put in the compiler path:
/usr/local/arago-2021.09/sysroots/x86_64-arago-linux/usr/bin/aarch64-none-linux-gnu-gcc
d) In the "Compilers" tab, hit "Add" and add a GCC > C++ compiler. Name it "TDA4 G++" and put in the compiler path:
/usr/local/arago-2021.09/sysroots/x86_64-arago-linux/usr/bin/aarch64-none-linux-gnu-g++
e) In the "Kits" tab, click "Add" and fill in the details with the following:
(top, unnamed) TDA4
File system name - blank
Device - Generic Linux Device
         (go create a new device if you'd like to get remote deployment running over in the Devices pane)
Sysroot - /usr/local/arago-2021.09/sysroots/aarch64-linux
Compiler - C :  TDA4 GCC 
           C++ : TDA4 G++
Qt version: Qt 5.14.2 (System) (or whatever it came up as when you added it in 7b)
Defaults for the rest

f) Hit "Apply" and the warning sign next to your TDA4 device should disappear. Hit "Ok" to quit the window.
g) In the sidebar under Build and Run, your TDA4 configuration should appear and be grayed out. Press the plus buttons and the Build and Run configurations will populate. Now, when you hit build, a runnable Gen2GUI.bin should build.
h) If you would like to use the remote deploy configuration, make sure to add to the "Run Configuration" pane's "Command line arguments" textbox the following:
-platform linuxfb
and check the "Run in Terminal" checkbox to make sure that the display's log statements end up visible to you during debugging.



## Mocking the TDA4

There are 2 scripts here to 'mock' the TDA input/output that can be
used to test the display behavior: tda_listerner.py and tda_writer.py. 
Brief instructions are in tda_writer.py. In short, a TTY loopback must 
be set up (using 'socat', see tda_writer.py) then the 'listener' will
print the output of what the display sends to the TDA, and the 'writer'
sends json packets to the display, as if it were the TDA. The 'writer'
provides a list of predefined json packets that it can send. Those
are sent by typing the name of the packet into the writer (it's a
command-line based script). Typing 'h' or '?' into the writer will
show the list of packets that it can send.


