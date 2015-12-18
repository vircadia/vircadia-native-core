This PR demonstrates one way in-world editing of objects might work.  We start with a spotlight.  When you distant grab the sliders, you can move them along their axis to change their values.  You may also rotate / move the block to which the spotlight is attached.

To test: https://rawgit.com/imgntn/hifi/light_mod/examples/lights/lightLoader.js
To reset, I recommend stopping all scripts then re-loading lightLoader.js

When you run the lightLoader.js script, several scripts will be loaded:
- handControllerGrab.js (custom)
- lightModifier.js (listens for message to create sliders for a given light)
- lightModifierTestScene.js (creates a light and parents it to a block, then sends a message ^^)
- slider.js (attached to each slider entity)
- lightParent.js (attached to the entity to which a light is parented, so you can move it around)
- closeButton.js (for closing the ui)
- ../libraries/lightOverlayManager.js (custom)
- ../libraries/entitySelectionTool.js



Current sliders are (top to bottom):
red
green
blue
intensity
cutoff
exponent

To-Do: Determine how to enter / exit edit mode , support near grab, add other input types (checkbox, etc), prevent velocity drift on slider release,button to hide entity

![capture](https://cloud.githubusercontent.com/assets/843228/11830366/2f2dfe70-a359-11e5-84f0-33a380ebeac7.PNG)
