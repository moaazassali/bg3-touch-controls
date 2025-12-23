# World Interaction Module

## Overview

This module provides an interface for hooking and intercepting the game’s World
Interaction System. It allows us to disable or modify some behaviors like hover
effects and clicks on world objects. We could also potentially use it to do our
own ray casting and then trigger custom interactions/input/action requests
based on that.

## How World Interaction Works
In order to understand why we hook the functions we do, we need to first
understand how the game updates the world interaction state and push updates to
it.

I started by looking at functions that read from the global cursor coordinates
address (the same thing that the `rawInputHandler` function acesses that is 
mentioned in the 'Move Internal Cursor' module). I found a function that seems
to do some 2D to 3D ray casting based on the cursor position and also seems to
run every frame. I call this function `worldInteractionUpdater`. Disabling this
function also disables all world interaction, so we're on the right track.

The function takes the ray cast result and populates 4 structs. It then uses
the same 'dispatcher' function 4 times. It passes a pointer to this dispatcher,
all of which are like `*(a1 + offset)`. The offsets used are: 472, 480, 488,
and 496. They seem to point to a vtable, and the first interesting function in
the vtable is what I assume to be the 'handle interaction' function. I haven't
rigorously verified that these functions are indeed the handlers for each of
the 4 dispatched calls, but it seems likely.

For instance, disabling the function from the vtable for the offset 472 call
disables all world item interactions (like NPCs and items). Also, disabling the
dispatch call that uses offset 472 also does the same. So, they seem to be
closely related. I call the handler function for the offset 472 dispatch call
`worldInteractionTargetResolver`.

The `worldInteractionTargetResolver` checks for all targets that match the ray
cast. It then picks the best target if there are multiple matches. It also does
some checks on the type of target and assigns the target ID at the
corresponding struct offset for that target type. The ID fields I found are:
- NPC
- NPC corpse
- Interactable item
- First candidate (maybe while cycling through targets))
- Winner target

I am not 100% confident, but they should be roughly correct.


## The Problem
When we tap on the screen, drag or do any touch interaction, we need to update
the internal cursor position. This, in turn, causes hover effects in the world
to trigger, which is distracting. Touch input doesn't have a concept of hover,
so we want to disable these effects.


## Naive Solution
We could move the internal cursor position when we want to disable hover
effects to some offscreen location, like (-10000, -10000).
This generally works but has some problems:
- It needs to be a really large offscreen position. I tested values like
  (-10, -10) and (-100, -100), but they still triggered hover effects.
- We move the cursor somewhere far like (-10000, -10000) while dragging for
  camera rotation or joystick movement. Then, we want to move the character by
  tapping somewhere on the world. However, since the cursor is far away, the
  game doesn't seem to register the tap unless we tap again. So, we end up
  needing to tap first to 'focus' the cursor back to the world, and then tap
  again to actually move the character.


## Better Solution
We hook `worldInteractionUpdater` and skip its execution by default. We enable
it only when we are doing some touch interaction that requires the world
interaction to be updated, like clicking on the world to move the character or
interacting with world objects.

However, this introduces an issue if we want to be able to use mouse input
simultaneously with touch input. This use case is highly unlikely, so we won't
support it for now. We could potentially fix it by detecting a mouse input
event and re-enabling the `worldInteractionUpdater`.