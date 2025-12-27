# Update Internal Cursor Module

## Overview

This module implements direct manipulation of the game's internal mouse cursor
state for touch input, bypassing SDL event processing to avoid race conditions
with the `isCursorOverUI` flag. Instead of sending `SDL_MouseMotionEvent`
events that would be processed asynchronously, it directly updates the game's
global mouse coordinates and performs immediate NoesisGui hit tests to
synchronously update UI hover state. This ensures that any touch handling logic
dependent on cursor position behaves correctly, eliminating the need for
artificial delays and other workarounds.

## How Mouse Movement Input is Processed

Why can't we just send an `SDL_MouseMotionEvent` directly to the game?

First, let's what the game does when it receives an `SDL_MouseMotionEvent`.

The entry point is a function I call `rawInputHandler`, which calls
`SDL_PollEvent` repeatedly to get events from the SDL event queue. When it
receives an `SDL_MouseMotionEvent`, it updates a global state variable holding
two float values called `x` and `y` window (not screen) coordinates of the
mouse.

This global mouse coordinate struct is used in various places in the game.
For example, when a left click is received, instead of checking the coordinates
of the click event directly, the game checks the global mouse coordinates.
There is only one source of truth for mouse coordinates: this global state
variable, which is updated only by `SDL_MouseMotionEvent`.

After updating the global mouse coordinates, the game creates some internal
input event. This then gets handled by a function I call `processInput`.
This then finds the `ActionRequest`(s) corresponding to the mouse motion event,
and adds them to an internal action queue using a function I call 
`pushToActionRequestsQueue`. Specifically, if `SDL_MouseMotionEvent` is sent
with just `x` and `y` without `xrel` and `yrel`, the game only creates a
`UIMoveMouse` action request.

Somewhere down the line, this triggers a call to NoesisGui `View.MouseMove`
function that acts on a (or the?) global? view. This function then does a hit
test to find the UI element under the mouse coordinates (passed to it as `int`
parameters). If it finds a UI element, then it activates whatever mouse-over
effects that element has and also triggers the registered callback. The game
registers a function I call `updateCursorOverUIFlagCallback` which is a small
wrapper around a function I call `updateCursorOverUIFlag`.

Finally, `updateCursorOverUIFlag` takes a single parameter: some manager struct and sets
a boolean field I call `isCursorOverUI` to true or false depending on whether
the hit test found a UI element or not.

## The Problem

If we recognize a tap and send an `SDL_MouseMotionEvent` followed by an
`SDL_MouseButtonEvent`, the game will process these events in the order they
were received. This means that the `SDL_MouseMotionEvent` will update the
global mouse coordinates, which is what we want. `SDL_MouseButtonEvent` will
then be registered at the updated mouse coordinates.

Everything seems fine so far...

The issue arises when we load into the game world where we have both UI and
non-UI elements (i.e. the game world). If we assume an initial state where the
cursor is over the world (i.e. `isCursorOverUI` is false), and we tap on a UI
element, the following happens:
1. `SDL_MouseMotionEvent` is processed first, updating the global mouse
   coordinates to be over the UI element. The game then pushes a `UIMoveMouse`
   action request to the action queue.
2. `SDL_MouseButtonEvent` is processed next, which checks the global mouse
   coordinates (which are now over the UI element).
3. However, `isCursorOverUI` is still false, because the game has not yet
   processed the `UIMoveMouse` action request that would have updated it to
   true.
4. The game then incorrectly interprets the mouse button event as a click on the
   game world instead of the UI element and the character moves to that location
   that is behind the UI element.
5. So, to actually click on a UI element, one would have to first click on it
   once to update `isCursorOverUI` to true, and then click on it again to
   actually activate the UI element.
6. This issue also occurs in reverse when clicking from a UI element to the
   game world. The first click on the game world now will do nothing and we
   have to click again to actually move the character.

## Naive Approach

Add a delay between sending `SDL_MouseMotionEvent` and `SDL_MouseButtonEvent`
to give the game time to process the mouse motion event and update
`isCursorOverUI`.

This is the solution I went with initially and it works "well enough" in
practice. However, the question of the delay duration is not straightforward.
For example, if the game is running at a low framerate, a longer delay may be
needed. Conversely, if the game is running at a high framerate, a delay may
suffice.

Also, what about the touch processing rate? If the touch processing rate is
higher than the delay and since we move the cursor at each touch processing
event, we may end up sending multiple `SDL_MouseMotionEvent` events before our
queued tasks like the `SDL_MouseButtonEvent` get processed. So, we may end up
with the same issue again where `isCursorOverUI` changed back to the original
value before the `SDL_MouseButtonEvent` is processed.

Even then, it's just so inelegant and increases the complexity with all the
functions that need to be queued and managed. It also increases the latency
of the touch input and makes it feel less responsive.

## Proper Solution

The ideal solution is to directly update `isCursorOverUI` after moving the\
cursor. Additionally, we would disable hover effects because touch input does
not have hover states.

This is possible by not sending an `SDL_MouseMotionEvent` to the game, and
instead directly updating the global mouse coordinates.

We still need the `isCursorOverUI` update. We could call the NoesisGui
`View.MouseMove` function directly after updating the global mouse but this
activates hover effects, which we do not want.

Instead, we can should call the `updateCursorOverUIFlagCallback` function.
However, to determine whether we are over a UI element or not, we need to
perform a hit test using the NoesisGui hit test function.
`updateCursorOverUIFlagCallback` takes some game manager struct and the UI
element from the hit test if successful.

This way, the `isCursorOverUI` flag is updated immediately after moving the
cursor, and we can safely execute any sequence of events that depend on the
cursor position.

### Caveat 1: Failing the hit test doesn't update `isCursorOverUI` flag
`updateCursorOverUIFlagCallback` doesn't update `isCursorOverUI` to false if
the hit test fails. Instead, it sets some flag which (I think) indicates that
the last hit test failed and that the game should re-evaluate `isCursorOverUI`
the next frame. So, if we fail a hit test, we also need to manually call a
function I call `updateCursorOverUIFlag` that  `updateCursorOverUIFlagCallback`
calls so we can ensure the flag is updated immediately in this case as well.

This is all of what's done in `updateCursorOverUIFlagCallback`:
```c
void __fastcall updateCursorOverUIFlagCallback(__int64 a1, __int64 a2) {
  *(a1 + 136) = a2;
  if ( a2 )
    updateCursorOverUIFlag(a1);
  else
    *(a1 + 449) |= 2u;
}
```
So, we don't need to call the callback; we can just directly call
`updateCursorOverUIFlag` and set the UI element field ourselves. We don't need
to set the 're-evaluate' flag either because we are doing the re-evaluation.

### Caveat 2: A cache of cursor coordinates is used if hit test fails
This is what `updateCursorOverUIFlag` does if called after the hit test fails:
```c
else {
    v11 = *(a1 + 0x190);
    v7 = (*(**(a1 + 112) + 56LL))(*(a1 + 112));
    *(a1 + 384) = sub_7FF79576E1D0(v7, &v11) != 0;
}
```
`*(a1 + 0x190)` is a cached copy of the last known cursor coordinates from the
last frame. So, we also have to update this cache when we move the cursor.
Otherwise, if the hit test fails, the game will use stale coordinates that were
over a UI element and incorrectly set `isCursorOverUI` to true.


### Caveat 3: World interaction coordinates cache for clicks
There is a function I call `worldInteractionUpdater` that runs every frame and
accesses the global mouse coordinates to do ray casting in the game world for
world interaction (like hover effects and clicks on world objects). Refer to
the 'World Interaction' module for more details.

What is relevant here is that `worldInteractionUpdater` seems to use a cached
copy of the 2D->3D ray cast from the last frame to handle world object clicks.
So, if we move the internal cursor and then immediately send a click event that
is on a world object (and not the ground), it hovers over the item but then
moves the character to the old cursor position. I still don't fully understand
the issue though. Maybe I am making things up here.

If we need to synchronously update the world interaction state after moving the
cursor, then we could call `worldInteractionUpdater` directly. However, it
looks like it relies on the result from an async consumer that it dispatches
the ray cast result to. But calling it directly would at least ensure that any
action that happens next will happen after the world interaction state is
updated since the consumer would have sent its result already (maybe?). At
least doing so seems to work so far in ensuring the just the next action is
correctly handled.

The altnerative is to this direct call is to instead queue whatever any action
that depends on the world interaction state (like some world clicks) to be
processed in the frame after the new cursor position was processed in.