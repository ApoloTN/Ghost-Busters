Student Number: 101346405

Description

Overview
In this supernatural investigation game, you manage a team of ghost hunters exploring a mysterious haunted house. Your objective is to gather three distinct pieces of evidence from the ghost haunting the location. But beware - hunters may leave due to fear, equipment failure, or other supernatural encounters!

Game Features

- Hunter Management

Team Size:initiate up to 4 hunters

Dynamic Team: Hunters can leave the investigation for various reasons:

High fear levels

Boredom

substantial evidence

Ghost Investigation
Evidence Collection: Find exactly 3 pieces of evidence to win

Multiple Ghost Types: Various ghosts with random behavior and evidence requirements

Procedural Elements: Ghost location and evidence placement change each game

This program simulates a small building containing rooms and ghosts. Each ghost has a type, a unique ID,, and can be placed inside a specific room.

The project is written in C and organized across multiple source files. It demonstrates and features dynamic memory management, structured design, semaphores, threading, bits and bytes.

Instructions
• Instructions:
Step 1: Locate the file

- Open your terminal.
- Navigate to the folder containing the .tar file by typing: ls to look for the .tar file
- if it's in a folder, type: cd foldername to get inside it
- If you already see it when you type ls you're in the right place

Step 2: Untar the file
-In the proper directory
-In your terminal, type: tar -xvf final.tar
-type ls and now all the files that were inside the tar should appear in the directory

Step 3: Compilation
-In the terminal, once more type: gcc -g - o final main.c hunter.c ghost.c roomstack.c helpers.c -lpthread

Step 4: Checking for memory leaks
-Once more in your terminal type: valgrind --leak-check=full ./final
-This will run behind the scenes of the program and display useful memory debugging information once the program is terminated.

Step 4: Run the program

- in terminal type: ./final
  -Now you should be all up and running!

Sources

Developed individually by Daeshawn Henry

“Understanding and Implementing a Linked List in C and Java” – Jacob Sorber

“Finding Memory Errors with Valgrind” – Jacob Sorber

- "What is a semaphore? How do they work? (Example in C) " - Jacob Sorber
- "How to create and join threads in C (pthreads)" - Jacob Sorber

- " How to Implement a Stack in C (+ encapsulation)" - Jacob Sorber

-"how does a Mutex even work? (atoms in the computer??)
" - Low Level

-"Mutex vs Semaphore Explained | Concurrency in a Nutshell" - Nutshell

Additional questions asked to clarify course concepts – ChatGPT
