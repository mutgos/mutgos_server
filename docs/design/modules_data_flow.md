This document explains, at a high level, all the modules of AngelScript and the data/program flow of using a command like 'say'.

Most modules have an 'Access' type singleton class in them.  When present, those are to be considered the entry point by any classes outside of the module, when those classes want to access services the module provides.


Modules (alphabetical order)
----------------------------

**angelscriptinterface**: Contains all code that interfaces with AngelScript, including optional addon components from AngelScript itself.

**channels**: Contains all code for Channels, which are an abstraction layer for transferring unidirectional streams of data between two different components.  For an example, sending input from the user (via a socket or websocket) to the command interpreter.

**clientmessages**: Wrappers for JSON messages that can be sent to and from the websocket client.  These would typically be sent via a ClientDataChannel.

**comminterface**: Common code and interfaces for all I/O components that interface with clients (socket, websocket, etc).  It handles common functions such as keeping track of sent data, authentication, and the main communications thread.

**concurrency**: A collection of classes used to abstract write and read locks (for thread safety).  Classes can implement interfaces located here, and use 'token' classes here to process locks.

**dbdump**: Common code that database dump file readers can use to convert a dump file into the native database format.  It also includes a sample file parser that can parse the 'MUTGOS Dump File Format'.

**dbinterface**: Common code that other modules use to access the database, and also performs overall database management (such as the background update/commit thread).  No actual interface code to a database is provided here, however it will use an implementation when provided.

**dbtypes**: All in-game types (Entity, Room, etc) and related classes are in here.

**events**: The event subsystem and event types are located here.  When something happens in-game (player connected, use of the 'say' command, moving between rooms, creating a Thing, etc), an event is typically triggered.  Other processes can subscribe to events by giving criteria, and be called back or notified when the criteria is satisfied.

**exe**: The executable 'main()' files are located here.  Currently it contains the server itself, dump file reader, and test drivers.

**executor**: The process management subsystem is located here.  Other modules can use this to launch an in-game process, manage it, query running processes, etc.

**logging**: The system-wide logger is located here.

**osinterface**: All code that interfaces directly with the operating system (or is platform specific) will be in here whenever possible.  In the future, should MUTGOS be ported to another platform, most of the changes will (in theory) be made here.

**primitives**: Common operations that softcode (and even C++ code, as desired) can use to perform functions within the game (for instance, using an Exit, moving a Thing, broadcasting text in a room, etc).  The operations are fully security checked and designed for reuse.  Components such as angelscriptinterface are simply wrappers that call operations within this module.  The intention is to allow multiple softcode interpreters, but without having to reimplement the MUTGOS-specific operations each time.

**security**: The security subsystem.  Used primarily by the primitives, this essentially tells the caller if a specific function can be performed or not, based on the user's security Context and what they are trying to do.  The subsystem is modular, with smaller security components being layered on top of each other.

**socketcomm**: Handles communication between plain and encrypted sockets and the comminterface.  Essentially abstracts away communicating with incoming sockets.

**softcode**: Provides an interface that softcode interpreters must implement, and provides a way for other modules to access and run softcode without caring what interpreter is needed.

**sqliteinterface**: Implementation of working with sqlite as a database backend.  Used by dbinterface.

**text**: All text processing and related code is here.  This includes 'ExternalText', which is a normalized, structured way of representing formatted text to and from the comminterface.  Parsers to convert ExternalText to plain ascii (with markup code) is also provided.  Conversion to ANSI colors and UTF8 processing code is here as well.

**useragent**: This is the 'agent' which is launched when a Player logs in.  It provides a command interpreter and subscribes to events so the Player can 'hear' what's going on in the room, get private messages, etc.

**utilities**: Misc classes that don't fall anywhere else, such as an interface to a third party JSON parser and global configuration.

**websocketcomm**: Handles communication between a websocket and the comminterface.  Essentially abstracts away communicating with incoming websockets.



High level data flow from user typing 'say Hello!' to someone else getting the message
--------------------------------------------------------------------------------------

Executing 'say hello' (not all modules shown, just the major ones):
  * socketcomm -> comminterface -> useragent -> softcode -> angelscriptinterface -> primitives -> events


Receiving 'Hello':
  * events -> useragent -> comminterface -> socketcomm


#### Narrative

At the user's client, they type 'say Hello!'.  The bytes travel along a socket, which get picked up by socketcomm.  From there, it's converted into ExternalText.  comminterface will soon query socketcomm and pick up pending data, which would include the ExternalText.  From there, the ExternalText travels along a Channel where the data is sent as a message to the useragent (via executor).

With the message pending, useragent is then given CPU time (process executes) and will process the message.  Using primitives, security, and dbtypes, it will locate the 'say' command.  Then, utilizing softcode and angelscriptinterface, an AngelScript interpreter process will be launched and managed by executor.  The process will run, and in the course of execution send an EmitEvent to events for processing.

On the receiving end, another Player's useragent is subscribed to EmitEvents for the same room.  The events subsystem will match that subscription and send the event to the useragent, via executor.  The useragent will wake up, process the event, and send the contents of the EmitEvent to the user's socket, by sending the ExternalText to comminterface via a Channel.  From there, comminterface sends the text to the user's socket via socketcomm.
