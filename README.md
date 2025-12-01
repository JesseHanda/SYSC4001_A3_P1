This part of the assignment simulates a basic interrupt-driven operating system model using a custom scheduler. The system loads processes from an input file, assigns memory partitions, updates PCB states, and handles different types of “interrupts” such as time slice expiration, I/O requests, and process completion.

The goal of Part 1 is to demonstrate how a simple OS reacts to events and moves processes between states like NEW, READY, RUNNING, WAITING, and TERMINATED.

Files
interrupts_*.cpp      // Part 1 logic (RR, EP, EP+RR versions)
interrupts.hpp        // Shared data structures: PCB, memory partitions, enums, helper methods
a3_input.txt          // Input job list used by the simulator


Your specific submitted files (RR / EP / EP+RR versions) follow the required template structure and include:

Memory assignment & deallocation

PCB state transitions

Ready queue handling

Event (interrupt) processing

Output of full execution trace

How to Compile

Each file is standalone. Example:

g++ -std=c++17 interrupts_RR.cpp -o p1_rr


For EP or EP+RR:

g++ -std=c++17 interrupts_EP.cpp -o p1_ep
g++ -std=c++17 interrupts_EP_RR.cpp -o p1_eprr

How to Run

Each program expects the input file name:

./p1_rr a3_input.txt
./p1_ep a3_input.txt
./p1_eprr a3_input.txt


The program prints a full detailed log that shows:

When processes enter READY

When they begin RUNNING

When they request I/O

When time slices end

When processes terminate

Memory allocation changes

Final PCB tables and event traces are written to output files as specified.

Features Implemented

- Memory partition assignment
- Process arrival handling
- Round Robin scheduling (Part 1 RR file)
- Event Processing (EP) scheduling (Part 1 EP file)
- Combined EP + RR version
- Proper state transitions using OS-style PCB updates
- All helper functions from interrupts.hpp used as required

Notes

The simulator follows the exact structure outlined in the assignment template.

Execution traces match the format expected by the TA (header, rows, footer).

All PCB updates are synchronized through the helper functions in interrupts.hpp.
