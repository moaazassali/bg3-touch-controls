# README



## Dump of useful information
### Camera Rotation
`CameraObject.rotationSpeed` is by default set to 90. 
Effect not tested.

`CameraObjection.angle` is the angle of the camera in degrees. 
It's range is (-360,360). 
If you rotate beyond -360, it resets to 0 and continutes going towards -360.
If you rotate beyond 360, it resets to 0 and continues going towards 360.
The extra [0, -360] seems redundant/strange.

`CameraObject.cameraRotation` is a vector3 where x corresponds to some scaled value of `angle`.
It's range seems to approximately (-0.805, 0.805).
If you rotate beyond the limits, it doesn't reset though.
Going beyond -0.805, for example, makes the value increase towards 0.
Going beyond 0.805 makes the value decrease towards 0.
So, it's like a circular range.

The y and z values of `cameraRotation` are not affected by the game's definition of "rotation"
(i.e. using Q/E on keyboard or the x component of right stick movement on controller)

`CameraObject.currentAngleDelta` is the speed of rotation currently.
Any non-zero value makes the camera move.
When pressing Q/E on keyboard, it gets set to 1 or -1 as long as the key is held down.
It then resets back to 0.

### Camera Zoom
`CameraObject.zoomDelta` alternates between 100 and -100. 
-100 when zooming in and 100 when zooming out.
When not zooming, it is 0.

`CameraObject.desiredZoom` and `CameraObject.currentZoom_160` seem to always be equal and correspond to the desired target zoom.

`CameraObject.currentZoomA` and `CameraObject.currentZoomB` seem to always be equal and correspond to the actual current zoom of the camera.
Both of these values eventually reach `desiredZoom`.

`CameraObject.prevZoom_15C` seems to lag behind the actual current zoom (`currentZoomA` and `currentZoomA`).
It's final value when the camera stops zooming is typically close to desired/current zoom, but not exactly equal.

The range for the zoom values are [3.5, 12]

### Camera Panning
`CameraObject.horizontalPanDelta` is the speed of horizontal panning.
`CameraObject.verticalPanDelta` is the speed of vertical panning.
Both values are 0 when not panning.
When panning with keyboard using arrow keys or WASD, the values are either 15 or -15 (default game settings).
Chaning either value when the camera is not in "exploration mode" has no effect.
The camera will remain centered on the player.


## Bugs to investigate
Crash when doing a lot of touch interactions while the game is launching?
Maybe this is because of the console?

Mouse zoom stops working after using touch to zoom?

I am noticing a small amount of inertia when I stop panning for camera rotation.
The issue might be because we are using the last 5 frames to calculate velocity,
which might include some high delta values from the oldest one or two frames.
Maybe we sould use a weighted average that gives more weight to the most recent frames?
Or we could use a smaller number of frames, like 3.

I am not really liking the feel of camera rotation and pitch.
Both panning and zooming don't have inertia but still feel smoother imo.
Maybe it's the linear mapping of translation delta to angle/pitch delta that feels off?

How can we prevent the cursor hovering behavior when using touch?
The entity higlighting behavior is tied to cursor position and we rely on that for touch,
but we don't want the cursor hovering behavior since it doesn't make sense for touch.

Should we try relying on action requests even for camera updates?
Currently we are directly modifying the camera object properties directly, which might be causing issues.
Maybe relying on action requests would be more robust and also more smooth?

Moving the cursor in the interaction context callback only doesn't feel smooth.
On the hand, moving the cursor in processBufferedPacketsIfReady() or in customWndProc()
feels good, but it will cause some issues like in the UI scrolling functionality which relies
on locking the cursor position.
We can probably have some state variable to indicate if we should activate "smooth cursor movement"
or not. Based on that, we can then conditionally move the cursor in processBufferedPacketsIfReady().

If we start a single finger swipe on a UI element and drag nothing then release the finger on the game world,
the release mouse button event will be processed by the game world and cause a click there even though the
press event was on the UI element. This makes the character move to that location at the release point.
Not sure how to fix this yet. We need to somehow know whether we dragged something or not or not use
an SDL release event at all. Instead, use in-game action requests? We could trigger UILeftMouse release
only. SDL left release also triggers Action1 release which is what causes the character to move.
We could also queue an Action1 release event immediately after the SDL left press event, which will
cancel the Action1 press event as we would still be over a UI element. An Action1 release event will
still be triggered, but I think it should be useless as it won't be preceded by an Action1 press event.