# Game Camera Module

## Overview

This module exposes the game camera object and its update functionality. This
is useful for scenarios where direct camera manipulation or querying is
required that are otherwise not available through higher-level abstractions.


## Motivation
1- We need to control the camera's pitch and there are no action requests that do
that for us. So we need direct access to the camera object.

2- We also need to detect when we are in combat mode, which the camera object
can provide.