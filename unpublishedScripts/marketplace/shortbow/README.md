# Shortbow

Shortbow is a wave-based archery game.

## Notes

There are several design decisions that are based on certain characteristics of the High Fidelity platform,
and in particular, [server entity scripts](https://wiki.highfidelity.com/wiki/Creating_Entity_Server_Scripts),
which are touched on below.
It is recommended that you understand the basics of client entity scripts and server entity scripts (and their
differences) if you plan on digging into the shortbow code.

 * Client entity scripts end in `ClientEntity.js` and server entity scripts end in `ServerEntity.js`.
 * Server entity scripts are not guaranteed to have access to *other* entities, and should not rely on it.
   * You should not rely on using `Entities.getEntityProperties` to access the properties of entities
     other than the entity that the server entity script is running on. This also applies to other
     functions like `Entities.findEntities`. This means that something like the `ShortGameManager` (described below)
     will not know the entity IDs of entities like the start button or scoreboard text entities, so it
     has to find ways to work around that limitation.
   * You can, however, use `Entities.editEntity` to edit other entities.
   * *NOTE*: It is likely that this will change in the future, and server entity scripts will be able to
     query the existence of other entities in some way. This will obviate the need for some of the workarounds
     used in shortbow.
 * The Entity Script Server (where server entity scripts) does not run a physics simulation
   * Server entity scripts do not generate collision events like would be used with
     `Script.addEventHandler(entityID, "collisionWithEntity", collideHandler)`.
     * High Fidelity distributes its physics simulation to clients, so collisions need to be handled
       there. In the case of enemies in shortbow, for instance, the collisions are handled in the
       client entity script `enemeyClientEntity.js`.
   * If no client is present to run the physics simulation, entities will not physically interact with other
     entities.
   * But, entities will still apply their basic physical properties.

## Shortbow Game Manager

This section describes both `shortbowServerEntity.js` and `shortbowGameManager.js`. The `shortbowServerEntity.js` script
exists on the main arena model entity, and is the parent of all of the other parts of the game, other than the
enemies and bows that are spawned during gameplay.

The `shortbowServerEntity.js` script is a server entity script that runs the shortbow game. The actual logic for
the game is stored inside `shortbowGameManager.js`, in the `ShortbowGameManager` prototype.

## Enemy Scripts

These scripts exist on each enemy that is spawned.

### enemyClientEntity.js

This script handles collision events on the client. There are two collisions events that it is interested in:

 1. Collisions with arrows
   1. Arrow entities have "projectile" in their name
   1. Any other entity with "projectile" in its name could be used to destroy the enemy
 1. Collisions with the gate that the enemies roll towards
   1. The gate entity has "GateCollider" in its name

### enemyServerEntity.js

This script acts as a fail-safe to work around some of the limitations that are mentioned under [Notes](#notes).
In particular, if no client is running the physics simulation of an enemy entity, it may fall through the floor
of the arena or never have a collision event generated against the gate. A server entity script also
cannot access the properties of another entity, so it can't know if the entity has moved far away from
the arena.

To handle this, this script is used to periodically send heartbeats to the [ShortbowGameManager](#shortbow-game-manager)
that runs the game. The heartbeats include the position of the enemy. If the script that received the
heartbeats hasn't heard from `enemyServerEntity.js` in too long, or the entity has moved too far away, it
will remove it from it's local list of enemies that still need to be destroyed. This ensure that the game
doesn't get into a "hung" state where it's waiting for an enemy that will never escape through the gate or be
destroyed.

## Start Button

These scripts exist on the start button.

### startGameButtonClientEntity.js

This script sends a message to the [ShortbowGameManager](#shortbow-game-manager) to request that the game be started.

### startGameButtonServerEntity.js

When the shortbow game starts the start button is hidden, and when the shortbow game ends it is shown again.
As described in the [Notes](#notes), server entity scripts cannot access other entities, including their IDs.
Because of this, the [ShortbowGameManager](#shortbow-game-manager) has no way of knowing the id of the start button,
and thus being able to use `Entities.editEntity` to set its `visible` property to `true` or `false`. One way around
this, and is what is used here, is to use `Messages` on a common channel to send messages to a server entity script
on the start button to request that it be shown or hidden.

## Display (Scoreboard)

This script exists on each text entity that scoreboard is composed of. The scoreboard area is composed of a text entity for each of the following: Score, High Score, Wave, Lives.

### displayServerEntity.js

The same problem that exists for [startGameButtonServerEntity.js](#startgamebuttonserverentityjs) exists for
the text entities on the scoreboard. This script works around the problem in the same way, but instead of
receiving a message that tells it whether to hide or show itself, it receives a message that contains the
text to update the text entity with. For intance, the "lives" display entity will receive a message each
time a life is lost with the current number of lives.

## How is the "common channel" determined that is used in some of the client and server entity scripts?

Imagine that there are two instances of shortbow next to each. If the channel being used is always the same,
and not differentiated in some way between the two instances, a "start game" message sent from [startGameButtonClientEntity.js](#startgamebuttoncliententityjs)
on one game will be received and handled by both games, causing both of them to start. We need a way to create
a channel that is unique to the scripts that are running for a particular instance of shortbow.

All of the entities in shortbow, besides the enemy entities, are children of the main arena entity that
runs the game. All of the scripts on these entities can access their parentID, so they can use
a prefix plus the parentID to generate a unique channel for that instance of shortbow.

