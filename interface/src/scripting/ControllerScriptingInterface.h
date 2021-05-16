//
//  ControllerScriptingInterface.h
//  interface/src/scripting
//
//  Created by Brad Hefta-Gaub on 12/17/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ControllerScriptingInterface_h
#define hifi_ControllerScriptingInterface_h

#include <QtCore/QObject>

#include <controllers/UserInputMapper.h>
#include <controllers/ScriptingInterface.h>

#include <KeyEvent.h>
#include <MouseEvent.h>
#include <SpatialEvent.h>
#include <TouchEvent.h>
#include <WheelEvent.h>
class ScriptEngine;


/*@jsdoc
 * The <code>Controller</code> API provides facilities to interact with computer and controller hardware.
 *
 * <h3>Facilities</h3>
 *
 * <h4>Properties</h4>
 * <p>Get <code>Controller</code> property trees.</p>
 * <ul>
 *   <li>{@link Controller.getActions|getActions}</li>
 *   <li>{@link Controller.getHardware|getHardware}</li>
 *   <li>{@link Controller.getStandard|getStandard}</li>
 * </ul>
 *
 * <h4>Mappings</h4>
 * <p>Create and enable or disable <code>Controller</code> mappings.</p>
 * <ul>
 *   <li>{@link Controller.disableMapping|disableMapping}</li>
 *   <li>{@link Controller.enableMapping|enableMapping}</li>
 *   <li>{@link Controller.loadMapping|loadMapping}</li>
 *   <li>{@link Controller.newMapping|newMapping}</li>
 *   <li>{@link Controller.parseMapping|parseMapping}</li>
 * </ul>
 *
 * <h4>Input, Hardware, and Action Reflection</h4>
 * <p>Information on the devices and actions available.</p>
 * <ul>
 *   <li>{@link Controller.findAction|findAction}</li>
 *   <li>{@link Controller.findDevice|findDevice}</li>
 *   <li>{@link Controller.getActionNames|getActionNames}</li>
 *   <li>{@link Controller.getAllActions|getAllActions}</li>
 *   <li>{@link Controller.getAvailableInputs|getAvailableInputs}</li>
 *   <li>{@link Controller.getDeviceName|getDeviceName}</li>
 *   <li>{@link Controller.getDeviceNames|getDeviceNames}</li>
 *   <li>{@link Controller.getRunningInputDevices|getRunningInputDevices}</li>
 * </ul>
 *
 * <h4>Input, Hardware, and Action Signals</h4>
 * <p>Notifications of device and action events.</p>
 * <ul>
 *   <li>{@link Controller.actionEvent|actionEvent}</li>
 *   <li>{@link Controller.hardwareChanged|hardwareChanged}</li>
 *   <li>{@link Controller.inputDeviceRunningChanged|inputDeviceRunningChanged}</li>
 *   <li>{@link Controller.inputEvent|inputEvent}</li>
 * </ul>
 *
 * <h4>Mouse, Keyboard, and Touch Signals</h4>
 * <p>Notifications of mouse, keyboard, and touch events.</p>
 * <ul>
 *   <li>{@link Controller.keyPressEvent|keyPressEvent}</li>
 *   <li>{@link Controller.keyReleaseEvent|keyReleaseEvent}</li>
 *   <li>{@link Controller.mouseDoublePressEvent|mouseDoublePressEvent}</li>
 *   <li>{@link Controller.mouseMoveEvent|mouseMoveEvent}</li>
 *   <li>{@link Controller.mousePressEvent|mousePressEvent}</li>
 *   <li>{@link Controller.mouseReleaseEvent|mouseReleaseEvent}</li>
 *   <li>{@link Controller.touchBeginEvent|touchBeginEvent}</li>
 *   <li>{@link Controller.touchEndEvent|touchEndEvent}</li>
 *   <li>{@link Controller.touchUpdateEvent|touchUpdateEvent}</li>
 *   <li>{@link Controller.wheelEvent|wheelEvent}</li>
 * </ul>
 *
 * <h4>Control Capturing</h4>
 * <p>Disable and enable the processing of mouse and touch events.</p>
 * <ul>
 *   <li>{@link Controller.captureMouseEvents|captureMouseEvents}</li>
 *   <li>{@link Controller.captureWheelEvents|captureWheelEvents}</li>
 *   <li>{@link Controller.captureTouchEvents|captureTouchEvents}</li>
 *   <li>{@link Controller.releaseMouseEvents|releaseMouseEvents}</li>
 *   <li>{@link Controller.releaseWheelEvents|releaseWheelEvents}</li>
 *   <li>{@link Controller.releaseTouchEvents|releaseTouchEvents}</li>
 * </ul>
 *
 * <h4>Action Capturing</h4>
 * <p>Disable and enable controller actions.</p>
 * <ul>
 *   <li>{@link Controller.captureActionEvents|captureActionEvents}</li>
 *   <li>{@link Controller.captureKeyEvents|captureKeyEvents}</li>
 *   <li>{@link Controller.captureJoystick|captureJoystick}</li>
 *   <li>{@link Controller.captureEntityClickEvents|captureEntityClickEvents}</li>
 *   <li>{@link Controller.releaseActionEvents|releaseActionEvents}</li>
 *   <li>{@link Controller.releaseKeyEvents|releaseKeyEvents}</li>
 *   <li>{@link Controller.releaseJoystick|releaseJoystick}</li>
 *   <li>{@link Controller.releaseEntityClickEvents|releaseEntityClickEvents}</li>
 * </ul>
 *
 * <h4>Controller and Action Values</h4>
 * <p>Get the current value of controller outputs and actions.</p>
 * <ul>
 *   <li>{@link Controller.getValue|getValue}</li>
 *   <li>{@link Controller.getAxisValue|getAxisValue}</li>
 *   <li>{@link Controller.getPoseValue|getPoseValue}</li>
 *   <li>{@link Controller.getActionValue|getActionValue}</li>
 * </ul>
 *
 * <h4>Haptics</h4>
 * <p>Trigger haptic pulses.</p>
 * <ul>
 *   <li>{@link Controller.triggerHapticPulse|triggerHapticPulse}</li>
 *   <li>{@link Controller.triggerHapticPulseOnDevice|triggerHapticPulseOnDevice}</li>
 *   <li>{@link Controller.triggerShortHapticPulse|triggerShortHapticPulse}</li>
 *   <li>{@link Controller.triggerShortHapticPulseOnDevice|triggerShortHapticPulseOnDevice}</li>
 * </ul>
 *
 * <h4>Display Information</h4>
 * <p>Get information on the display.</p>
 * <ul>
 *   <li>{@link Controller.getViewportDimensions|getViewportDimensions}</li>
 *   <li>{@link Controller.getRecommendedHUDRect|getRecommendedHUDRect}</li>
 * </ul>
 *
 * <h4>Virtual Game Pad</h4>
 * <p>Use the virtual game pad which is available on some devices.</p>
 * <ul>
 *   <li>{@link Controller.setVPadEnabled|setVPadEnabled}</li>
 *   <li>{@link Controller.setVPadHidden|setVPadHidden}</li>
 *   <li>{@link Controller.setVPadExtraBottomMargin|setVPadExtraBottomMargin}</li>
 * </ul>
 *
 * <h4>Input Recordings</h4>
 * <p>Create and play input recordings.</p>
 * <ul>
 *   <li>{@link Controller.startInputRecording|startInputRecording}</li>
 *   <li>{@link Controller.stopInputRecording|stopInputRecording}</li>
 *   <li>{@link Controller.saveInputRecording|saveInputRecording}</li>
 *   <li>{@link Controller.getInputRecorderSaveDirectory|getInputRecorderSaveDirectory}</li>
 *   <li>{@link Controller.loadInputRecording|loadInputRecording}</li>
 *   <li>{@link Controller.startInputPlayback|startInputPlayback}</li>
 *   <li>{@link Controller.stopInputPlayback|stopInputPlayback}</li>
 * </ul>
 *
 * <h3>Entity Methods</h3>
 *
 * <p>The default scripts implement hand controller actions that use {@link Entities.callEntityMethod} to call entity script 
 * methods, if present, in the entity being interacted with.</p>
 *
 * <table>
 *   <thead>
 *     <tr><th>Method Name</th><th>Description</th><th>Example</th></tr>
 *   </thead>
 *   <tbody>
 *     <tr>
 *       <td><code>startFarTrigger</code><br /><code>continueFarTrigger</code><br /><code>stopFarTrigger</code></td>
 *       <td>These methods are called when a user is more than 0.3m away from the entity, the entity is triggerable, and the 
 *         user starts, continues, or stops squeezing the trigger.</td>
 *       <td>A light switch that can be toggled on and off from a distance.</td>
 *     </tr>
 *     <tr>
 *       <td><code>startNearTrigger</code><br /><code>continueNearTrigger</code><br /><code>stopNearTrigger</code></td>
 *       <td>These methods are called when a user is less than 0.3m away from the entity, the entity is triggerable, and the 
 *         user starts, continues, or stops squeezing the trigger.</td>
 *       <td>A doorbell that can be rung when a user is near.</td>
 *     </tr>
 *     <tr>
 *       <td><code>startDistanceGrab</code><br /><code>continueDistanceGrab</code><br /></td>
 *       <td>These methods are called when a user is more than 0.3m away from the entity, the entity is either cloneable, or
 *         grabbable and not locked, and the user starts or continues to squeeze the trigger.</td>
 *       <td>A comet that emits icy particle trails when a user is dragging it through the sky.</td>
 *     </tr>
 *     <tr>
 *       <td><code>startNearGrab</code><br /><code>continueNearGrab</code><br /></td>
 *       <td>These methods are called when a user is less than 0.3m away from the entity, the entity is either cloneable, or 
 *         grabbable and not locked, and the user starts or continues to squeeze the trigger.</td>
 *       <td>A ball that glows when it's being held close.</td>
 *     </tr>
 *     <tr>
 *       <td><code>releaseGrab</code></td>
 *       <td>This method is called when a user releases the trigger when having been either distance or near grabbing an 
 *         entity.</td>
 *       <td>Turn off the ball glow or comet trail with the user finishes grabbing it.</td>
 *     </tr>
 *     <tr>
 *       <td><code>startEquip</code><br /><code>continueEquip</code><br /><code>releaseEquip</code></td>
 *       <td>These methods are called when a user starts, continues, or stops equipping an entity.</td>
 *       <td>A glass that stays in the user's hand after the trigger is clicked.</td>
 *     </tr>
 *   </tbody>
 * </table>
 * <p>All the entity methods are called with the following two arguments:</p>
 * <ul>
 *   <li>The entity ID.</li>
 *   <li>A string, <code>"hand,userID"</code> &mdash; where "hand" is <code>"left"</code> or <code>"right"</code>, and "userID"
 *     is the user's {@link MyAvatar|MyAvatar.sessionUUID}.</li>
 * </ul>
 *
 * @namespace Controller
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-avatar
 *
 * @property {Controller.Actions} Actions - Predefined actions on Interface and the user's avatar. These can be used as end
 *     points in a {@link RouteObject} mapping. A synonym for <code>Controller.Hardware.Actions</code>.
 *     <em>Read-only.</em>
 *     <p>Default mappings are provided from the <code>Controller.Hardware.Keyboard</code> and <code>Controller.Standard</code> 
 *     to actions in 
 *     <a href="https://github.com/highfidelity/hifi/blob/master/interface/resources/controllers/keyboardMouse.json">
 *     keyboardMouse.json</a> and 
 *     <a href="https://github.com/highfidelity/hifi/blob/master/interface/resources/controllers/standard.json">
 *     standard.json</a>, respectively.</p>
 *
 * @property {Controller.Hardware} Hardware - Standard and hardware-specific controller and computer outputs, plus predefined 
 *     actions on Interface and the user's avatar. The outputs can be mapped to <code>Actions</code> or functions in a 
 *     {@link RouteObject} mapping. Additionally, hardware-specific controller outputs can be mapped to 
 *     <code>Controller.Standard</code> controller outputs. <em>Read-only.</em>
 *
 * @property {Controller.Standard} Standard - Standard controller outputs that can be mapped to <code>Actions</code> or 
 *     functions in a {@link RouteObject} mapping. <em>Read-only.</em>
 *     <p>Each hardware device has a mapping from its outputs to <code>Controller.Standard</code> items, specified in a JSON file. 
 *     For example, <a href="https://github.com/highfidelity/hifi/blob/master/interface/resources/controllers/leapmotion.json">
 *     leapmotion.json</a> and 
 *     <a href="https://github.com/highfidelity/hifi/blob/master/interface/resources/controllers/vive.json">vive.json</a>.</p>
 */

/// handles scripting of input controller commands from JS
class ControllerScriptingInterface : public controller::ScriptingInterface {
    Q_OBJECT

public:    
    virtual ~ControllerScriptingInterface() {}

    void emitKeyPressEvent(QKeyEvent* event);
    void emitKeyReleaseEvent(QKeyEvent* event);
    
    void emitMouseMoveEvent(QMouseEvent* event);
    void emitMousePressEvent(QMouseEvent* event); 
    void emitMouseDoublePressEvent(QMouseEvent* event);
    void emitMouseReleaseEvent(QMouseEvent* event);

    void emitTouchBeginEvent(const TouchEvent& event);
    void emitTouchEndEvent(const TouchEvent& event); 
    void emitTouchUpdateEvent(const TouchEvent& event);
    
    void emitWheelEvent(QWheelEvent* event);

    bool isKeyCaptured(QKeyEvent* event) const;
    bool isKeyCaptured(const KeyEvent& event) const;
    bool isJoystickCaptured(int joystickIndex) const;
    bool areEntityClicksCaptured() const;

public slots:

    /*@jsdoc
     * Disables default Interface actions for a particular key event.
     * @function Controller.captureKeyEvents
     * @param {KeyEvent} event - Details of the key event to be captured. The <code>key</code> property must be specified. The 
     *     <code>text</code> property is ignored. The other properties default to <code>false</code>.
     * @example <caption>Disable left and right strafing.</caption>
     * var STRAFE_LEFT = { "key": 16777234, isShifted: true };
     * var STRAFE_RIGHT = { "key": 16777236, isShifted: true };
     *
     * Controller.captureKeyEvents(STRAFE_LEFT);
     * Controller.captureKeyEvents(STRAFE_RIGHT);
     *
     * Script.scriptEnding.connect(function () {
     *     Controller.releaseKeyEvents(STRAFE_LEFT);
     *     Controller.releaseKeyEvents(STRAFE_RIGHT);
     * });
     */
    virtual void captureKeyEvents(const KeyEvent& event);

    /*@jsdoc
     * Re-enables default Interface actions for a particular key event that has been disabled using 
     * {@link Controller.captureKeyEvents|captureKeyEvents}.
     * @function Controller.releaseKeyEvents
     * @param {KeyEvent} event - Details of the key event to release from capture. The <code>key</code> property must be 
     *     specified. The <code>text</code> property is ignored. The other properties default to <code>false</code>.
     */
    virtual void releaseKeyEvents(const KeyEvent& event);

    /*@jsdoc
     * Disables default Interface actions for a joystick.
     * @function Controller.captureJoystick
     * @param {number} joystickID - The integer ID of the joystick.
     * @deprecated This function is deprecated and will be removed. It no longer has any effect.
     */
    virtual void captureJoystick(int joystickIndex);

    /*@jsdoc
     * Re-enables default Interface actions for a joystick that has been disabled using 
     * {@link Controller.captureJoystick|captureJoystick}.
     * @function Controller.releaseJoystick
     * @param {number} joystickID - The integer ID of the joystick.
     * @deprecated This function is deprecated and will be removed. It no longer has any effect.
     */
    virtual void releaseJoystick(int joystickIndex);

    /*@jsdoc
     * Disables {@link Entities.mousePressOnEntity} and {@link Entities.mouseDoublePressOnEntity} events on entities.
     * @function Controller.captureEntityClickEvents
     * @example <caption>Disable entity click events for a short period.</caption>
     * Entities.mousePressOnEntity.connect(function (entityID, event) {
     *     print("Clicked on entity: " + entityID);
     * });
     *
     * Script.setTimeout(function () {
     *     Controller.captureEntityClickEvents();
     * }, 5000);
     *
     * Script.setTimeout(function () {
     *     Controller.releaseEntityClickEvents();
     * }, 10000);
     */
    virtual void captureEntityClickEvents();

    /*@jsdoc
     * Re-enables {@link Entities.mousePressOnEntity} and {@link Entities.mouseDoublePressOnEntity} events on entities that were 
     * disabled using {@link Controller.captureEntityClickEvents|captureEntityClickEvents}.
     * @function Controller.releaseEntityClickEvents
     */
    virtual void releaseEntityClickEvents();


    /*@jsdoc
     * Gets the dimensions of the Interface window's interior if in desktop mode or the HUD surface if in HMD mode.
     * @function Controller.getViewportDimensions
     * @returns {Vec2} The dimensions of the Interface window interior if in desktop mode or HUD surface if in HMD mode.
     */
    virtual glm::vec2 getViewportDimensions() const;

    /*@jsdoc
     * Gets the recommended area to position UI on the HUD surface if in HMD mode or Interface's window interior if in desktop 
     * mode.
     * @function Controller.getRecommendedHUDRect
     * @returns {Rect} The recommended area in which to position UI.
     */
    virtual QVariant getRecommendedHUDRect() const;

    /*@jsdoc
     * Enables or disables the virtual game pad that is displayed on certain devices (e.g., Android).
     * @function Controller.setVPadEnabled
     * @param {boolean} enable - If <code>true</code> then the virtual game pad doesn't work, otherwise it does work provided 
     *     that it is not hidden by {@link Controller.setVPadHidden|setVPadHidden}.
     *    
     */
    virtual void setVPadEnabled(bool enable);

    /*@jsdoc
     * Shows or hides the virtual game pad that is displayed on certain devices (e.g., Android).
     * @function Controller.setVPadHidden
     * @param {boolean} hidden - If <code>true</code> then the virtual game pad is hidden, otherwise it is shown.
     */
    virtual void setVPadHidden(bool hidden); // Call it when a window should hide it

    /*@jsdoc
     * Sets the amount of extra margin between the virtual game pad that is displayed on certain devices (e.g., Android) and 
     * the bottom of the display.
     * @function Controller.setVPadExtraBottomMargin
     * @param {number} margin - Integer number of pixels in the extra margin.
     */
    virtual void setVPadExtraBottomMargin(int margin);

signals:
    /*@jsdoc
     * Triggered when a keyboard key is pressed.
     * @function Controller.keyPressEvent
     * @param {KeyEvent} event - Details of the key press.
     * @returns {Signal}
     * @example <caption>Report the KeyEvent details for each key press.</caption>
     * Controller.keyPressEvent.connect(function (event) {
     *     print(JSON.stringify(event));
     * });
     */
    void keyPressEvent(const KeyEvent& event);

    /*@jsdoc
     * Triggered when a keyboard key is released from being pressed.
     * @function Controller.keyReleaseEvent
     * @param {KeyEvent} event - Details of the key release.
     * @returns {Signal}
     */
    void keyReleaseEvent(const KeyEvent& event);

    /*@jsdoc
     * Triggered when the mouse moves.
     * @function Controller.mouseMoveEvent
     * @param {MouseEvent} event - Details of the mouse movement.
     * @returns {Signal}
     * @example <caption>Report the MouseEvent details for each mouse move.</caption>
     * Controller.mouseMoveEvent.connect(function (event) {
     *     print(JSON.stringify(event));
     * });
     */
    void mouseMoveEvent(const MouseEvent& event);

    /*@jsdoc
     * Triggered when a mouse button is pressed.
     * @function Controller.mousePressEvent
     * @param {MouseEvent} event - Details of the button press.
     * @returns {Signal}
     */
    void mousePressEvent(const MouseEvent& event);

    /*@jsdoc
     * Triggered when a mouse button is double-pressed.
     * @function Controller.mouseDoublePressEvent
     * @param {MouseEvent} event - Details of the button double-press.
     * @returns {Signal}
     */
    void mouseDoublePressEvent(const MouseEvent& event);

    /*@jsdoc
     * Triggered when a mouse button is released from being pressed.
     * @function Controller.mouseReleaseEvent
     * @param {MouseEvent} event - Details of the button release.
     * @returns {Signal}
     */
    void mouseReleaseEvent(const MouseEvent& event);

    /*@jsdoc
     * Triggered when a touch event starts in the Interface window on a touch-enabled display or device.
     * @function Controller.touchBeginEvent
     * @param {TouchEvent} event - Details of the touch begin.
     * @returns {Signal}
     * @example <caption>Report the TouchEvent details when a touch event starts.</caption>
     * Controller.touchBeginEvent.connect(function (event) {
     *     print(JSON.stringify(event));
     * });
     */
    void touchBeginEvent(const TouchEvent& event);

    /*@jsdoc
     * Triggered when a touch event ends in the Interface window on a touch-enabled display or device.
     * @function Controller.touchEndEvent
     * @param {TouchEvent} event - Details of the touch end.
     * @returns {Signal}
     */
    void touchEndEvent(const TouchEvent& event);

    /*@jsdoc
     * Triggered when a touch event update occurs in the Interface window on a touch-enabled display or device.
     * @function Controller.touchUpdateEvent
     * @param {TouchEvent} event - Details of the touch update.
     * @returns {Signal}
     */
    void touchUpdateEvent(const TouchEvent& event);

    /*@jsdoc
     * Triggered when the mouse wheel is rotated.
     * @function Controller.wheelEvent
     * @param {WheelEvent} event - Details of the wheel movement.
     * @returns {Signal}
     * @example <caption>Report the WheelEvent details for each wheel rotation.</caption>
     * Controller.wheelEvent.connect(function (event) {
     *     print(JSON.stringify(event));
     * });
     */
    void wheelEvent(const WheelEvent& event);

private:
    QMultiMap<int,KeyEvent> _capturedKeys;
    QSet<int> _capturedJoysticks;
    bool _captureEntityClicks;

    using InputKey = controller::InputController::Key;
};

const int NUMBER_OF_SPATIALCONTROLS_PER_PALM = 2; // the hand and the tip
const int NUMBER_OF_JOYSTICKS_PER_PALM = 1;
const int NUMBER_OF_TRIGGERS_PER_PALM = 1;
const int NUMBER_OF_BUTTONS_PER_PALM = 6;
const int PALM_SPATIALCONTROL = 0;
const int TIP_SPATIALCONTROL = 1;


#endif // hifi_ControllerScriptingInterface_h
