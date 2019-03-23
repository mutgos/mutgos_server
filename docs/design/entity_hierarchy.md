Quick cheat sheet for the Entity hierarchy and what each type is for.  Note not all functionality has been implemented yet.

* Entity (abstract)
  * Group
    * Capability
  * PropertyEntity (abstract)
    * ContainerPropertyEntity (abstract)
      * Region
      * Room
      * Player
        * Guest
      * Thing
        * Puppet
        * Vehicle
    * Program
    * ActionEntity (abstract)
      * Exit
      * Command


**Entity**:  The root of all database types, this abstract type has attributes and functions common to all Entities, such as name and ID.  It also handles features such as read/write locks, keeping track of which attributes changed (and calling the database listener), and maintaining a list of references (who refers to this Entity).

**Group**:  Represents a list of Entities.  This is used primarily for the security system.  As an example, you can create a Group and use it to specify a list of admins for one or more rooms.  These can be user created in-game.

**Capability**:  Created during a site's creation, these can never be created or deleted otherwise.  They work just like groups, except only one Capability can have a particular name.  Each Capability represents some sort of privileged function; anyone in the Capability will be able to do that function.

**PropertyEntity**:  An abstract type of Entity that can have properties on it.

**ContainerPropertyEntity**:  An abstract type of Entity that can have properties and contain other ContainerPropertyEntities (and ActionEntities).  'Contains' means 'holds' or 'has'.  An example is a room containing the Entities present in it, or a Player containing items in their inventory.  The Entity to be contained has a field such as 'contained_by'; when this is set with an ID, the ContainerPropertyEntity it is referring to will add a reference to it.  Therefore, to see what Entities are being contained by a ContainerPropertyEntity, simply look at the references (on the container) for the 'contained_by' and 'action_contained_by' fields.

**Region**:  A region can only contain other rooms, actions, and regions.  This is used to provide common properties and commands for one or more rooms.  Depending on future design choices, it could also be used to designate who has admin or special privileges over the rooms and regions underneath.

**Room**:  A standard room, like in any text game.  It generally contains Players, Things, and ActionEntities.

**Player**:  Represents a user that can log in to the game via an external interface (socket, websocket, etc).

**Guest**:  Represents a Player with severely limited permissions, typically used by an unauthenticated user who wants to look around first before requesting a Player.

**Thing**:  A standard object in the game (a box, sword, monster, etc).  Things can contain other Entities, including other Things.

**Puppet**:  A Thing that can be directly controlled by a Player.  Puppets can listen to what's going on around them and forward events to their player.  In short, this allows the creation of controllable characters on the fly by the player.

**Vehicle**:  A Thing that has an 'interior' (another Room), can listen to events around it, and forward those events to a specific room.  Vehicles can also be controlled via an action so they can move around and emit events.

**Program**:  Represents softcode, so that Commands and other Entities can link to it and execute the softcode.

**ActionEntity**:  An abstract type with attributes and behavior common to all actions, such as success and fail messages, locks, etc.

**Exit**:  Represents a linkage (transition) from one place to another.  By activating the Exit, you are moved from the room you are currently in to the destination.

**Command**:  Represents a linkage to a Program.  By activating the Command, the program is ran with the provided argument(s).
