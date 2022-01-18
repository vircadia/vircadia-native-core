//
//  Created by James B. Pollack @imgntn on April 18, 2016.
//  Copyright 2016 High Fidelity, Inc.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// An animation of the avatar clapping its hands while standing
var ANIM_URL = "https://cdn-1.vircadia.com/us-e-1/Developer/Tutorials/avatarAnimation/clap.fbx";

// overrideRoleAnimation only replaces a single animation at a time.  the rest of the motion is driven normally
// @animationRole - name of animation
// @animationURL - url
// @playbackRate  - frames per second
// @loopFlag - should loop
// @startFrame
// @endFrame
MyAvatar.overrideRoleAnimation("idleStand", ANIM_URL, 30, true, 0, 53);

//stop a a minute because your hands got tired.
Script.setTimeout(function(){
MyAvatar.restoreRoleAnimation("idleStand");
Script.stop();
},60000)

//or stop when the script is manually stopped
Script.scriptEnding.connect(function () {
MyAvatar.restoreRoleAnimation("idleStand");
});

//overRideAnimation replaces all animations, so you dont have to provide a role name
// @animationURL - url
// @playbackRate  - frames per second
// @loopFlag - should loop
// @startFrame
// @endFrame
//MyAvatar.overrideAnimation(ANIM_URL, 30, true, 0, 53);
