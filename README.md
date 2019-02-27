mutgos_server
=============
MUTGOS (Multi User Text Game Operating System) is an attempt to provide a more modern, flexible, extensible server for text-based gaming (MUCK, MUD, MOO, MUSH, MU*, etc).  The basic idea is to bring text-based gaming into the modern world, so more can discover, understand easily, create, and enjoy it.

Some of the features MUTGOS plans to offer (or already does) includes, in no particular order:
  * Abstraction in major components, to allow for switching out communication mechanisms, commandline interpreters, database backends, in-game softcode languages, security, etc.  In turn, this should lead to better maintainability, with code that's easier to fix or modify.
  * Better support for handling poor internet connections (cell, public wifi, etc).
  * Rich client support (via JSON, for web clients or yet-to-be-written client applications), to support features such as building areas graphically, a graphical debugger, mouseover descriptions, click-to-move, separate tabs for private messages and running programs, sidebars with details about the current room, simple UI elements for programs, etc.
  * An in-game language that is more comparable to modern languages and has modern features (AngelScript).
  * A security subsystem that has consistent rules while also allowing more flexibility in sharing administrative tasks, but without giving people or programs more privileges (called 'capabilities') than they need.
  * Multi-tenant support, to allow multiple worlds (sites) to reside in the same server instance.  This could allow a web-based system to get people started on creating a new site easily, without requiring them to set up and administer their own server.
  * Nearly every command programmed in softcode to make them easier to implement and modify, and to provide a new installation with plenty of features built-in, rather than having to hunt down and install programs elsewhere or code everything manually.
  * A 'global' site where said programs can reside, allowing them to be utilized in every site without making copies.
  * Federating different installations (or even different game server types) to allow sending messages, etc.
  * Better UTF8 and multi-language support.
  * Richer datatypes for object properties beyond the usual number and text string.
  * Multi-threaded to allow better handling of load.
  * A more extensive commandline interpreter to allow features such as redirection of input/output.
  * Version control, to allow backing out of a change to an object.
  * 'instances' of an object, to allow for something like a temporary dungeon instance.
  * Your idea here!
  
Currently, a working, but extremely basic (not complete enough to run an actual game) prototype exists and much of the framework has been laid out.  There are many unique challenges still ahead, but it's also an opportunity to do lots of learning and create something amazing!  Still reading and want to contribute?  Join us!


Building and Running The Prototype
==================================
Refer to the 'docs' directory for information on how to do this.  Currently this is best suited for semi-experienced C++ developers with Linux experience, but this will get much easier as time goes on.
