Security High Level Overview and Concepts
-----------------------------------------

Entity Security
---------------

Currently, Entities have security that is organized like this, similar to UNIX permissions:
  * Admins
  * Group / List
  * Everyone else / Other

Admins have access to everything on the Entity as if the owner, including deleting it.  Group and Everyone else can have nearly as much access, but it is broken down into several flags for each (again, similar to UNIX):
  * Read
  * Write
  * Base

Read allows viewing any attribute of the Entity (except properties; only listing property applications is allowed).  Write allows updating any attribute of the Entity (except properties, unless they are creating a new property application).  Base allows read-only access to only certain (basic) attributes, such as 'name', 'owner', 'entity id', etc.  The full list is in security::GetEntityFieldChecker.

If someone only has 'basic' access to the Entity, generally they must also be in the same room (locality) as the Entity they wish to query, otherwise access is denied.  Read and Write permissions have no such limitation.


Property Security
-----------------

Properties are governed by their own security; only 'base' access to the Entity is required to read them.  At the 'root' level, properties are stored in groups called 'applications'.  An application has a name, owner (which may be different from the Entity owner), and security settings, which represent the security for all properties underneath it.  Similar to Entities, each application has an Admin, Group, and Everyone level of security.  For Group and Everyone, the only two flags allowed are Read and Write.

Because these applications may store sensitive game data (for instance, the amount of a currency owned by a Player), an application can be owned by something or someone other than the Entity owner, thus blocking the Entity from reading or otherwise controlling its own property.  This also allows extra flexbility: A 'look' program can run with no special privileges if the description is stored in a property application that everyone has read access to.


Capabilities
------------

For privileged operations that don't fall into the security listed above, Capabilities exist.  A Capability is a specially named Group (only one Capability will have a given name) whose members are allowed to perform functions specified by that capability.  For instance, the ability to create or delete Entities is controlled by a 'Builder' capability.  Adding a Player or Program to that capability will allow it to create or delete objects, otherwise it will be denied from doing so.

Capabilities are created during site initialization and cannot be created or deleted, as they are specific to the MUTGOS C++ code.  They can of course be modified, to add or remove Entities to the group list.

Refer to the security::OperationsCapabilities header file for a list of operations and their associated Capability.


Modular Security
----------------

The security module is organized by operation (such as 'create Entity').  Each operation is then associated with one or more 'security checkers'.  When a primitive is about to perform a privileged function, it calls the security module with the associated operation to ask if it can proceed.  The security module then looks up which security checkers are related to the operation.  If all checkers indicate it is OK to proceed, then it returns success, else failure (security violation).
  
The security checkers each check a particular aspect of the security required to do an operation, in order to simplify and make security more consistant, easier to understand, and easier to validate.  For instance, there is a checker to see if the user is a site admin.  If so, it indicates no other checks are required.  There is also a checker that confirms the user isn't trying to access another site (other than #1).
