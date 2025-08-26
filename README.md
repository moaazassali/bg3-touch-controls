# README



## Dump of useful information
`CameraObject.rotationSpeed` is by default set to 90. 
Effect not tested.

`CameraObjection.angle` is the angle of the camera in degrees. 
It's range is (-360,360). 
If you rotate beyond -360, it resets to 0 and continutes going towards -360.
If you rotate beyond 360, it resets to 0 and continues going towards 360.
Seems extra [0, -360] seems redundant/strange.

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

`CameraObject.unkZoom_13C` is always 1. Seems irrelevant.

`CameraObject.zoomDelta` alternates between 100 and -100. 
-100 when zooming in and 100 when zooming out.
When not zooming, it is 0.

`CameraObject.desiredZoom` and `CameraObject.currentZoom_160` seem to always be equal and correspond to the desired target zoom.

`CameraObject.currentZoomA` and `CameraObject.currentZoomB` seem to always be equal and correspond to the actual current zoom of the camera.
Both of these values eventually reach `desiredZoom`.

`CameraObject.prevZoom_15C` seems to lag behind the actual current zoom (`currentZoomA` and `currentZoomA`).
It's final value when the camera stops zooming is typically close to desired/current zoom, but not exactly equal.

The range for the zoom values are [3.5, 12]


## Bugs to investigate
Crash when doing a lot of touch interactions while the game is launching?
Maybe this is because of the console?

Mouse zoom stops working after using touch to zoom?
