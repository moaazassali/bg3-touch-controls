# UI Bindings Module

## Overview

This module exposes any needed UI bindings. Specifically, this is what a 'UI
Binding' refers to:
In NoesisGUI xaml files, you will find strings like this:
```
Binding Path=DataContext.CurrentPlayer.SelectedCharacter.InTurnBasedMode
```
So if we see any useful bindings in the UI xaml files, we can do a text search
for it in the disassembled code, and find out the function that gets called to
get the value for that binding. So we can use those same functions to get a lot
of useful information about the game state.