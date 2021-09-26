//
//  Created by Bradley Austin Davis on 2015/10/19
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Actions.h"

#include "impl/endpoints/ActionEndpoint.h"

namespace controller {

    Input::NamedPair makePair(ChannelType type, Action action, const QString& name) {
        auto input = Input(UserInputMapper::ACTIONS_DEVICE, toInt(action), type);
        return Input::NamedPair(input, name);
    }

    Input::NamedPair makeAxisPair(Action action, const QString& name) {
        return makePair(ChannelType::AXIS, action, name);
    }

    Input::NamedPair makeButtonPair(Action action, const QString& name) {
        return makePair(ChannelType::BUTTON, action, name);
    }

    Input::NamedPair makePosePair(Action action, const QString& name) {
        return makePair(ChannelType::POSE, action, name);
    }

    EndpointPointer ActionsDevice::createEndpoint(const Input& input) const {
        return std::make_shared<ActionEndpoint>(input);
    }

    /*@jsdoc
     * <p>The <code>Controller.Actions</code> object has properties representing predefined actions on the user's avatar and 
     * Interface. The property values are integer IDs, uniquely identifying each action. <em>Read-only.</em></p>
     * <p>These actions can be used as end points in the routes of a {@link MappingObject}. The data item routed to each action 
     * is either a number or a {@link Pose}.</p>
     *
     * <table>
     *   <thead>
     *     <tr><th>Property</th><th>Type</th><th>Data</th><th>Description</th></tr>
     *   </thead>
     *   <tbody>
     *     <tr><td colSpan=4><strong>Avatar Movement</strong></td></tr>
     *     <tr><td><code>TranslateX</code></td><td>number</td><td>number</td><td>Move the user's avatar in the direction of its 
     *       x-axis, if the camera isn't in independent or mirror modes.</td></tr>
     *     <tr><td><code>TranslateY</code></td><td>number</td><td>number</td><td>Move the user's avatar in the direction of its 
     *       y-axis, if the camera isn't in independent or mirror modes.</td></tr>
     *     <tr><td><code>TranslateZ</code></td><td>number</td><td>number</td><td>Move the user's avatar in the direction of its 
     *       z-axis, if the camera isn't in independent or mirror modes.</td></tr>
     *     <tr><td><code>Pitch</code></td><td>number</td><td>number</td><td>Rotate the user's avatar head and attached camera 
     *       about its negative x-axis (i.e., positive values pitch down) at a rate proportional to the control value, if the 
     *       camera isn't in HMD, independent, or mirror modes.</td></tr>
     *     <tr><td><code>Yaw</code></td><td>number</td><td>number</td><td>Rotate the user's avatar about its y-axis at a rate 
     *       proportional to the control value, if the camera isn't in independent or mirror modes.</td></tr>
     *     <tr><td><code>Roll</code></td><td>number</td><td>number</td><td>No action.</td></tr>
     *     <tr><td><code>DeltaPitch</code></td><td>number</td><td>number</td><td>Rotate the user's avatar head and attached 
     *       camera about its negative x-axis (i.e., positive values pitch down) by an amount proportional to the control value, 
     *       if the camera isn't in HMD, independent, or mirror modes.</td></tr>
     *     <tr><td><code>DeltaYaw</code></td><td>number</td><td>number</td><td>Rotate the user's avatar about its y-axis by an 
     *       amount proportional to the control value, if the camera isn't in independent or mirror modes.</td></tr>
     *     <tr><td><code>DeltaRoll</code></td><td>number</td><td>number</td><td>No action.</td></tr>
     *     <tr><td><code>StepTranslateX</code></td><td>number</td><td>number</td><td>No action.</td></tr>
     *     <tr><td><code>StepTranslateY</code></td><td>number</td><td>number</td><td>No action.</td></tr>
     *     <tr><td><code>StepTranslateZ</code></td><td>number</td><td>number</td><td>No action.</td></tr>
     *     <tr><td><code>StepPitch</code></td><td>number</td><td>number</td><td>No action.</td></tr>
     *     <tr><td><code>StepYaw</code></td><td>number</td><td>number</td><td>Rotate the user's avatar about its y-axis in a 
     *       step increment, if the camera isn't in independent or mirror modes.</td></tr>
     *     <tr><td><code>StepRoll</code></td><td>number</td><td>number</td><td>No action.</td></tr>
     *
     *     <tr><td colSpan=4><strong>Avatar Skeleton</strong></td></tr>
     *     <tr><td><code>Hips</code></td><td>number</td><td>{@link Pose}</td><td>Set the hips pose of the user's avatar.
     *       </td></tr>
     *     <tr><td><code>Spine2</code></td><td>number</td><td>{@link Pose}</td><td>Set the spine2 pose of the user's avatar.
     *       </td></tr>
     *     <tr><td><code>Head</code></td><td>number</td><td>{@link Pose}</td><td>Set the head pose of the user's avatar.
     *       </td></tr>
     *     <tr><td><code>LeftArm</code></td><td>number</td><td>{@link Pose}</td><td>Set the left arm pose of the user's avatar.
     *       </td></tr>
     *     <tr><td><code>RightArm</code></td><td>number</td><td>{@link Pose}</td><td>Set the right arm pose of the user's 
     *       avatar.</td></tr>
     *     <tr><td><code>LeftHand</code></td><td>number</td><td>{@link Pose}</td><td>Set the left hand pose of the user's
     *       avatar.</td></tr>
     *     <tr><td><code>LeftHandThumb1</code></td><td>number</td><td>{@link Pose}</td><td>Set the left thumb 1 finger joint 
     *       pose of the user's avatar.</td></tr>
     *     <tr><td><code>LeftHandThumb2</code></td><td>number</td><td>{@link Pose}</td><td>Set the left thumb 2 finger joint 
     *       pose of the user's avatar.</td></tr>
     *     <tr><td><code>LeftHandThumb3</code></td><td>number</td><td>{@link Pose}</td><td>Set the left thumb 3 finger joint 
     *       pose of the user's avatar.</td></tr>
     *     <tr><td><code>LeftHandThumb4</code></td><td>number</td><td>{@link Pose}</td><td>Set the left thumb 4 finger joint 
     *       pose of the user's avatar.</td></tr>
     *     <tr><td><code>LeftHandIndex1</code></td><td>number</td><td>{@link Pose}</td><td>Set the left index 1 finger joint 
     *       pose of the user's avatar.</td></tr>
     *     <tr><td><code>LeftHandIndex2</code></td><td>number</td><td>{@link Pose}</td><td>Set the left index 2 finger joint 
     *       pose of the user's avatar.</td></tr>
     *     <tr><td><code>LeftHandIndex3</code></td><td>number</td><td>{@link Pose}</td><td>Set the left index 3 finger joint 
     *       pose of the user's avatar.</td></tr>
     *     <tr><td><code>LeftHandIndex4</code></td><td>number</td><td>{@link Pose}</td><td>Set the left index 4 finger joint 
     *       pose of the user's avatar.</td></tr>
     *     <tr><td><code>LeftHandMiddle1</code></td><td>number</td><td>{@link Pose}</td><td>Set the left middle 1 finger joint 
     *       pose of the user's avatar.</td></tr>
     *     <tr><td><code>LeftHandMiddle2</code></td><td>number</td><td>{@link Pose}</td><td>Set the left middle 2 finger joint 
     *       pose of the user's avatar.</td></tr>
     *     <tr><td><code>LeftHandMiddle3</code></td><td>number</td><td>{@link Pose}</td><td>Set the left middle 3 finger joint 
     *       pose of the user's avatar.</td></tr>
     *     <tr><td><code>LeftHandMiddle4</code></td><td>number</td><td>{@link Pose}</td><td>Set the left middle 4 finger joint 
     *       pose of the user's avatar.</td></tr>
     *     <tr><td><code>LeftHandRing1</code></td><td>number</td><td>{@link Pose}</td><td>Set the left ring 1 finger joint pose 
     *       of the user's avatar.</td></tr>
     *     <tr><td><code>LeftHandRing2</code></td><td>number</td><td>{@link Pose}</td><td>Set the left ring 2 finger joint pose 
     *       of the user's avatar.</td></tr>
     *     <tr><td><code>LeftHandRing3</code></td><td>number</td><td>{@link Pose}</td><td>Set the left ring 3 finger joint pose 
     *       of the user's avatar.</td></tr>
     *     <tr><td><code>LeftHandRing4</code></td><td>number</td><td>{@link Pose}</td><td>Set the left ring 4 finger joint pose 
     *       of the user's avatar.</td></tr>
     *     <tr><td><code>LeftHandPinky1</code></td><td>number</td><td>{@link Pose}</td><td>Set the left pinky 1 finger joint 
     *       pose of the user's avatar.</td></tr>
     *     <tr><td><code>LeftHandPinky2</code></td><td>number</td><td>{@link Pose}</td><td>Set the left pinky 2 finger joint 
     *       pose of the user's avatar.</td></tr>
     *     <tr><td><code>LeftHandPinky3</code></td><td>number</td><td>{@link Pose}</td><td>Set the left pinky 3 finger joint 
     *       pose of the user's avatar.</td></tr>
     *     <tr><td><code>LeftHandPinky4</code></td><td>number</td><td>{@link Pose}</td><td>Set the left pinky 4 finger joint 
     *       pose of the user's avatar.</td></tr>
     *     <tr><td><code>RightHand</code></td><td>number</td><td>{@link Pose}</td><td>Set the right hand of the user's avatar.
     *       </td></tr>
     *     <tr><td><code>RightHandThumb1</code></td><td>number</td><td>{@link Pose}</td><td>Set the right thumb 1 finger joint 
     *       pose of the user's avatar.</td></tr>
     *     <tr><td><code>RightHandThumb2</code></td><td>number</td><td>{@link Pose}</td><td>Set the right thumb 2 finger joint 
     *       pose of the user's avatar.</td></tr>
     *     <tr><td><code>RightHandThumb3</code></td><td>number</td><td>{@link Pose}</td><td>Set the right thumb 3 finger joint 
     *       pose of the user's avatar.</td></tr>
     *     <tr><td><code>RightHandThumb4</code></td><td>number</td><td>{@link Pose}</td><td>Set the right thumb 4 finger joint 
     *       pose of the user's avatar.</td></tr>
     *     <tr><td><code>RightHandIndex1</code></td><td>number</td><td>{@link Pose}</td><td>Set the right index 1 finger joint 
     *       pose of the user's avatar.</td></tr>
     *     <tr><td><code>RightHandIndex2</code></td><td>number</td><td>{@link Pose}</td><td>Set the right index 2 finger joint 
     *       pose of the user's avatar.</td></tr>
     *     <tr><td><code>RightHandIndex3</code></td><td>number</td><td>{@link Pose}</td><td>Set the right index 3 finger joint 
     *       pose of the user's avatar.</td></tr>
     *     <tr><td><code>RightHandIndex4</code></td><td>number</td><td>{@link Pose}</td><td>Set the right index 4 finger joint 
     *       pose of the user's avatar.</td></tr>
     *     <tr><td><code>RightHandMiddle1</code></td><td>number</td><td>{@link Pose}</td><td>Set the right middle 1 finger 
     *       joint pose of the user's avatar.</td></tr>
     *     <tr><td><code>RightHandMiddle2</code></td><td>number</td><td>{@link Pose}</td><td>Set the right middle 2 finger 
     *       joint pose of the user's avatar.</td></tr>
     *     <tr><td><code>RightHandMiddle3</code></td><td>number</td><td>{@link Pose}</td><td>Set the right middle 3 finger 
     *       joint pose of the user's avatar.</td></tr>
     *     <tr><td><code>RightHandMiddle4</code></td><td>number</td><td>{@link Pose}</td><td>Set the right middle 4 finger 
     *       joint pose of the user's avatar.</td></tr>
     *     <tr><td><code>RightHandRing1</code></td><td>number</td><td>{@link Pose}</td><td>Set the right ring 1 finger joint 
     *       pose of the user's avatar.</td></tr>
     *     <tr><td><code>RightHandRing2</code></td><td>number</td><td>{@link Pose}</td><td>Set the right ring 2 finger joint 
     *       pose of the user's avatar.</td></tr>
     *     <tr><td><code>RightHandRing3</code></td><td>number</td><td>{@link Pose}</td><td>Set the right ring 3 finger joint 
     *       pose of the user's avatar.</td></tr>
     *     <tr><td><code>RightHandRing4</code></td><td>number</td><td>{@link Pose}</td><td>Set the right ring 4 finger joint 
     *       pose of the user's avatar.</td></tr>
     *     <tr><td><code>RightHandPinky1</code></td><td>number</td><td>{@link Pose}</td><td>Set the right pinky 1 finger joint 
     *       pose of the user's avatar.</td></tr>
     *     <tr><td><code>RightHandPinky2</code></td><td>number</td><td>{@link Pose}</td><td>Set the right pinky 2 finger joint 
     *       pose of the user's avatar.</td></tr>
     *     <tr><td><code>RightHandPinky3</code></td><td>number</td><td>{@link Pose}</td><td>Set the right pinky 3 finger joint 
     *       pose of the user's avatar.</td></tr>
     *     <tr><td><code>RightHandPinky4</code></td><td>number</td><td>{@link Pose}</td><td>Set the right pinky 4 finger joint 
     *       pose of the user's avatar.</td></tr>
     *     <tr><td><code>LeftFoot</code></td><td>number</td><td>{@link Pose}</td><td>Set the left foot pose of the user's
     *       avatar.</td></tr>
     *     <tr><td><code>RightFoot</code></td><td>number</td><td>{@link Pose}</td><td>Set the right foot pose of the user's
     *       avatar.</td></tr>
     *
     *     <tr><td colSpan=4><strong>Application</strong></td></tr>
     *     <tr><td><code>BoomIn</code></td><td>number</td><td>number</td><td>Zoom camera in from third person toward first 
     *       person view.</td></tr>
     *     <tr><td><code>BoomOut</code></td><td>number</td><td>number</td><td>Zoom camera out from first person to third 
     *       person view.</td></tr>
     *     <tr><td><code>CycleCamera</code></td><td>number</td><td>number</td><td>Cycle the camera view from first person look 
     *       at, to (third person) look at, to selfie if in desktop mode, then back to first person and repeat.</td></tr>
     *     <tr><td><code>ContextMenu</code></td><td>number</td><td>number</td><td>Show/hide the tablet.</td></tr>
     *     <tr><td><code>ToggleMute</code></td><td>number</td><td>number</td><td>Toggle the microphone mute.</td></tr>
     *     <tr><td><code>TogglePushToTalk</code></td><td>number</td><td>number</td><td>Toggle push to talk.</td></tr>
     *     <tr><td><code>ToggleOverlay</code></td><td>number</td><td>number</td><td>Toggle the display of overlays.</td></tr>
     *     <tr><td><code>Sprint</code></td><td>number</td><td>number</td><td>Set avatar sprint mode.</td></tr>
     *     <tr><td><code>ReticleClick</code></td><td>number</td><td>number</td><td>Set mouse-pressed.</td></tr>
     *     <tr><td><code>ReticleX</code></td><td>number</td><td>number</td><td>Move the cursor left/right in the x direction.
     *       </td></tr>
     *     <tr><td><code>ReticleY</code></td><td>number</td><td>number</td><td>move the cursor up/down in the y direction.
     *       </td></tr>
     *     <tr><td><code>ReticleLeft</code></td><td>number</td><td>number</td><td>Move the cursor left.</td></tr>
     *     <tr><td><code>ReticleRight</code></td><td>number</td><td>number</td><td>Move the cursor right.</td></tr>
     *     <tr><td><code>ReticleUp</code></td><td>number</td><td>number</td><td>Move the cursor up.</td></tr>
     *     <tr><td><code>ReticleDown</code></td><td>number</td><td>number</td><td>Move the cursor down.</td></tr>
     *     <tr><td><code>UiNavLateral</code></td><td>number</td><td>number</td><td>Generate a keyboard left or right arrow key 
     *       event.</td></tr>
     *     <tr><td><code>UiNavVertical</code></td><td>number</td><td>number</td><td>Generate a keyboard up or down arrow key 
     *       event.</td></tr>
     *     <tr><td><code>UiNavGroup</code></td><td>number</td><td>number</td><td>Generate a keyboard tab or back-tab key event.
     *       </td></tr>
     *     <tr><td><code>UiNavSelect</code></td><td>number</td><td>number</td><td>Generate a keyboard Enter key event.
     *       </td></tr>
     *     <tr><td><code>UiNavBack</code></td><td>number</td><td>number</td><td>Generate a keyboard Esc key event.</td></tr>
     *     <tr><td><code>LeftHandClick</code></td><td>number</td><td>number</td><td><span class="important">Deprecated: This 
     *       action is deprecated and will be removed. It takes no action.</span></td></tr>
     *     <tr><td><code>RightHandClick</code></td><td>number</td><td>number</td><td><span class="important">Deprecated: This 
     *       action is deprecated and will be removed. It takes no action.</span></td></tr>
     *     <tr><td><code>Shift</code></td><td>number</td><td>number</td><td><span class="important">Deprecated: This 
     *       action is deprecated and will be removed. It takes no action.</span></td></tr>
     *     <tr><td><code>PrimaryAction</code></td><td>number</td><td>number</td><td><span class="important">Deprecated: This 
     *       action is deprecated and will be removed. It takes no action.</span></td></tr>
     *     <tr><td><code>SecondaryAction</code></td><td>number</td><td>number</td><td><span class="important">Deprecated: This 
     *       action is deprecated and will be removed. It takes no action.</span></td></tr>
     *
     *     <tr><td colSpan=4><strong>Aliases</strong></td></tr>
     *     <tr><td><code>Backward</code></td><td>number</td><td>number</td><td>Alias for <code>TranslateZ</code> in the 
     *       positive direction.</td></tr>
     *     <tr><td><code>Forward</code></td><td>number</td><td>number</td><td>Alias for <code>TranslateZ</code> in the negative 
     *       direction.</td></tr>
     *     <tr><td><code>StrafeRight</code></td><td>number</td><td>number</td><td>Alias for <code>TranslateX</code> in the
     *       positive direction.</td></tr>
     *     <tr><td><code>StrafeLeft</code></td><td>number</td><td>number</td><td>Alias for <code>TranslateX</code> in the
     *       negative direction.</td></tr>
     *     <tr><td><code>Up</code></td><td>number</td><td>number</td><td>Alias for <code>TranslateY</code> in the positive
     *       direction.</td></tr>
     *     <tr><td><code>Down</code></td><td>number</td><td>number</td><td>Alias for <code>TranslateY</code> in the negative 
     *       direction.</td></tr>
     *     <tr><td><code>PitchDown</code></td><td>number</td><td>number</td><td>Alias for <code>Pitch</code> in the positive 
     *       direction.</td></tr>
     *     <tr><td><code>PitchUp</code></td><td>number</td><td>number</td><td>Alias for <code>Pitch</code> in the negative
     *       direction.</td></tr>
     *     <tr><td><code>YawLeft</code></td><td>number</td><td>number</td><td>Alias for <code>Yaw</code> in the positive
     *       direction.</td></tr>
     *     <tr><td><code>YawRight</code></td><td>number</td><td>number</td><td>Alias for <code>Yaw</code> in the negative 
     *       direction.</td></tr>
     *
     *     <tr><td colSpan=4><strong>Deprecated Aliases</strong></td></tr>
     *     <tr><td><code>LEFT_HAND</code></td><td>number</td><td>{@link Pose}</td><td><span class="important">Deprecated: This 
     *       action is deprecated and will be removed. Use <code>LeftHand</code> instead.</span></td></tr>
     *     <tr><td><code>RIGHT_HAND</code></td><td>number</td><td>{@link Pose}</td><td><span class="important">Deprecated: This 
     *       action is deprecated and will be removed. Use <code>RightHand</code> instead.</span></td></tr>
     *     <tr><td><code>BOOM_IN</code></td><td>number</td><td>number</td><td><span class="important">Deprecated: This 
     *       action is deprecated and will be removed. Use <code>BoomIn</code> instead.</span></td></tr>
     *     <tr><td><code>BOOM_OUT</code></td><td>number</td><td>number</td><td><span class="important">Deprecated: This 
     *       action is deprecated and will be removed. Use <code>BoomOut</code> instead.</span></td></tr>
     *     <tr><td><code>CONTEXT_MENU</code></td><td>number</td><td>number</td><td><span class="important">Deprecated: This 
     *       action is deprecated and will be removed. Use <code>ContextMenu</code> instead.</span></td></tr>
     *     <tr><td><code>TOGGLE_MUTE</code></td><td>number</td><td>number</td><td><span class="important">Deprecated: This 
     *       action is deprecated and will be removed. Use <code>ToggleMute</code> instead.</span></td></tr>
     *     <tr><td><code>TOGGLE_PUSHTOTALK</code></td><td>number</td><td>number</td><td><span class="important">Deprecated: This 
     *       action is deprecated and will be removed. Use <code>TogglePushToTalk</code> instead.</span></td></tr>
     *     <tr><td><code>SPRINT</code></td><td>number</td><td>number</td><td><span class="important">Deprecated: This 
     *       action is deprecated and will be removed. Use <code>Sprint</code> instead.</span></td></tr>
     *     <tr><td><code>LONGITUDINAL_BACKWARD</code></td><td>number</td><td>number</td><td><span class="important">Deprecated: This 
     *       action is deprecated and will be removed. Use <code>Backward</code> instead.</span></td></tr>
     *     <tr><td><code>LONGITUDINAL_FORWARD</code></td><td>number</td><td>number</td><td><span class="important">Deprecated: This 
     *       action is deprecated and will be removed. Use <code>Forward</code> instead.</span></td></tr>
     *     <tr><td><code>LATERAL_LEFT</code></td><td>number</td><td>number</td><td><span class="important">Deprecated: This 
     *       action is deprecated and will be removed. Use <code>StrafeLeft</code> instead.</span></td></tr>
     *     <tr><td><code>LATERAL_RIGHT</code></td><td>number</td><td>number</td><td><span class="important">Deprecated: This 
     *       action is deprecated and will be removed. Use <code>StrafeRight</code> instead.</span></td></tr>
     *     <tr><td><code>VERTICAL_UP</code></td><td>number</td><td>number</td><td><span class="important">Deprecated: This 
     *       action is deprecated and will be removed. Use <code>Up</code> instead.</span></td></tr>
     *     <tr><td><code>VERTICAL_DOWN</code></td><td>number</td><td>number</td><td><span class="important">Deprecated: This 
     *       action is deprecated and will be removed. Use <code>Down</code> instead.</span></td></tr>
     *     <tr><td><code>PITCH_DOWN</code></td><td>number</td><td>number</td><td><span class="important">Deprecated: This 
     *       action is deprecated and will be removed. Use <code>PitchDown</code> instead.</span></td></tr>
     *     <tr><td><code>PITCH_UP</code></td><td>number</td><td>number</td><td><span class="important">Deprecated: This 
     *       action is deprecated and will be removed. Use <code>PitchUp</code> instead.</span></td></tr>
     *     <tr><td><code>YAW_LEFT</code></td><td>number</td><td>number</td><td><span class="important">Deprecated: This 
     *       action is deprecated and will be removed. Use <code>YawLeft</code> instead.</span></td></tr>
     *     <tr><td><code>YAW_RIGHT</code></td><td>number</td><td>number</td><td><span class="important">Deprecated: This 
     *       action is deprecated and will be removed. Use <code>YawRight</code> instead.</span></td></tr>
     *     <tr><td><code>LEFT_HAND_CLICK</code></td><td>number</td><td>number</td><td><span class="important">Deprecated: This 
     *       action is deprecated and will be removed. Use <code>LeftHandClick</code> instead.</span></td></tr>
     *     <tr><td><code>RIGHT_HAND_CLICK</code></td><td>number</td><td>number</td><td><span class="important">Deprecated: This 
     *       action is deprecated and will be removed. Use <code>RightHandClick</code> instead.</span></td></tr>
     *     <tr><td><code>SHIFT</code></td><td>number</td><td>number</td><td><span class="important">Deprecated: This 
     *       action is deprecated and will be removed. Use <code>Shift</code> instead.</span></td></tr>
     *     <tr><td><code>ACTION1</code></td><td>number</td><td>number</td><td><span class="important">Deprecated: This 
     *       action is deprecated and will be removed. Use <code>PrimaryAction</code> instead.</span></td></tr>
     *     <tr><td><code>ACTION2</code></td><td>number</td><td>number</td><td><span class="important">Deprecated: This 
     *       action is deprecated and will be removed. Use <code>SecondaryAction</code> instead.</span></td></tr>
     *
     *     <tr><td colSpan=4><strong>Deprecated Trackers</strong></td><tr>
     *     <tr><td><code>TrackedObject00</code></td><td>number</td><td>{@link Pose}</td><td><span class="important">Deprecated: 
     *       This action is deprecated and will be removed. It takes no action.</span></td></tr>
     *     <tr><td><code>TrackedObject01</code></td><td>number</td><td>{@link Pose}</td><td><span class="important">Deprecated: 
     *       This action is deprecated and will be removed. It takes no action.</span></td></tr>
     *     <tr><td><code>TrackedObject02</code></td><td>number</td><td>{@link Pose}</td><td><span class="important">Deprecated: 
     *       This action is deprecated and will be removed. It takes no action.</span></td></tr>
     *     <tr><td><code>TrackedObject03</code></td><td>number</td><td>{@link Pose}</td><td><span class="important">Deprecated: 
     *       This action is deprecated and will be removed. It takes no action.</span></td></tr>
     *     <tr><td><code>TrackedObject04</code></td><td>number</td><td>{@link Pose}</td><td><span class="important">Deprecated: 
     *       This action is deprecated and will be removed. It takes no action.</span></td></tr>
     *     <tr><td><code>TrackedObject05</code></td><td>number</td><td>{@link Pose}</td><td><span class="important">Deprecated: 
     *       This action is deprecated and will be removed. It takes no action.</span></td></tr>
     *     <tr><td><code>TrackedObject06</code></td><td>number</td><td>{@link Pose}</td><td><span class="important">Deprecated: 
     *       This action is deprecated and will be removed. It takes no action.</span></td></tr>
     *     <tr><td><code>TrackedObject07</code></td><td>number</td><td>{@link Pose}</td><td><span class="important">Deprecated: 
     *       This action is deprecated and will be removed. It takes no action.</span></td></tr>
     *     <tr><td><code>TrackedObject08</code></td><td>number</td><td>{@link Pose}</td><td><span class="important">Deprecated: 
     *       This action is deprecated and will be removed. It takes no action.</span></td></tr>
     *     <tr><td><code>TrackedObject09</code></td><td>number</td><td>{@link Pose}</td><td><span class="important">Deprecated: 
     *       This action is deprecated and will be removed. It takes no action.</span></td></tr>
     *     <tr><td><code>TrackedObject10</code></td><td>number</td><td>{@link Pose}</td><td><span class="important">Deprecated: 
     *       This action is deprecated and will be removed. It takes no action.</span></td></tr>
     *     <tr><td><code>TrackedObject11</code></td><td>number</td><td>{@link Pose}</td><td><span class="important">Deprecated: 
     *       This action is deprecated and will be removed. It takes no action.</span></td></tr>
     *     <tr><td><code>TrackedObject12</code></td><td>number</td><td>{@link Pose}</td><td><span class="important">Deprecated: 
     *       This action is deprecated and will be removed. It takes no action.</span></td></tr>
     *     <tr><td><code>TrackedObject13</code></td><td>number</td><td>{@link Pose}</td><td><span class="important">Deprecated: 
     *       This action is deprecated and will be removed. It takes no action.</span></td></tr>
     *     <tr><td><code>TrackedObject14</code></td><td>number</td><td>{@link Pose}</td><td><span class="important">Deprecated: 
     *       This action is deprecated and will be removed. It takes no action.</span></td></tr>
     *     <tr><td><code>TrackedObject15</code></td><td>number</td><td>{@link Pose}</td><td><span class="important">Deprecated: 
     *       This action is deprecated and will be removed. It takes no action.</span></td></tr>
     *   </tbody>
     * </table>
     * @typedef {object} Controller.Actions
     */
    // Device functions
    Input::NamedVector ActionsDevice::getAvailableInputs() const {
        static Input::NamedVector availableInputs {
            makeAxisPair(Action::TRANSLATE_X, "TranslateX"),
            makeAxisPair(Action::TRANSLATE_Y, "TranslateY"),
            makeAxisPair(Action::TRANSLATE_Z, "TranslateZ"),
            makeAxisPair(Action::ROLL, "Roll"),
            makeAxisPair(Action::PITCH, "Pitch"),
            makeAxisPair(Action::YAW, "Yaw"),
            makeAxisPair(Action::DELTA_YAW, "DeltaYaw"),
            makeAxisPair(Action::DELTA_PITCH, "DeltaPitch"),
            makeAxisPair(Action::DELTA_ROLL, "DeltaRoll"),
            makeAxisPair(Action::STEP_YAW, "StepYaw"),
            makeAxisPair(Action::STEP_PITCH, "StepPitch"),
            makeAxisPair(Action::STEP_ROLL, "StepRoll"),
            makeAxisPair(Action::STEP_TRANSLATE_X, "StepTranslateX"),
            makeAxisPair(Action::STEP_TRANSLATE_Y, "StepTranslateY"),
            makeAxisPair(Action::STEP_TRANSLATE_Z, "StepTranslateZ"),

            makePosePair(Action::LEFT_HAND, "LeftHand"),
            makePosePair(Action::RIGHT_HAND, "RightHand"),
            makePosePair(Action::RIGHT_ARM, "RightArm"),
            makePosePair(Action::LEFT_ARM, "LeftArm"),
            makePosePair(Action::LEFT_FOOT, "LeftFoot"),
            makePosePair(Action::RIGHT_FOOT, "RightFoot"),
            makePosePair(Action::HIPS, "Hips"),
            makePosePair(Action::SPINE2, "Spine2"),
            makePosePair(Action::HEAD, "Head"),
            makePosePair(Action::LEFT_EYE, "LeftEye"),
            makePosePair(Action::RIGHT_EYE, "RightEye"),

            // blendshapes
            makeAxisPair(Action::EYEBLINK_L, "EyeBlink_L"),
            makeAxisPair(Action::EYEBLINK_R, "EyeBlink_R"),
            makeAxisPair(Action::EYESQUINT_L, "EyeSquint_L"),
            makeAxisPair(Action::EYESQUINT_R, "EyeSquint_R"),
            makeAxisPair(Action::EYEDOWN_L, "EyeDown_L"),
            makeAxisPair(Action::EYEDOWN_R, "EyeDown_R"),
            makeAxisPair(Action::EYEIN_L, "EyeIn_L"),
            makeAxisPair(Action::EYEIN_R, "EyeIn_R"),
            makeAxisPair(Action::EYEOPEN_L, "EyeOpen_L"),
            makeAxisPair(Action::EYEOPEN_R, "EyeOpen_R"),
            makeAxisPair(Action::EYEOUT_L, "EyeOut_L"),
            makeAxisPair(Action::EYEOUT_R, "EyeOut_R"),
            makeAxisPair(Action::EYEUP_L, "EyeUp_L"),
            makeAxisPair(Action::EYEUP_R, "EyeUp_R"),
            makeAxisPair(Action::BROWSD_L, "BrowsD_L"),
            makeAxisPair(Action::BROWSD_R, "BrowsD_R"),
            makeAxisPair(Action::BROWSU_C, "BrowsU_C"),
            makeAxisPair(Action::BROWSU_L, "BrowsU_L"),
            makeAxisPair(Action::BROWSU_R, "BrowsU_R"),
            makeAxisPair(Action::JAWFWD, "JawFwd"),
            makeAxisPair(Action::JAWLEFT, "JawLeft"),
            makeAxisPair(Action::JAWOPEN, "JawOpen"),
            makeAxisPair(Action::JAWRIGHT, "JawRight"),
            makeAxisPair(Action::MOUTHLEFT, "MouthLeft"),
            makeAxisPair(Action::MOUTHRIGHT, "MouthRight"),
            makeAxisPair(Action::MOUTHFROWN_L, "MouthFrown_L"),
            makeAxisPair(Action::MOUTHFROWN_R, "MouthFrown_R"),
            makeAxisPair(Action::MOUTHSMILE_L, "MouthSmile_L"),
            makeAxisPair(Action::MOUTHSMILE_R, "MouthSmile_R"),
            makeAxisPair(Action::MOUTHDIMPLE_L, "MouthDimple_L"),
            makeAxisPair(Action::MOUTHDIMPLE_R, "MouthDimple_R"),
            makeAxisPair(Action::LIPSSTRETCH_L, "LipsStretch_L"),
            makeAxisPair(Action::LIPSSTRETCH_R, "LipsStretch_R"),
            makeAxisPair(Action::LIPSUPPERCLOSE, "LipsUpperClose"),
            makeAxisPair(Action::LIPSLOWERCLOSE, "LipsLowerClose"),
            makeAxisPair(Action::LIPSFUNNEL, "LipsFunnel"),
            makeAxisPair(Action::LIPSPUCKER, "LipsPucker"),
            makeAxisPair(Action::PUFF, "Puff"),
            makeAxisPair(Action::CHEEKSQUINT_L, "CheekSquint_L"),
            makeAxisPair(Action::CHEEKSQUINT_R, "CheekSquint_R"),
            makeAxisPair(Action::MOUTHCLOSE, "MouthClose"),
            makeAxisPair(Action::MOUTHUPPERUP_L, "MouthUpperUp_L"),
            makeAxisPair(Action::MOUTHUPPERUP_R, "MouthUpperUp_R"),
            makeAxisPair(Action::MOUTHLOWERDOWN_L, "MouthLowerDown_L"),
            makeAxisPair(Action::MOUTHLOWERDOWN_R, "MouthLowerDown_R"),
            makeAxisPair(Action::MOUTHPRESS_L, "MouthPress_L"),
            makeAxisPair(Action::MOUTHPRESS_R, "MouthPress_R"),
            makeAxisPair(Action::MOUTHSHRUGLOWER, "MouthShrugLower"),
            makeAxisPair(Action::MOUTHSHRUGUPPER, "MouthShrugUpper"),
            makeAxisPair(Action::NOSESNEER_L, "NoseSneer_L"),
            makeAxisPair(Action::NOSESNEER_R, "NoseSneer_R"),
            makeAxisPair(Action::TONGUEOUT, "TongueOut"),
            makeAxisPair(Action::USERBLENDSHAPE0, "UserBlendshape0"),
            makeAxisPair(Action::USERBLENDSHAPE1, "UserBlendshape1"),
            makeAxisPair(Action::USERBLENDSHAPE2, "UserBlendshape2"),
            makeAxisPair(Action::USERBLENDSHAPE3, "UserBlendshape3"),
            makeAxisPair(Action::USERBLENDSHAPE4, "UserBlendshape4"),
            makeAxisPair(Action::USERBLENDSHAPE5, "UserBlendshape5"),
            makeAxisPair(Action::USERBLENDSHAPE6, "UserBlendshape6"),
            makeAxisPair(Action::USERBLENDSHAPE7, "UserBlendshape7"),
            makeAxisPair(Action::USERBLENDSHAPE8, "UserBlendshape8"),
            makeAxisPair(Action::USERBLENDSHAPE9, "UserBlendshape9"),

            makePosePair(Action::LEFT_HAND_THUMB1, "LeftHandThumb1"),
            makePosePair(Action::LEFT_HAND_THUMB2, "LeftHandThumb2"),
            makePosePair(Action::LEFT_HAND_THUMB3, "LeftHandThumb3"),
            makePosePair(Action::LEFT_HAND_THUMB4, "LeftHandThumb4"),
            makePosePair(Action::LEFT_HAND_INDEX1, "LeftHandIndex1"),
            makePosePair(Action::LEFT_HAND_INDEX2, "LeftHandIndex2"),
            makePosePair(Action::LEFT_HAND_INDEX3, "LeftHandIndex3"),
            makePosePair(Action::LEFT_HAND_INDEX4, "LeftHandIndex4"),
            makePosePair(Action::LEFT_HAND_MIDDLE1, "LeftHandMiddle1"),
            makePosePair(Action::LEFT_HAND_MIDDLE2, "LeftHandMiddle2"),
            makePosePair(Action::LEFT_HAND_MIDDLE3, "LeftHandMiddle3"),
            makePosePair(Action::LEFT_HAND_MIDDLE4, "LeftHandMiddle4"),
            makePosePair(Action::LEFT_HAND_RING1, "LeftHandRing1"),
            makePosePair(Action::LEFT_HAND_RING2, "LeftHandRing2"),
            makePosePair(Action::LEFT_HAND_RING3, "LeftHandRing3"),
            makePosePair(Action::LEFT_HAND_RING4, "LeftHandRing4"),
            makePosePair(Action::LEFT_HAND_PINKY1, "LeftHandPinky1"),
            makePosePair(Action::LEFT_HAND_PINKY2, "LeftHandPinky2"),
            makePosePair(Action::LEFT_HAND_PINKY3, "LeftHandPinky3"),
            makePosePair(Action::LEFT_HAND_PINKY4, "LeftHandPinky4"),

            makePosePair(Action::RIGHT_HAND_THUMB1, "RightHandThumb1"),
            makePosePair(Action::RIGHT_HAND_THUMB2, "RightHandThumb2"),
            makePosePair(Action::RIGHT_HAND_THUMB3, "RightHandThumb3"),
            makePosePair(Action::RIGHT_HAND_THUMB4, "RightHandThumb4"),
            makePosePair(Action::RIGHT_HAND_INDEX1, "RightHandIndex1"),
            makePosePair(Action::RIGHT_HAND_INDEX2, "RightHandIndex2"),
            makePosePair(Action::RIGHT_HAND_INDEX3, "RightHandIndex3"),
            makePosePair(Action::RIGHT_HAND_INDEX4, "RightHandIndex4"),
            makePosePair(Action::RIGHT_HAND_MIDDLE1, "RightHandMiddle1"),
            makePosePair(Action::RIGHT_HAND_MIDDLE2, "RightHandMiddle2"),
            makePosePair(Action::RIGHT_HAND_MIDDLE3, "RightHandMiddle3"),
            makePosePair(Action::RIGHT_HAND_MIDDLE4, "RightHandMiddle4"),
            makePosePair(Action::RIGHT_HAND_RING1, "RightHandRing1"),
            makePosePair(Action::RIGHT_HAND_RING2, "RightHandRing2"),
            makePosePair(Action::RIGHT_HAND_RING3, "RightHandRing3"),
            makePosePair(Action::RIGHT_HAND_RING4, "RightHandRing4"),
            makePosePair(Action::RIGHT_HAND_PINKY1, "RightHandPinky1"),
            makePosePair(Action::RIGHT_HAND_PINKY2, "RightHandPinky2"),
            makePosePair(Action::RIGHT_HAND_PINKY3, "RightHandPinky3"),
            makePosePair(Action::RIGHT_HAND_PINKY4, "RightHandPinky4"),

            makePosePair(Action::TRACKED_OBJECT_00, "TrackedObject00"),
            makePosePair(Action::TRACKED_OBJECT_01, "TrackedObject01"),
            makePosePair(Action::TRACKED_OBJECT_02, "TrackedObject02"),
            makePosePair(Action::TRACKED_OBJECT_03, "TrackedObject03"),
            makePosePair(Action::TRACKED_OBJECT_04, "TrackedObject04"),
            makePosePair(Action::TRACKED_OBJECT_05, "TrackedObject05"),
            makePosePair(Action::TRACKED_OBJECT_06, "TrackedObject06"),
            makePosePair(Action::TRACKED_OBJECT_07, "TrackedObject07"),
            makePosePair(Action::TRACKED_OBJECT_08, "TrackedObject08"),
            makePosePair(Action::TRACKED_OBJECT_09, "TrackedObject09"),
            makePosePair(Action::TRACKED_OBJECT_10, "TrackedObject10"),
            makePosePair(Action::TRACKED_OBJECT_11, "TrackedObject11"),
            makePosePair(Action::TRACKED_OBJECT_12, "TrackedObject12"),
            makePosePair(Action::TRACKED_OBJECT_13, "TrackedObject13"),
            makePosePair(Action::TRACKED_OBJECT_14, "TrackedObject14"),
            makePosePair(Action::TRACKED_OBJECT_15, "TrackedObject15"),

            makeButtonPair(Action::LEFT_HAND_CLICK, "LeftHandClick"),
            makeButtonPair(Action::RIGHT_HAND_CLICK, "RightHandClick"),

            makeButtonPair(Action::SHIFT, "Shift"),
            makeButtonPair(Action::ACTION1, "PrimaryAction"),
            makeButtonPair(Action::ACTION2, "SecondaryAction"),
            makeButtonPair(Action::CONTEXT_MENU, "ContextMenu"),
            makeButtonPair(Action::TOGGLE_MUTE, "ToggleMute"),
            makeButtonPair(Action::TOGGLE_PUSHTOTALK, "TogglePushToTalk"),
            makeButtonPair(Action::CYCLE_CAMERA, "CycleCamera"),
            makeButtonPair(Action::TOGGLE_OVERLAY, "ToggleOverlay"),
            makeButtonPair(Action::SPRINT, "Sprint"),

            makeAxisPair(Action::RETICLE_CLICK, "ReticleClick"),
            makeAxisPair(Action::RETICLE_X, "ReticleX"),
            makeAxisPair(Action::RETICLE_Y, "ReticleY"),
            makeAxisPair(Action::RETICLE_LEFT, "ReticleLeft"),
            makeAxisPair(Action::RETICLE_RIGHT, "ReticleRight"),
            makeAxisPair(Action::RETICLE_UP, "ReticleUp"),
            makeAxisPair(Action::RETICLE_DOWN, "ReticleDown"),

            makeAxisPair(Action::UI_NAV_LATERAL, "UiNavLateral"),
            makeAxisPair(Action::UI_NAV_VERTICAL, "UiNavVertical"),
            makeAxisPair(Action::UI_NAV_GROUP, "UiNavGroup"),
            makeAxisPair(Action::UI_NAV_SELECT, "UiNavSelect"),
            makeAxisPair(Action::UI_NAV_BACK, "UiNavBack"),

            // Aliases and bisected versions
            makeAxisPair(Action::LONGITUDINAL_BACKWARD, "Backward"),
            makeAxisPair(Action::LONGITUDINAL_FORWARD, "Forward"),
            makeAxisPair(Action::LATERAL_LEFT, "StrafeLeft"),
            makeAxisPair(Action::LATERAL_RIGHT, "StrafeRight"),
            makeAxisPair(Action::VERTICAL_DOWN, "Down"),
            makeAxisPair(Action::VERTICAL_UP, "Up"),
            makeAxisPair(Action::YAW_LEFT, "YawLeft"),
            makeAxisPair(Action::YAW_RIGHT, "YawRight"),
            makeAxisPair(Action::PITCH_DOWN, "PitchDown"),
            makeAxisPair(Action::PITCH_UP, "PitchUp"),
            makeAxisPair(Action::BOOM_IN, "BoomIn"),
            makeAxisPair(Action::BOOM_OUT, "BoomOut"),

            // Deprecated aliases
            // FIXME remove after we port all scripts
            makeAxisPair(Action::LONGITUDINAL_BACKWARD, "LONGITUDINAL_BACKWARD"),
            makeAxisPair(Action::LONGITUDINAL_FORWARD, "LONGITUDINAL_FORWARD"),
            makeAxisPair(Action::LATERAL_LEFT, "LATERAL_LEFT"),
            makeAxisPair(Action::LATERAL_RIGHT, "LATERAL_RIGHT"),
            makeAxisPair(Action::VERTICAL_DOWN, "VERTICAL_DOWN"),
            makeAxisPair(Action::VERTICAL_UP, "VERTICAL_UP"),
            makeAxisPair(Action::YAW_LEFT, "YAW_LEFT"),
            makeAxisPair(Action::YAW_RIGHT, "YAW_RIGHT"),
            makeAxisPair(Action::PITCH_DOWN, "PITCH_DOWN"),
            makeAxisPair(Action::PITCH_UP, "PITCH_UP"),
            makeAxisPair(Action::BOOM_IN, "BOOM_IN"),
            makeAxisPair(Action::BOOM_OUT, "BOOM_OUT"),

            makePosePair(Action::LEFT_HAND, "LEFT_HAND"),
            makePosePair(Action::RIGHT_HAND, "RIGHT_HAND"),

            makeButtonPair(Action::LEFT_HAND_CLICK, "LEFT_HAND_CLICK"),
            makeButtonPair(Action::RIGHT_HAND_CLICK, "RIGHT_HAND_CLICK"),

            makeButtonPair(Action::SHIFT, "SHIFT"),
            makeButtonPair(Action::ACTION1, "ACTION1"),
            makeButtonPair(Action::ACTION2, "ACTION2"),
            makeButtonPair(Action::CONTEXT_MENU, "CONTEXT_MENU"),
            makeButtonPair(Action::TOGGLE_MUTE, "TOGGLE_MUTE"),
            makeButtonPair(Action::SPRINT, "SPRINT")
        };
        return availableInputs;
    }

    ActionsDevice::ActionsDevice() : InputDevice("Actions") {
        _deviceID = UserInputMapper::ACTIONS_DEVICE;
    }

}
