# Ability Usage Module

## Overview

This module is responsible for detecting abilities being used. This encompasses
"abilities" in the traditional sense but also includes actions like "jump" or
"throw" which are available in the game's hotbar. It also includes items that
can be used in the world, such as scrolls and throwable items.

Additionally, it handles the UI changes needed to add the "confirm" button when
an ability is being used that requires confirmation.

## Motivation

Consider the case where the player uses an ability outside of combat like some
AOE spell. They want to have precise control over its targeting, so we can't
just send a click event where the user clicked. Instead, we need to detect that
we are using an ability and disable clicks. Clicking on the world should simply
move the cursor to update the hover effects like showing the area of effect and
impacted targets. The actual execution of the ability should happen when the
user presses some 'confirm' button.

We could rely on a second tap on the same location as a signal to for
confirmation but we would need some distance threshold from the initial click
to register whether the second tap is a confirmation or a new target location
since touch input is inherently imprecise. However, this would eliminate the
ability to do micro-adjustments. So having a dedicated confirm button provides
a better user experience.