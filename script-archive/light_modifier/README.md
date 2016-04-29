*Temporarily Deprecated - needs a better way to know when 'grab beams' intersect with 'light overlays'.  Sending messages containing the ray from the hand grab script to the overlay intersection test doesn't seem to be sustainable. *

This PR demonstrates one way in-world editing of objects might work. 

Running this script will show light overlay icons in-world.  Enter edit mode by running your distance beam through a light overlay.  Exit using the red X.

When you distant grab the sliders, you can move them along their axis to change their values.  You may also rotate / move the block to which the spotlight is attached.  

To test: https://rawgit.com/imgntn/hifi/light_mod/examples/lights/lightLoader.js
To reset, I recommend stopping all scripts then re-loading lightLoader.js

When you run the lightLoader.js script, several scripts will be loaded:
- handControllerGrab.js (will not impart velocity when you move the parent or a slider, will not move sliders with head movement,will constrain movement for a slider to a given axis start and end, will support blacklisting of entities for raypicking during search for objects)
- lightModifier.js (listens for message to create sliders for a given light.  will start with slider set to the light's initial properties)
- lightModifierTestScene.js (creates a light)
- slider.js (attached to each slider entity)
- lightParent.js (attached to a 3d model of a light, to which a light is parented, so you can move it around.  or keep the current parent if a light already has a parent)
- visiblePanel.js (the transparent panel)
- closeButton.js (for closing the ui)
- ../libraries/lightOverlayManager.js (shows 2d overlays for lights in the world)
- ../libraries/entitySelectionTool.js (visualizes volume of the lights)

Current sliders are (top to bottom):
red
green
blue
intensity
cutoff
exponent

![capture](https://cloud.githubusercontent.com/assets/843228/11910139/afaaf1ae-a5a5-11e5-8b66-0eb3fc6976df.PNG)
