# lab8-nataliechen413

Welcome to lab8 for CSE 5462 Network Programming!

The goal of this semester-long project was to create and implement a protocol that allows a set of nodes to communicate with each other, even when there is no true end-to-end connectivity. Drones are set up across a 2D grid with row and column dimensions specified as input from the user. The initial locations of the drones are determined by a config file (named 'config.file' here). The protocol would allow drone A and drone C to communicate, even when they are out of range of one another, by using drone B (assuming drone B moves back and forth so that it is sometimes close enough to talk to A and sometimes close enough to talk to C). Drones also have the ability to move other drones by sending a move message with the appropriate protocol formatting as listed below.

Correct protocol formatting for regular messages includes:

- string of key:value pairs separated by a single space
- at most one key with the name 'msg', and the value has only one set of quotation marks to encase it
- exactly one key with the name 'toPort', and the value as the port of that the message is intended for
- exactly one key with the name 'version', and the value as '8'

Correct protocol formatting for move messages includes:

- string of key:value pairs separated by a single space
- exactly one key with the name 'move', and the value as the target location of the move
- exactly one key with the name 'toPort', and the value as the port of that the message is intended for
- exactly one key with the name 'version', and the value as '8'

How to Run:

1. Run 'make' in the terminal
2. Run 'drone8 <portnumber> <numberOfRows> <numberOfColumns>' in the terminal
3. As the programming is running, enter a message (with correct protocol formatting) into the console in order to send the message to other drones on the same IP address.

How to Run with Test Example:

1. In one terminal, run 'make'
2. In this first terminal, run 'drone8 50001 5 3'
3. In a second terminal, run 'drone8 50007 5 3'
4. In a third terminal, run 'drone8 50015 5 3'

5. In the first terminal, enter 'toPort:50007 move:7 version:8' into the command line

- Notice how the second terminal will display the move message from the key:value pairs

6. In the first terminal again, enter 'toPort:50015 msg:"hello 15" version:8' into the command line

- Notice how the third terminal will display the message from the key:value pairs
- Notice how the first terminal will display the corresponding ACK message from port 50015
