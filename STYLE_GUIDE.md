MUTGOS Style Guide
==================

This style guide is still very much a work in progress and will be updated as more people contribute and issues are identified.  For now, some basics will be covered.

The best way to see the code style is by example.  Access the 'prototype' release tag and look at the code to get a feel for the style.

In short, the basic idea of the style guide is to keep MUTGOS readable, maintainable, and reliable.  Text games can often have a long lifespan (some run the same server code for 20 years or more!); this is considerably different from most software.  While the code style for software of this size is important, it is not going to be exact; reasonable variations are fine and the main concern is to keep the code readable and to catch mistakes easier. 

Other than following good C++, OO, and separation of concerns practices (this can be very opinionated, but multiple approaches to the same problem are often correct and any can be used if they are consistent with the rest of MUTGOS), the code should be easy to look at and in theory look like only one or two people did all the work.  This is to let your eyes and mind focus on the code itself rather than how to read the code - it can make a bigger difference than you think!  'Easy to look at' can also be fairly opinionated and there are multiple correct answers, but try to follow the style laid out in existing files for consistancy.  If modifying a file, please keep the same formatting even if it differs from other files.  Being consistent within the file is more important than following the overall project style exactly.

Some specific style and best practices advice is below:
  * Creating files
    * Pick an existing source directory that contains functionality similar or related to yours.  If none match, create a new directory.
    * The naming convention is innernamespace_ClassName.h/.cpp .  Example:  if the inner namespace is 'comm' and the class is 'CommAccess', the header would be called 'comm_CommAccess.h'
  * Contents of files
    * The outer namespace is always 'mutgos'.  An inner namespace must be specified and is dependent upon the area you're working in.  Creating new inner namespaces is allowed.  There is no need to indent between the outer and inner namespace.
    * Namespaces are short and in lowercase, one word.
    * Class names are MixedCase.
    * Method names, class attributes, and variables use_underscores_like_this.
    * NEVER use the 'using' keyword unless there is a need when integrating with an outside library.  This cuts down on confusion as to which class you are referring to and where it is located.
    * Line length is about 80 characters.  You can go a little beyond this if it makes things more readable now and again.  The optional exception is class attributes (though maybe this should be reconsidered?) which can go beyond 80 characters, with the assumption an IDE of some sort is used to decode the comments as a pop-up.
    * Use spaces, not tabs.
    * Spacing should be at 4 spaces per tab, but this is flexible.
    * Everything should be thoroughly commented or documented in some way.  The code, attribute, or method may look obvious to you, but it may not be to everyone else.  This also makes for nice pop-up help in IDEs and other auto-generated documents.  Use JS and/or Doxygen style to maximize readability.
    * Separators between method implementations should be present to visually note when one method ends and another begins.
    * Use forward declarations whenever possible.
    * Use constants (in the anonymous namespace) when possible.
    * Brackets must be present for all 'if', 'for', 'while', etc statements to reduce error, even if there is only a single line to execute.
    * Brackets should be on a separate line from other code to make code stand out better.  If you absolutely, positively cannot abide by this for whatever reason, then you may put the opening bracket at the end of a line, providing the code contained within is indented.  The exception is one-liners inline in headers.
    * Methods (definition or use of) that take lots of arguments should have one argument per line, indented, to make it easier to keep track of inputs.
  * C++ version used
    * Currently, C++11 64bit is selected as the target for maximum compatibility
    * Code may be written to the C++98 64bit standard as well (the initial prototype was written to this standard due to how long it took to get finished).
    * Avoid using platform specific functionality (use BOOST whenever possible).  If it is unavoidable, put it in osinterface so it can be easily reused and modified to support additional platforms.
    * If you can do an implementation without using advanced features of C++, then do not use those features.  The goal is to keep the code readable and maintainable even by those who are not familiar with every bit of C++ functionality.  If it makes sense to use a feature because it will significantly speed up development, performance, etc, then by all means use it.
  * Pointers
    * Check for pointer validity in almost all methods.  Remember one bad pointer will crash the whole thing!
    * Document clearly when pointer ownership changes (header documentation, etc).
    * The code currently does not use smart pointers due to multi-threaded, container, and MUTGOS-specific concerns; at some point when the original author gets up to speed on the latest in smart pointers and an appropriate one is found, this may change.  You may use smart pointers within methods if the lifetime of the pointer is within that method only.
    * Remember to run valgrind to find memory errors!
  * MUTGOS is multithreaded
    * Remember, your code will be accessed by multiple threads at the same time, unless noted otherwise.  Use BOOST mutexes, etc to prevent data corruption.
    * This is a tricky area; don't hesitate to get help or ask for clarification.
    * In many cases, re-entrant methods return copies of data rather than the stored data itself, to prevent multiple threads from overwriting it.
  * Dependencies on other libraries
    * Minimal dependencies are desired to make this easy to maintain over a long period of time.
    * If you need/want to depend on a library not already depended on and that is not a common, stable system library (IE: most C++ implementations have it built in), it must be approved by the project admins beforehand.
  * Logging
    * Use the logging macro.  The goal of logging is to help diagnose problems without using a debugger, or as an aid even when using a debugger.  As long as most logging is at a debug level then there should be no performance impact.
    * NEVER log passwords, arguments to programs being executed, text sent to or generated by players, or the contents of Entities.  This is to preserve people's privacy at a basic level.  If the text you are logging could contain user-generated content, then do not log it!
  * Code cleanup
    * If you want to cleanup the formatting of existing code (no logic changes), please do it as a separate commit and pull request.  This makes it easier to review and merge.  Avoid providing a pull request containing extensive formatting changes AND logic changes; these will be rejected.
