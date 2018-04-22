undefined/**
     * @namespace
     * @augments Camera
     */
/**
     * @property cameraEntity {Uuid} The ID of the entity that the camera position and orientation follow when the camera is in
     *     entity mode.
     */
/**
   * Get the ID of the entity that the camera is set to use the position and orientation from when it's in entity mode. You can
   *     also get the entity ID using the <code>Camera.cameraEntity</code> property.
   * @function Camera.getCameraEntity
   * @returns {Uuid} The ID of the entity that the camera is set to follow when in entity mode; <code>null</code> if no camera
   *     entity has been set.
   */
/**
    * Set the entity that the camera should use the position and orientation from when it's in entity mode. You can also set the
    *     entity using the <code>Camera.cameraEntity</code> property.
    * @function Camera.setCameraEntity
    * @param {Uuid} entityID The entity that the camera should follow when it's in entity mode.
    * @example <caption>Move your camera to the position and orientation of the closest entity.</caption>
    * Camera.setModeString("entity");
    * var entity = Entities.findClosestEntity(MyAvatar.position, 100.0);
    * Camera.setCameraEntity(entity);
    */
/**
 * The <code>"far-grab"</code> {@link Entities.ActionType|ActionType} moves and rotates an entity to a target position and 
 * orientation, optionally relative to another entity. Collisions between the entity and the user's avatar are disabled during 
 * the far-grab.
 * It has arguments in addition to the common {@link Entities.ActionArguments|ActionArguments}.
 *
 * @typedef {object} Entities.ActionArguments-FarGrab
 * @property {Vec3} targetPosition=0,0,0 - The target position.
 * @property {Quat} targetRotation=0,0,0,1 - The target rotation.
 * @property {Uuid} otherID=null - If an entity ID, the <code>targetPosition</code> and <code>targetRotation</code> are 
 *     relative to this entity's position and rotation.
 * @property {number} linearTimeScale=3.4e+38 - Controls how long it takes for the entity's position to catch up with the
 *     target position. The value is the time for the action to catch up to 1/e = 0.368 of the target value, where the action 
 *     is applied using an exponential decay.
 * @property {number} angularTimeScale=3.4e+38 - Controls how long it takes for the entity's orientation to catch up with the
 *     target orientation. The value is the time for the action to catch up to 1/e = 0.368 of the target value, where the 
 *     action is applied using an exponential decay.
 */
/**
 * The <code>"hold"</code> {@link Entities.ActionType|ActionType} positions and rotates an entity relative to an avatar's hand. 
 * Collisions between the entity and the user's avatar are disabled during the hold.
 * It has arguments in addition to the common {@link Entities.ActionArguments|ActionArguments}.
 *
 * @typedef {object} Entities.ActionArguments-Hold
 * @property {Uuid} holderID=MyAvatar.sessionUUID - The ID of the avatar holding the entity.
 * @property {Vec3} relativePosition=0,0,0 - The target position relative to the avatar's hand.
 * @property {Vec3} relativeRotation=0,0,0,1 - The target rotation relative to the avatar's hand.
 * @property {number} timeScale=3.4e+38 - Controls how long it takes for the entity's position and rotation to catch up with 
 *     the target. The value is the time for the action to catch up to 1/e = 0.368 of the target value, where the action is 
 *     applied using an exponential decay.
 * @property {string} hand=right - The hand holding the entity: <code>"left"</code> or <code>"right"</code>.
 * @property {boolean} kinematic=false - If <code>true</code>, the entity is made kinematic during the action; the entity won't 
 *    lag behind the hand but constraint actions such as <code>"hinge"</code> won't act properly.
 * @property {boolean} kinematicSetVelocity=false - If <code>true</code> and <code>kinematic</code> is <code>true</code>, the 
 *    entity's <code>velocity</code> property will be set during the action, e.g., so that other scripts may use the value.
 * @property {boolean} ignoreIK=false - If <code>true</code>, the entity follows the HMD controller rather than the avatar's 
 *    hand.
 */
/**
     * Your avatar is your in-world representation of you. The MyAvatar API is used to manipulate the avatar.
     * For example, using the MyAvatar API you can customize the avatar's appearance, run custom avatar animations,
     * change the avatar's position within the domain, or manage the avatar's collisions with other objects.
     * NOTE: MyAvatar extends Avatar and AvatarData, see those namespace for more properties/methods.
     *
     * @namespace MyAvatar
     * @augments Avatar
     * @property qmlPosition {Vec3} Used as a stopgap for position access by QML, as glm::vec3 is unavailable outside of scripts
     * @property shouldRenderLocally {bool} Set it to true if you would like to see MyAvatar in your local interface,
     *   and false if you would not like to see MyAvatar in your local interface.
     * @property motorVelocity {Vec3} Can be used to move the avatar with this velocity.
     * @property motorTimescale {float} Specifies how quickly the avatar should accelerate to meet the motorVelocity,
     *   smaller values will result in higher acceleration.
     * @property motorReferenceFrame {string} Reference frame of the motorVelocity, must be one of the following: "avatar", "camera", "world"
     * @property motorMode {string} Type of scripted motor behavior, "simple" = use motorTimescale property (default mode) and "dynamic" = use action motor's timescales
     * @property collisionSoundURL {string} Specifies the sound to play when the avatar experiences a collision.
     *   You can provide a mono or stereo 16-bit WAV file running at either 24 Khz or 48 Khz.
     *   The latter is downsampled by the audio mixer, so all audio effectively plays back at a 24 Khz sample rate.
     *   48 Khz RAW files are also supported.
     * @property audioListenerMode {number} When hearing spatialized audio this determines where the listener placed.
     *   Should be one of the following values:
     *   MyAvatar.audioListenerModeHead - the listener located at the avatar's head.
     *   MyAvatar.audioListenerModeCamera - the listener is relative to the camera.
     *   MyAvatar.audioListenerModeCustom - the listener is at a custom location specified by the MyAvatar.customListenPosition
     *   and MyAvatar.customListenOrientation properties.
     * @property customListenPosition {Vec3} If MyAvatar.audioListenerMode == MyAvatar.audioListenerModeHead, then this determines the position
     *   of audio spatialization listener.
     * @property customListenOrientation {Quat} If MyAvatar.audioListenerMode == MyAvatar.audioListenerModeHead, then this determines the orientation
     *   of the audio spatialization listener.
     * @property audioListenerModeHead {number} READ-ONLY. When passed to MyAvatar.audioListenerMode, it will set the audio listener
     *   around the avatar's head.
     * @property audioListenerModeCamera {number} READ-ONLY. When passed to MyAvatar.audioListenerMode, it will set the audio listener
     *   around the camera.
     * @property audioListenerModeCustom {number} READ-ONLY. When passed to MyAvatar.audioListenerMode, it will set the audio listener
     *  around the value specified by MyAvatar.customListenPosition and MyAvatar.customListenOrientation.
     * @property leftHandPosition {Vec3} READ-ONLY. The desired position of the left wrist in avatar space, determined by the hand controllers.
     *   Note: only valid if hand controllers are in use.
     * @property rightHandPosition {Vec3} READ-ONLY. The desired position of the right wrist in avatar space, determined by the hand controllers.
     *   Note: only valid if hand controllers are in use.
     * @property leftHandTipPosition {Vec3} READ-ONLY. A position 30 cm offset from MyAvatar.leftHandPosition
     * @property rightHandTipPosition {Vec3} READ-ONLY. A position 30 cm offset from MyAvatar.rightHandPosition
     * @property leftHandPose {Pose} READ-ONLY. Returns full pose (translation, orientation, velocity & angularVelocity) of the desired
     *   wrist position, determined by the hand controllers.
     * @property rightHandPose {Pose} READ-ONLY. Returns full pose (translation, orientation, velocity & angularVelocity) of the desired
     *   wrist position, determined by the hand controllers.
     * @property leftHandTipPose {Pose} READ-ONLY. Returns a pose offset 30 cm from MyAvatar.leftHandPose
     * @property rightHandTipPose {Pose} READ-ONLY. Returns a pose offset 30 cm from MyAvatar.rightHandPose
     * @property hmdLeanRecenterEnabled {bool} This can be used disable the hmd lean recenter behavior.  This behavior is what causes your avatar
     *   to follow your HMD as you walk around the room, in room scale VR.  Disabling this is useful if you desire to pin the avatar to a fixed location.
     * @property collisionsEnabled {bool} This can be used to disable collisions between the avatar and the world.
     * @property useAdvancedMovementControls {bool} Stores the user preference only, does not change user mappings, this is done in the defaultScript
     *   "scripts/system/controllers/toggleAdvancedMovementForHandControllers.js".
     * @property userHeight {number} The height of the user in sensor space. (meters).
     * @property userEyeHeight {number} Estimated height of the users eyes in sensor space. (meters)
     * @property SELF_ID {string} READ-ONLY. UUID representing "my avatar". Only use for local-only entities and overlays in situations where MyAvatar.sessionUUID is not available (e.g., if not connected to a domain).
     *   Note: Likely to be deprecated.
     * @property hmdRollControlEnabled {bool} When enabled the roll angle of your HMD will turn your avatar while flying.
     * @property hmdRollControlDeadZone {number} If hmdRollControlEnabled is true, this value can be used to tune what roll angle is required to begin turning.
     *   This angle is specified in degrees.
     * @property hmdRollControlRate {number} If hmdRollControlEnabled is true, this value determines the maximum turn rate of your avatar when rolling your HMD in degrees per second.
     */
/**
     * Moves and orients the avatar, such that it is directly underneath the HMD, with toes pointed forward.
     * @function MyAvatar.centerBody
     */
/**
     * The internal inverse-kinematics system maintains a record of which joints are "locked". Sometimes it is useful to forget this history, to prevent
     * contorted joints.
     * @function MyAvatar.clearIKJointLimitHistory
     */
/**
     * The default position in world coordinates of the point directly between the avatar's eyes
     * @function MyAvatar.getDefaultEyePosition
     * @example <caption>This example gets the default eye position and prints it to the debug log.</caption>
     * var defaultEyePosition = MyAvatar.getDefaultEyePosition();
     * print (JSON.stringify(defaultEyePosition));
     * @returns {Vec3} Position between the avatar's eyes.
     */
/**
     * The avatar animation system includes a set of default animations along with rules for how those animations are blended
     * together with procedural data (such as look at vectors, hand sensors etc.). overrideAnimation() is used to completely
     * override all motion from the default animation system (including inverse kinematics for hand and head controllers) and
     * play a specified animation.  To end this animation and restore the default animations, use MyAvatar.restoreAnimation.
     * @function MyAvatar.overrideAnimation
     * @example <caption> Play a clapping animation on your avatar for three seconds. </caption>
     * // Clap your hands for 3 seconds then restore animation back to the avatar.
     * var ANIM_URL = "https://s3.amazonaws.com/hifi-public/animations/ClapAnimations/ClapHands_Standing.fbx";
     * MyAvatar.overrideAnimation(ANIM_URL, 30, true, 0, 53);
     * Script.setTimeout(function () {
     *     MyAvatar.restoreAnimation();
     * }, 3000);
     * @param url {string} The URL to the animation file. Animation files need to be .FBX format, but only need to contain the avatar skeleton and animation data.
     * @param fps {number} The frames per second (FPS) rate for the animation playback. 30 FPS is normal speed.
     * @param loop {bool} Set to true if the animation should loop.
     * @param firstFrame {number} The frame the animation should start at.
     * @param lastFrame {number} The frame the animation should end at.
     */
/**
     * The avatar animation system includes a set of default animations along with rules for how those animations are blended together with
     * procedural data (such as look at vectors, hand sensors etc.). Playing your own custom animations will override the default animations.
     * restoreAnimation() is used to restore all motion from the default animation system including inverse kinematics for hand and head
     * controllers. If you aren't currently playing an override animation, this function will have no effect.
     * @function MyAvatar.restoreAnimation
     * @example <caption> Play a clapping animation on your avatar for three seconds. </caption>
     * // Clap your hands for 3 seconds then restore animation back to the avatar.
     * var ANIM_URL = "https://s3.amazonaws.com/hifi-public/animations/ClapAnimations/ClapHands_Standing.fbx";
     * MyAvatar.overrideAnimation(ANIM_URL, 30, true, 0, 53);
     * Script.setTimeout(function () {
     *     MyAvatar.restoreAnimation();
     * }, 3000);
     */
/**
     * Each avatar has an avatar-animation.json file that defines which animations are used and how they are blended together with procedural data
     * (such as look at vectors, hand sensors etc.). Each animation specified in the avatar-animation.json file is known as an animation role.
     * Animation roles map to easily understandable actions that the avatar can perform, such as "idleStand", "idleTalk", or "walkFwd."
     * getAnimationRoles() is used get the list of animation roles defined in the avatar-animation.json.
     * @function MyAvatar.getAnimatationRoles
     * @example <caption>This example prints the list of animation roles defined in the avatar's avatar-animation.json file to the debug log.</caption>
     * var roles = MyAvatar.getAnimationRoles();
     * print("Animation Roles:");
     * for (var i = 0; i < roles.length; i++) {
     *     print(roles[i]);
     * }
     * @returns {string[]} Array of role strings
     */
/**
     * Each avatar has an avatar-animation.json file that defines a set of animation roles. Animation roles map to easily understandable actions
     * that the avatar can perform, such as "idleStand", "idleTalk", or "walkFwd". To get the full list of roles, use getAnimationRoles().
     * For each role, the avatar-animation.json defines when the animation is used, the animation clip (.FBX) used, and how animations are blended
     * together with procedural data (such as look at vectors, hand sensors etc.).
     * overrideRoleAnimation() is used to change the animation clip (.FBX) associated with a specified animation role.
     * Note: Hand roles only affect the hand. Other 'main' roles, like 'idleStand', 'idleTalk', 'takeoffStand' are full body.
     * @function MyAvatar.overrideRoleAnimation
     * @example <caption>The default avatar-animation.json defines an "idleStand" animation role. This role specifies that when the avatar is not moving,
     * an animation clip of the avatar idling with hands hanging at its side will be used. It also specifies that when the avatar moves, the animation
     * will smoothly blend to the walking animation used by the "walkFwd" animation role.
     * In this example, the "idleStand" role animation clip has been replaced with a clapping animation clip. Now instead of standing with its arms
     * hanging at its sides when it is not moving, the avatar will stand and clap its hands. Note that just as it did before, as soon as the avatar
     * starts to move, the animation will smoothly blend into the walk animation used by the "walkFwd" animation role.</caption>
     * // An animation of the avatar clapping its hands while standing
     * var ANIM_URL = "https://s3.amazonaws.com/hifi-public/animations/ClapAnimations/ClapHands_Standing.fbx";
     * MyAvatar.overrideRoleAnimation("idleStand", ANIM_URL, 30, true, 0, 53);
     * // To restore the default animation, use MyAvatar.restoreRoleAnimation().
     * @param role {string} The animation role to override
     * @param url {string} The URL to the animation file. Animation files need to be .FBX format, but only need to contain the avatar skeleton and animation data.
     * @param fps {number} The frames per second (FPS) rate for the animation playback. 30 FPS is normal speed.
     * @param loop {bool} Set to true if the animation should loop
     * @param firstFrame {number} The frame the animation should start at
     * @param lastFrame {number} The frame the animation should end at
     */
/**
     * Each avatar has an avatar-animation.json file that defines a set of animation roles. Animation roles map to easily understandable actions that
     * the avatar can perform, such as "idleStand", "idleTalk", or "walkFwd". To get the full list of roles, use getAnimationRoles(). For each role,
     * the avatar-animation.json defines when the animation is used, the animation clip (.FBX) used, and how animations are blended together with
     * procedural data (such as look at vectors, hand sensors etc.). You can change the animation clip (.FBX) associated with a specified animation
     * role using overrideRoleAnimation().
     * restoreRoleAnimation() is used to restore a specified animation role's default animation clip. If you have not specified an override animation
     * for the specified role, this function will have no effect.
     * @function MyAvatar.restoreRoleAnimation
     * @param role {string} The animation role clip to restore
     */
/**
    *The triggerVerticalRecenter function activates one time the recentering 
    *behaviour in the vertical direction. This call is only takes effect when the property
    *MyAvatar.hmdLeanRecenterEnabled is set to false.
    *@function MyAvatar.triggerVerticalRecenter
    *
    */
/**
    *The triggerHorizontalRecenter function activates one time the recentering behaviour
    *in the horizontal direction. This call is only takes effect when the property
    *MyAvatar.hmdLeanRecenterEnabled is set to false.
    *@function MyAvatar.triggerHorizontalRecenter
    */
/**
    *The triggerRotationRecenter function activates one time the recentering behaviour
    *in the rotation of the root of the avatar. This call is only takes effect when the property
    *MyAvatar.hmdLeanRecenterEnabled is set to false.
    *@function MyAvatar.triggerRotationRecenter
    */
/**
 * The Clipboard API enables you to export and import entities to and from JSON files.
 *
 * @namespace Clipboard
 */
/**
     * Compute the extents of the contents held in the clipboard.
     * @function Clipboard.getContentsDimensions
     * @returns {Vec3} The extents of the contents held in the clipboard.
     */
/**
     * Compute the largest dimension of the extents of the contents held in the clipboard.
     * @function Clipboard.getClipboardContentsLargestDimension
     * @returns {number} The largest dimension computed.
     */
/**
     * Import entities from a JSON file containing entity data into the clipboard.
     * You can generate a JSON file using {@link Clipboard.exportEntities}.
     * @function Clipboard.importEntities
     * @param {string} filename Path and name of file to import.
     * @returns {boolean} <code>true</code> if the import was successful, otherwise <code>false</code>.
     */
/**
     * Export the entities specified to a JSON file.
     * @function Clipboard.exportEntities
     * @param {string} filename Path and name of the file to export the entities to. Should have the extension ".json".
     * @param {Uuid[]} entityIDs Array of IDs of the entities to export.
     * @returns {boolean} <code>true</code> if the export was successful, otherwise <code>false</code>.
     */
/**
    * Export the entities with centers within a cube to a JSON file.
    * @function Clipboard.exportEntities
    * @param {string} filename Path and name of the file to export the entities to. Should have the extension ".json".
    * @param {number} x X-coordinate of the cube center.
    * @param {number} y Y-coordinate of the cube center.
    * @param {number} z Z-coordinate of the cube center.
    * @param {number} scale Half dimension of the cube.
    * @returns {boolean} <code>true</code> if the export was successful, otherwise <code>false</code>.
    */
/**
     * Paste the contents of the clipboard into the world.
     * @function Clipboard.pasteEntities
     * @param {Vec3} position Position to paste the clipboard contents at.
     * @returns {Uuid[]} Array of entity IDs for the new entities that were created as a result of the paste operation.
     */
/**
 * The Menu API provides access to the menu that is displayed at the top of the window
 * on a user's desktop and in the tablet when the "MENU" button is pressed.
 *
 * <p />
 *
 * <h3>Groupings</h3>
 * 
 * A "grouping" provides a way to group a set of menus or menu items together so 
 * that they can all be set visible or invisible as a group. 
 * There are two available groups: <code>"Advanced"</code> and <code>"Developer"</code>.
 * These groupings can be toggled in the "Settings" menu.
 * If a menu item doesn't belong to a group it is always displayed.
 *
 * @namespace Menu
 */
/**
     * Add a new top-level menu.
     * @function Menu.addMenu
     * @param {string} menuName - Name that will be displayed for the menu. Nested menus can be described using the ">" symbol.
     * @param {string} [grouping] - Name of the grouping, if any, to add this menu to.
     *
     * @example <caption>Add a menu and a nested submenu.</caption>
     * Menu.addMenu("Test Menu");
     * Menu.addMenu("Test Menu > Test Sub Menu");
     *
     * @example <caption>Add a menu to the Settings menu that is only visible if Settings > Advanced is enabled.</caption>
     * Menu.addMenu("Settings > Test Grouping Menu", "Advanced");
     */
/**
     * Remove a top-level menu.
     * @function Menu.removeMenu
     * @param {string} menuName - Name of the menu to remove.
     * @example <caption>Remove a menu and nested submenu.</caption>
     * Menu.removeMenu("Test Menu > Test Sub Menu");
     * Menu.removeMenu("Test Menu");
     */
/**
     * Check whether a top-level menu exists.
     * @function Menu.menuExists
     * @param {string} menuName - Name of the menu to check for existence.
     * @returns {boolean} <code>true</code> if the menu exists, otherwise <code>false</code>.
     * @example <caption>Check if the "Developer" menu exists.</caption>
     * if (Menu.menuExists("Developer")) {
     *     print("Developer menu exists.");
     * }
     */
/**
     * Add a separator with an unclickable label below it. The separator will be placed at the bottom of the menu.
     * If you want to add a separator at a specific point in the menu, use {@link Menu.addMenuItem} with
     * {@link Menu.MenuItemProperties} instead.
     * @function Menu.addSeparator
     * @param {string} menuName - Name of the menu to add a separator to.
     * @param {string} separatorName - Name of the separator that will be displayed as the label below the separator line.
     * @example <caption>Add a separator.</caption>
     * Menu.addSeparator("Developer","Test Separator");
     */
/**
     * Remove a separator from a menu.
     * @function Menu.removeSeparator
     * @param {string} menuName - Name of the menu to remove the separator from.
     * @param {string} separatorName - Name of the separator to remove.
     * @example <caption>Remove a separator.</caption>
     * Menu.removeSeparator("Developer","Test Separator");
     */
/**
     * Add a new menu item to a menu.
     * @function Menu.addMenuItem
     * @param {Menu.MenuItemProperties} properties - Properties of the menu item to create.
     * @example <caption>Add a menu item using {@link Menu.MenuItemProperties}.</caption>
     * Menu.addMenuItem({
     *     menuName:     "Developer",
     *     menuItemName: "Test",
     *     afterItem:    "Log",
     *     shortcutKey:  "Ctrl+Shift+T",
     *     grouping:     "Advanced"
     * });
     */
/**
     * Add a new menu item to a menu. The new item is added at the end of the menu.
     * @function Menu.addMenuItem
     * @param {string} menuName - Name of the menu to add a menu item to.
     * @param {string} menuItem - Name of the menu item. This is what will be displayed in the menu.
     * @param {string} [shortcutKey] A shortcut key that can be used to trigger the menu item.
     * @example <caption>Add a menu item to the end of the "Developer" menu.</caption>
     * Menu.addMenuItem("Developer", "Test", "Ctrl+Shift+T");
     */
/**
     * Remove a menu item from a menu.
     * @function Menu.removeMenuItem
     * @param {string} menuName - Name of the menu to remove a menu item from.
     * @param {string} menuItem - Name of the menu item to remove.
     * Menu.removeMenuItem("Developer", "Test");
     */
/**
     * Check if a menu item exists.
     * @function Menu.menuItemExists
     * @param {string} menuName - Name of the menu that the menu item is in.
     * @param {string} menuItem - Name of the menu item to check for existence of.
     * @returns {boolean} <code>true</code> if the menu item exists, otherwise <code>false</code>.
     * @example <caption>Determine if the Developer &gt; Stats menu exists.</caption>
     * if (Menu.menuItemExists("Developer", "Stats")) {
     *     print("Developer > Stats menu item exists.");
     * }
     */
/**
     * Check whether a checkable menu item is checked.
     * @function Menu.isOptionChecked
     * @param {string} menuOption - The name of the menu item.
     * @returns {boolean} <code>true</code> if the option is checked, otherwise <code>false</code>.
     * @example <caption>Report whether the Settings > Advanced menu item is turned on.</caption>
     * print(Menu.isOptionChecked("Advanced Menus")); // true or false
     */
/**
     * Set a checkable menu item as checked or unchecked.
     * @function Menu.setIsOptionChecked
     * @param {string} menuOption - The name of the menu item to modify.
     * @param {boolean} isChecked - If <code>true</code>, the menu item will be checked, otherwise it will not be checked.
     * @example <caption>Turn on Settings > Advanced Menus.</caption>
     * Menu.setIsOptionChecked("Advanced Menus", true);
     * print(Menu.isOptionChecked("Advanced Menus")); // true
     */
/**
     * Trigger the menu item as if the user clicked on it.
     * @function Menu.triggerOption
     * @param {string} menuOption - The name of the menu item to trigger.
     * @example <caption>Open the help window.</caption>
     * Menu.triggerOption('Help...');
     */
/**
     * Check whether a menu or menu item is enabled. If disabled, the item is grayed out and unusable.
     * Menus are enabled by default.
     * @function Menu.isMenuEnabled
     * @param {string} menuName The name of the menu or menu item to check.
     * @returns {boolean} <code>true</code> if the menu is enabled, otherwise <code>false</code>.
     * @example <caption>Report with the Settings > Advanced Menus menu item is enabled.</caption>
     * print(Menu.isMenuEnabled("Settings > Advanced Menus")); // true or false
     */
/**
     * Set a menu or menu item to be enabled or disabled. If disabled, the item is grayed out and unusable.
     * @function Menu.setMenuEnabled
     * @param {string} menuName - The name of the menu or menu item to modify.
     * @param {boolean} isEnabled - If <code>true</code>, the menu will be enabled, otherwise it will be disabled.
     * @example <caption>Disable the Settings > Advanced Menus menu item.</caption>
     * Menu.setMenuEnabled("Settings > Advanced Menus", false);
     * print(Menu.isMenuEnabled("Settings > Advanced Menus")); // false
     */
/**
     * Triggered when a menu item is clicked (or triggered by {@link Menu.triggerOption}).
     * @function Menu.menuItemEvent
     * @param {string} menuItem - Name of the menu item that was clicked.
     * @returns {Signal}
     * @example <caption>Detect menu item events.</caption>
     * function onMenuItemEvent(menuItem) {
     *     print("You clicked on " + menuItem);
     * }
     * 
     * Menu.menuItemEvent.connect(onMenuItemEvent);
     */
/**
    * Query the names of all the selection lists
    * @function Selection.getListNames
    * @return An array of names of all the selection lists
    */
/**
    * Removes a named selection from the list of selections.
    * @function Selection.removeListFromMap
    * @param listName {string} name of the selection
    * @returns {bool} true if the selection existed and was successfully removed.
    */
/**
    * Add an item in a selection.
    * @function Selection.addToSelectedItemsList
    * @param listName {string} name of the selection
    * @param itemType {string} the type of the item (one of "avatar", "entity" or "overlay")
    * @param id {EntityID} the Id of the item to add to the selection
    * @returns {bool} true if the item was successfully added.
    */
/**
    * Remove an item from a selection.
    * @function Selection.removeFromSelectedItemsList
    * @param listName {string} name of the selection
    * @param itemType {string} the type of the item (one of "avatar", "entity" or "overlay")
    * @param id {EntityID} the Id of the item to remove
    * @returns {bool} true if the item was successfully removed.
    */
/**
    * Remove all items from a selection.
    * @function Selection.clearSelectedItemsList
    * @param listName {string} name of the selection
    * @returns {bool} true if the item was successfully cleared.
    */
/**
    * Prints out the list of avatars, entities and overlays stored in a particular selection.
    * @function Selection.printList
    * @param listName {string} name of the selection
    */
/**
    * Query the list of avatars, entities and overlays stored in a particular selection.
    * @function Selection.getList
    * @param listName {string} name of the selection
    * @return a js object describing the content of a selection list with the following properties:
    *  - "entities": [ and array of the entityID of the entities in the selection]
    *  - "avatars": [ and array of the avatarID of the avatars in the selection]
    *  - "overlays": [ and array of the overlayID of the overlays in the selection]
    *  If the list name doesn't exist, the function returns an empty js object with no properties.
    */
/**
    * Query the names of the highlighted selection lists
    * @function Selection.getHighlightedListNames
    * @return An array of names of the selection list currently highlight enabled
    */
/**
    * Enable highlighting for the named selection.
    * If the Selection doesn't exist, it will be created.
    * All objects in the list will be displayed with the highlight effect as specified from the highlightStyle.
    * The function can be called several times with different values in the style to modify it.
    *
    * @function Selection.enableListHighlight
    * @param listName {string} name of the selection
    * @param highlightStyle {jsObject} highlight style fields (see Selection.getListHighlightStyle for a detailed description of the highlightStyle).
    * @returns {bool} true if the selection was successfully enabled for highlight.
    *
    * Note: This function will implicitly call Selection.enableListToScene
    */
/**
    * Disable highlighting for the named selection.
    * If the Selection doesn't exist or wasn't enabled for highliting then nothing happens simply returning false.
    *
    * @function Selection.disableListHighlight
    * @param listName {string} name of the selection
    * @returns {bool} true if the selection was successfully disabled for highlight, false otherwise.
    *
    * Note: This function will implicitly call Selection.disableListToScene
    */
/**
    * Enable scene selection for the named selection.
    * If the Selection doesn't exist, it will be created.
    * All objects in the list will be sent to a scene selection.
    *
    * @function Selection.enableListToScene
    * @param listName {string} name of the selection
    * @returns {bool} true if the selection was successfully enabled on the scene.
    */
/**
    * Disable scene selection for the named selection.
    * If the Selection doesn't exist or wasn't enabled on the scene then nothing happens simply returning false.
    *
    * @function Selection.disableListToScene
    * @param listName {string} name of the selection
    * @returns {bool} true if the selection was successfully disabled on the scene, false otherwise.
    */
/**
    * Query the highlight style values for the named selection.
    * If the Selection doesn't exist or hasn't been highlight enabled yet, it will return an empty object.
    * Otherwise, the jsObject describes the highlight style properties:
    * - outlineUnoccludedColor: {xColor} Color of the specified highlight region
    * - outlineOccludedColor: {xColor} "
    * - fillUnoccludedColor: {xColor} "
    * - fillOccludedColor: {xColor} "
    *
    * - outlineUnoccludedAlpha: {float} Alpha value ranging from 0.0 (not visible) to 1.0 (fully opaque) for the specified highlight region
    * - outlineOccludedAlpha: {float} "
    * - fillUnoccludedAlpha: {float} "
    * - fillOccludedAlpha: {float} "
    *
    * - outlineWidth: {float} width of the outline expressed in pixels
    * - isOutlineSmooth: {bool} true to enable oultine smooth falloff
    *
    * @function Selection.getListHighlightStyle
    * @param listName {string} name of the selection
    * @returns {jsObject} highlight style as described above
    */
/**
     * Exits the application
     */
/**
    * Waits for all texture transfers to be complete
    */
/**
    * Waits for all pending downloads to be complete
    */
/**
    * Waits for all file parsing operations to be complete
    */
/**
    * Waits for all pending downloads, parsing and texture transfers to be complete
    */
/**
    * Start recording Chrome compatible tracing events
    * logRules can be used to specify a set of logging category rules to limit what gets captured
    */
/**
    * Stop recording Chrome compatible tracing events and serialize recorded events to a file
    * Using a filename with a .gz extension will automatically compress the output file
    */
/**
     * Write detailed timing stats of next physics stepSimulation() to filename
     */
/**
 * <p>The buttons that may be included in a message box created by {@link Window.openMessageBox|openMessageBox} are defined by
 * numeric values:
 * <table>
 *   <thead>
 *     <tr>
 *       <th>Button</th>
 *       <th>Value</th>
 *       <th>Description</th>
 *     </tr>
 *   </thead>
 *   <tbody>
 *     <tr> <td><strong>NoButton</strong></td> <td><code>0x0</code></td> <td>An invalid button.</td> </tr>
 *     <tr> <td><strong>Ok</strong></td> <td><code>0x400</code></td> <td>"OK"</td> </tr>
 *     <tr> <td><strong>Save</strong></td> <td><code>0x800</code></td> <td>"Save"</td> </tr>
 *     <tr> <td><strong>SaveAll</strong></td> <td><code>0x1000</code></td> <td>"Save All"</td> </tr>
 *     <tr> <td><strong>Open</strong></td> <td><code>0x2000</code></td> <td>"Open"</td> </tr>
 *     <tr> <td><strong>Yes</strong></td> <td><code>0x4000</code></td> <td>"Yes"</td> </tr>
 *     <tr> <td><strong>YesToAll</strong></td> <td><code>0x8000</code></td> <td>"Yes to All"</td> </tr>
 *     <tr> <td><strong>No</strong></td> <td><code>0x10000</code></td> <td>"No"</td> </tr>
 *     <tr> <td><strong>NoToAll</strong></td> <td><code>0x20000</code></td> <td>"No to All"</td> </tr>
 *     <tr> <td><strong>Abort</strong></td> <td><code>0x40000</code></td> <td>"Abort"</td> </tr>
 *     <tr> <td><strong>Retry</strong></td> <td><code>0x80000</code></td> <td>"Retry"</td> </tr>
 *     <tr> <td><strong>Ignore</strong></td> <td><code>0x100000</code></td> <td>"Ignore"</td> </tr>
 *     <tr> <td><strong>Close</strong></td> <td><code>0x200000</code></td> <td>"Close"</td> </tr>
 *     <tr> <td><strong>Cancel</strong></td> <td><code>0x400000</code></td> <td>"Cancel"</td> </tr>
 *     <tr> <td><strong>Discard</strong></td> <td><code>0x800000</code></td> <td>"Discard" or "Don't Save"</td> </tr>
 *     <tr> <td><strong>Help</strong></td> <td><code>0x1000000</code></td> <td>"Help"</td> </tr>
 *     <tr> <td><strong>Apply</strong></td> <td><code>0x2000000</code></td> <td>"Apply"</td> </tr>
 *     <tr> <td><strong>Reset</strong></td> <td><code>0x4000000</code></td> <td>"Reset"</td> </tr>
 *     <tr> <td><strong>RestoreDefaults</strong></td> <td><code>0x8000000</code></td> <td>"Restore Defaults"</td> </tr>
 *   </tbody>
 * </table>
 * @typedef Window.MessageBoxButton
 */
/**
 * The Window API provides various facilities not covered elsewhere: window dimensions, window focus, normal or entity camera
 * view, clipboard, announcements, user connections, common dialog boxes, snapshots, file import, domain changes, domain 
 * physics.
 *
 * @namespace Window
 * @property {number} innerWidth - The width of the drawable area of the Interface window (i.e., without borders or other 
 *     chrome), in pixels. <em>Read-only.</em>
 * @property {number} innerHeight - The height of the drawable area of the Interface window (i.e., without borders or other
 *     chrome), in pixels. <em>Read-only.</em>
 * @property {object} location - Provides facilities for working with your current metaverse location. See {@link location}.
 * @property {number} x - The x coordinate of the top left corner of the Interface window on the display. <em>Read-only.</em>
 * @property {number} y - The y coordinate of the top left corner of the Interface window on the display. <em>Read-only.</em>
 */
/**
     * Check if the Interface window has focus.
     * @function Window.hasFocus
     * @returns {boolean} <code>true</code> if the Interface window has focus, otherwise <code>false</code>.
     */
/**
     * Make the Interface window have focus. On Windows, if Interface doesn't already have focus, the task bar icon flashes to 
     * indicate that Interface wants attention but focus isn't taken away from the application that the user is using.
     * @function Window.setFocus
     */
/**
     * Raise the Interface window if it is minimized. If raised, the window gains focus.
     * @function Window.raise
     */
/**
     * Raise the Interface window if it is minimized. If raised, the window gains focus.
     * @function Window.raiseMainWindow
     * @deprecated Use {@link Window.raise|raise} instead.
     */
/**
     * Display a dialog with the specified message and an "OK" button. The dialog is non-modal; the script continues without
     * waiting for a user response.
     * @function Window.alert
     * @param {string} message="" - The message to display.
     * @example <caption>Display a friendly greeting.</caption>
     * Window.alert("Welcome!");
     * print("Script continues without waiting");
     */
/**
     * Prompt the user to confirm something. Displays a modal dialog with a message plus "Yes" and "No" buttons.
     * responds.
     * @function Window.confirm
     * @param {string} message="" - The question to display.
     * @returns {boolean} <code>true</code> if the user selects "Yes", otherwise <code>false</code>.
     * @example <caption>Ask the user a question requiring a yes/no answer.</caption>
     * var answer = Window.confirm("Are you sure?");
     * print(answer);  // true or false
     */
/**
     * Prompt the user to enter some text. Displays a modal dialog with a message and a text box, plus "OK" and "Cancel" 
     * buttons.
     * @function Window.prompt
     * @param {string} message - The question to display.
     * @param {string} defaultText - The default answer text.
     * @returns {string} The text that the user entered if they select "OK", otherwise "".
     * @example <caption>Ask the user a question requiring a text answer.</caption>
     * var answer = Window.prompt("Question", "answer");
     * if (answer === "") {
     *     print("User canceled");
     * } else {
     *     print("User answer: " + answer);
     * }
     */
/**
     * Prompt the user to enter some text. Displays a non-modal dialog with a message and a text box, plus "OK" and "Cancel" 
     * buttons. A {@link Window.promptTextChanged|promptTextChanged} signal is emitted when the user OKs the dialog; no signal 
     * is emitted if the user cancels the dialog.
     * @function Window.promptAsync
     * @param {string} message - The question to display.
     * @param {string} defaultText - The default answer text.
     * @example <caption>Ask the user a question requiring a text answer without waiting for the answer.</caption>
     * function onPromptTextChanged(text) {
     *     print("User answer: " + text);
     * }
     * Window.promptTextChanged.connect(onPromptTextChanged);
     *
     * Window.promptAsync("Question", "answer");
     * print("Script continues without waiting");
     */
/**
     * Prompt the user to choose a directory. Displays a modal dialog that navigates the directory tree.
     * @function Window.browseDir
     * @param {string} title="" - The title to display at the top of the dialog.
     * @param {string} directory="" - The initial directory to start browsing at.
     * @returns {string} The path of the directory if one is chosen, otherwise <code>null</code>.
     * @example <caption>Ask the user to choose a directory.</caption>
     * var directory = Window.browseDir("Select Directory", Paths.resources);
     * print("Directory: " + directory);
     */
/**
     * Prompt the user to choose a directory. Displays a non-modal dialog that navigates the directory tree. A
     * {@link Window.browseDirChanged|browseDirChanged} signal is emitted when a directory is chosen; no signal is emitted if
     * the user cancels the dialog.
     * @function Window.browseDirAsync
     * @param {string} title="" - The title to display at the top of the dialog.
     * @param {string} directory="" - The initial directory to start browsing at.
     * @example <caption>Ask the user to choose a directory without waiting for the answer.</caption>
     * function onBrowseDirChanged(directory) {
     *     print("Directory: " + directory);
     * }
     * Window.browseDirChanged.connect(onBrowseDirChanged);
     *
     * Window.browseDirAsync("Select Directory", Paths.resources);
     * print("Script continues without waiting");
     */
/**
     * Prompt the user to choose a file. Displays a modal dialog that navigates the directory tree.
     * @function Window.browse
     * @param {string} title="" - The title to display at the top of the dialog.
     * @param {string} directory="" - The initial directory to start browsing at.
     * @param {string} nameFilter="" - The types of files to display. Examples: <code>"*.json"</code> and 
     *     <code>"Images (*.png *.jpg *.svg)"</code>. All files are displayed if a filter isn't specified.
     * @returns {string} The path and name of the file if one is chosen, otherwise <code>null</code>.
     * @example <caption>Ask the user to choose an image file.</caption>
     * var filename = Window.browse("Select Image File", Paths.resources, "Images (*.png *.jpg *.svg)");
     * print("File: " + filename);
     */
/**
     * Prompt the user to choose a file. Displays a non-modal dialog that navigates the directory tree. A
     * {@link Window.browseChanged|browseChanged} signal is emitted when a file is chosen; no signal is emitted if the user
     * cancels the dialog.
     * @function Window.browseAsync
     * @param {string} title="" - The title to display at the top of the dialog.
     * @param {string} directory="" - The initial directory to start browsing at.
     * @param {string} nameFilter="" - The types of files to display. Examples: <code>"*.json"</code> and
     *     <code>"Images (*.png *.jpg *.svg)"</code>. All files are displayed if a filter isn't specified.
     * @example <caption>Ask the user to choose an image file without waiting for the answer.</caption>
     * function onBrowseChanged(filename) {
     *     print("File: " + filename);
     * }
     * Window.browseChanged.connect(onBrowseChanged);
     *
     * Window.browseAsync("Select Image File", Paths.resources, "Images (*.png *.jpg *.svg)");
     * print("Script continues without waiting");
     */
/**
     * Prompt the user to specify the path and name of a file to save to. Displays a model dialog that navigates the directory
     * tree and allows the user to type in a file name.
     * @function Window.save
     * @param {string} title="" - The title to display at the top of the dialog.
     * @param {string} directory="" - The initial directory to start browsing at.
     * @param {string} nameFilter="" - The types of files to display. Examples: <code>"*.json"</code> and
     *     <code>"Images (*.png *.jpg *.svg)"</code>. All files are displayed if a filter isn't specified.
     * @returns {string} The path and name of the file if one is specified, otherwise <code>null</code>. If a single file type
     *     is specified in the nameFilter, that file type extension is automatically appended to the result when appropriate.
     * @example <caption>Ask the user to specify a file to save to.</caption>
     * var filename = Window.save("Save to JSON file", Paths.resources, "*.json");
     * print("File: " + filename);
     */
/**
     * Prompt the user to specify the path and name of a file to save to. Displays a non-model dialog that navigates the
     * directory tree and allows the user to type in a file name. A {@link Window.saveFileChanged|saveFileChanged} signal is
     * emitted when a file is specified; no signal is emitted if the user cancels the dialog.
     * @function Window.saveAsync
     * @param {string} title="" - The title to display at the top of the dialog.
     * @param {string} directory="" - The initial directory to start browsing at.
     * @param {string} nameFilter="" - The types of files to display. Examples: <code>"*.json"</code> and
     *     <code>"Images (*.png *.jpg *.svg)"</code>. All files are displayed if a filter isn't specified.
     * @example <caption>Ask the user to specify a file to save to without waiting for an answer.</caption>
     * function onSaveFileChanged(filename) {
     *     print("File: " + filename);
     * }
     * Window.saveFileChanged.connect(onSaveFileChanged);
     *
     * Window.saveAsync("Save to JSON file", Paths.resources, "*.json");
     * print("Script continues without waiting");
     */
/**
     * Prompt the user to choose an Asset Server item. Displays a modal dialog that navigates the tree of assets on the Asset
     * Server.
     * @function Window.browseAssets
     * @param {string} title="" - The title to display at the top of the dialog.
     * @param {string} directory="" - The initial directory to start browsing at.
     * @param {string} nameFilter="" - The types of files to display. Examples: <code>"*.json"</code> and 
     *     <code>"Images (*.png *.jpg *.svg)"</code>. All files are displayed if a filter isn't specified.
     * @returns {string} The path and name of the asset if one is chosen, otherwise <code>null</code>.
     * @example <caption>Ask the user to select an FBX asset.</caption>
     * var asset = Window.browseAssets("Select FBX File", "/", "*.fbx");
     * print("FBX file: " + asset);
     */
/**
     * Prompt the user to choose an Asset Server item. Displays a non-modal dialog that navigates the tree of assets on the 
     * Asset Server. A {@link Window.assetsDirChanged|assetsDirChanged} signal is emitted when an asset is chosen; no signal is
     * emitted if the user cancels the dialog.
     * @function Window.browseAssetsAsync
     * @param {string} title="" - The title to display at the top of the dialog.
     * @param {string} directory="" - The initial directory to start browsing at.
     * @param {string} nameFilter="" - The types of files to display. Examples: <code>"*.json"</code> and
     *     <code>"Images (*.png *.jpg *.svg)"</code>. All files are displayed if a filter isn't specified.
     * @example
     * function onAssetsDirChanged(asset) {
     *     print("FBX file: " + asset);
     * }
     * Window.assetsDirChanged.connect(onAssetsDirChanged);
     *
     * Window.browseAssetsAsync("Select FBX File", "/", "*.fbx");
     * print("Script continues without waiting");
     */
/**
     * Open the Asset Browser dialog. If a file to upload is specified, the user is prompted to enter the folder and name to
     * map the file to on the asset server.
     * @function Window.showAssetServer
     * @param {string} uploadFile="" - The path and name of a file to upload to the asset server.
     * @example <caption>Upload a file to the asset server.</caption>
     * var filename = Window.browse("Select File to Add to Asset Server", Paths.resources);
     * print("File: " + filename);
     * Window.showAssetServer(filename);
     */
/**
     * Get Interface's build number.
     * @function Window.checkVersion
     * @returns {string} Interface's build number.
     */
/**
     * Get the signature for Interface's protocol version.
     * @function Window.protocolSignature
     * @returns {string} A string uniquely identifying the version of the metaverse protocol that Interface is using.
     */
/**
     * Copies text to the operating system's clipboard.
     * @function Window.copyToClipboard
     * @param {string} text - The text to copy to the operating system's clipboard.
     */
/**
     * Takes a snapshot of the current Interface view from the primary camera. When a still image only is captured, 
     * {@link Window.stillSnapshotTaken|stillSnapshotTaken} is emitted; when a still image plus moving images are captured, 
     * {@link Window.processingGifStarted|processingGifStarted} and {@link Window.processingGifCompleted|processingGifCompleted}
     * are emitted. The path to store the snapshots and the length of the animated GIF to capture are specified in Settings >
     * NOTE:  to provide a non-default value - all previous parameters must be provided.
     * General > Snapshots.
     * @function Window.takeSnapshot
     * @param {boolean} notify=true - This value is passed on through the {@link Window.stillSnapshotTaken|stillSnapshotTaken}
     *     signal.
     * @param {boolean} includeAnimated=false - If <code>true</code>, a moving image is captured as an animated GIF in addition 
     *     to a still image.
     * @param {number} aspectRatio=0 - The width/height ratio of the snapshot required. If the value is <code>0</code> the
     *     full resolution is used (window dimensions in desktop mode; HMD display dimensions in HMD mode), otherwise one of the
     *     dimensions is adjusted in order to match the aspect ratio.
     * @param {string} filename="" - If this parameter is not given, the image will be saved as 'hifi-snap-by-<user name>-YYYY-MM-DD_HH-MM-SS'.
     *     If this parameter is <code>""</code> then the image will be saved as ".jpg".
     *     Otherwise, the image will be saved to this filename, with an appended ".jpg".
     *
     * @example <caption>Using the snapshot function and signals.</caption>
     * function onStillSnapshotTaken(path, notify) {
     *     print("Still snapshot taken: " + path);
     *     print("Notify: " + notify);
     * }
     *
     * function onProcessingGifStarted(stillPath) {
     *     print("Still snapshot taken: " + stillPath);
     * }
     *
     * function onProcessingGifCompleted(animatedPath) {
     *     print("Animated snapshot taken: " + animatedPath);
     * }
     *
     * Window.stillSnapshotTaken.connect(onStillSnapshotTaken);
     * Window.processingGifStarted.connect(onProcessingGifStarted);
     * Window.processingGifCompleted.connect(onProcessingGifCompleted);
     *
     * var notify = true;
     * var animated = true;
     * var aspect = 1920 / 1080;
     * var filename = "";
     * Window.takeSnapshot(notify, animated, aspect, filename);
     */
/**
     * Takes a still snapshot of the current view from the secondary camera that can be set up through the {@link Render} API.
     * NOTE:  to provide a non-default value - all previous parameters must be provided.
     * @function Window.takeSecondaryCameraSnapshot
     * @param {string} filename="" - If this parameter is not given, the image will be saved as 'hifi-snap-by-<user name>-YYYY-MM-DD_HH-MM-SS'.
     *     If this parameter is <code>""</code> then the image will be saved as ".jpg".
     *     Otherwise, the image will be saved to this filename, with an appended ".jpg".
     *
     * var filename = QString();
     */
/**
     * Emit a {@link Window.connectionAdded|connectionAdded} or a {@link Window.connectionError|connectionError} signal that
     * indicates whether or not a user connection was successfully made using the Web API.
     * @function Window.makeConnection
     * @param {boolean} success - If <code>true</code> then {@link Window.connectionAdded|connectionAdded} is emitted, otherwise
     *     {@link Window.connectionError|connectionError} is emitted.
     * @param {string} description - Descriptive text about the connection success or error. This is sent in the signal emitted.
     */
/**
     * Display a notification message. Notifications are displayed in panels by the default script, nofications.js. An
     * {@link Window.announcement|announcement} signal is emitted when this function is called.
     * @function Window.displayAnnouncement
     * @param {string} message - The announcement message.
     * @example <caption>Send and capture an announcement message.</caption>
     * function onAnnouncement(message) {
     *     // The message is also displayed as a notification by notifications.js.
     *     print("Announcement: " + message);
     * }
     * Window.announcement.connect(onAnnouncement);
     *
     * Window.displayAnnouncement("Hello");
     */
/**
     * Prepare a snapshot ready for sharing. A {@link Window.snapshotShared|snapshotShared} signal is emitted when the snapshot
     * has been prepared.
     * @function Window.shareSnapshot
     * @param {string} path - The path and name of the image file to share.
     * @param {string} href="" - The metaverse location where the snapshot was taken.
     */
/**
     * Check to see if physics is active for you in the domain you're visiting - there is a delay between your arrival at a
     * domain and physics becoming active for you in that domain.
     * @function Window.isPhysicsEnabled
     * @returns {boolean} <code>true</code> if physics is currently active for you, otherwise <code>false</code>.
     * @example <caption>Wait for physics to be enabled when you change domains.</caption>
     * function checkForPhysics() {
     *     var isPhysicsEnabled = Window.isPhysicsEnabled();
     *     print("Physics enabled: " + isPhysicsEnabled);
     *     if (!isPhysicsEnabled) {
     *         Script.setTimeout(checkForPhysics, 1000);
     *     }
     * }
     *
     * function onDomainChanged(domain) {
     *     print("Domain changed: " + domain);
     *     Script.setTimeout(checkForPhysics, 1000);
     * }
     *
     * Window.domainChanged.connect(onDomainChanged);
     */
/**
     * Set what to show on the PC display: normal view or entity camera view. The entity camera is configured using
     * {@link Camera.setCameraEntity} and {@link Camera|Camera.mode}.
     * @function Window.setDisplayTexture
     * @param {Window.DisplayTexture} texture - The view to display.
     * @returns {boolean} <code>true</code> if the display texture was successfully set, otherwise <code>false</code>.
     */
/**
     * <p>The views that may be displayed on the PC display.</p>
     * <table>
     *   <thead>
     *     <tr>
     *       <th>Value</th>
     *       <th>View Displayed</th>
     *     </tr>
     *   </thead>
     *   <tbody>
     *     <tr>
     *       <td><code>""</code></td>
     *       <td>Normal view.</td>
     *     </tr>
     *     <tr>
     *       <td><code>"resource://spectatorCameraFrame"</code></td>
     *       <td>Entity camera view.</td>
     *     </tr>
     *   </tbody>
     * </table>
     * @typedef Window.DisplayTexture
     */
/**
     * Check if a 2D point is within the desktop window if in desktop mode, or the drawable area of the HUD overlay if in HMD
     * mode.
     * @function Window.isPointOnDesktopWindow
     * @param {Vec2} point - The point to check.
     * @returns {boolean} <code>true</code> if the point is within the window or HUD, otherwise <code>false</code>.
     */
/**
     * Get the size of the drawable area of the Interface window if in desktop mode or the HMD rendering surface if in HMD mode.
     * @function Window.getDeviceSize
     * @returns {Vec2} The width and height of the Interface window or HMD rendering surface, in pixels.
     */
/**
     * Open a non-modal message box that can have a variety of button combinations. See also, 
     * {@link Window.updateMessageBox|updateMessageBox} and {@link Window.closeMessageBox|closeMessageBox}.
     * @function Window.openMessageBox
     * @param {string} title - The title to display for the message box.
     * @param {string} text - Text to display in the message box.
     * @param {Window.MessageBoxButton} buttons - The buttons to display on the message box; one or more button values added
     *     together.
     * @param {Window.MessageBoxButton} defaultButton - The button that has focus when the message box is opened.
     * @returns {number} The ID of the message box created.
     * @example <caption>Ask the user whether that want to reset something.</caption>
     * var messageBox;
     * var resetButton = 0x4000000;
     * var cancelButton = 0x400000;
     *
     * function onMessageBoxClosed(id, button) {
     *     if (id === messageBox) {
     *         if (button === resetButton) {
     *             print("Reset");
     *         } else {
     *             print("Don't reset");
     *         }
     *     }
     * }
     * Window.messageBoxClosed.connect(onMessageBoxClosed);
     *
     * messageBox = Window.openMessageBox("Reset Something", 
     *     "Do you want to reset something?",
     *     resetButton + cancelButton, cancelButton);
     */
/**
     * Update the content of a message box that was opened with {@link Window.openMessageBox|openMessageBox}.
     * @function Window.updateMessageBox
     * @param {number} id - The ID of the message box.
     * @param {string} title - The title to display for the message box.
     * @param {string} text - Text to display in the message box.
     * @param {Window.MessageBoxButton} buttons - The buttons to display on the message box; one or more button values added
     *     together.
     * @param {Window.MessageBoxButton} defaultButton - The button that has focus when the message box is opened.
     */
/**
     * Close a message box that was opened with {@link Window.openMessageBox|openMessageBox}.
     * @function Window.closeMessageBox
     * @param {number} id - The ID of the message box.
     */
/**
     * Triggered when you change the domain you're visiting. <strong>Warning:</strong> Is not emitted if you go to domain that 
     * isn't running.
     * @function Window.domainChanged
     * @param {string} domainURL - The domain's URL.
     * @returns {Signal}
     * @example <caption>Report when you change domains.</caption>
     * function onDomainChanged(domain) {
     *     print("Domain changed: " + domain);
     * }
     *
     * Window.domainChanged.connect(onDomainChanged);
     */
/**
     * Triggered when you try to navigate to a *.json, *.svo, or *.svo.json URL in a Web browser within Interface.
     * @function Window.svoImportRequested
     * @param {string} url - The URL of the file to import.
     * @returns {Signal}
     */
/**
     * Triggered when you try to visit a domain but are refused connection.
     * @function Window.domainConnectionRefused
     * @param {string} reasonMessage - A description of the refusal.
     * @param {Window.ConnectionRefusedReason} reasonCode - Integer number that enumerates the reason for the refusal.
     * @param {string} extraInfo - Extra information about the refusal.
     * @returns {Signal}
     */
/**
     * Triggered when a still snapshot has been taken by calling {@link Window.takeSnapshot|takeSnapshot} with 
     *     <code>includeAnimated = false</code> or {@link Window.takeSecondaryCameraSnapshot|takeSecondaryCameraSnapshot}.
     * @function Window.stillSnapshotTaken
     * @param {string} pathStillSnapshot - The path and name of the snapshot image file.
     * @param {boolean} notify - The value of the <code>notify</code> parameter that {@link Window.takeSnapshot|takeSnapshot}
     *     was called with.
     * @returns {Signal}
     */
/**
     * Triggered when a snapshot submitted via {@link Window.shareSnapshot|shareSnapshot} is ready for sharing. The snapshot
     * may then be shared via the {@link Account.metaverseServerURL} Web API.
     * @function Window.snapshotShared
     * @param {boolean} isError - <code>true</code> if an error was encountered preparing the snapshot for sharing, otherwise
     *     <code>false</code>.
     * @param {string} reply - JSON-formatted information about the snapshot.
     * @returns {Signal}
     */
/**
     * Triggered when the snapshot images have been captured by {@link Window.takeSnapshot|takeSnapshot} and the GIF is
     *     starting to be processed.
     * @function Window.processingGifStarted
     * @param {string} pathStillSnapshot - The path and name of the still snapshot image file.
     * @returns {Signal}
     */
/**
     * Triggered when a GIF has been prepared of the snapshot images captured by {@link Window.takeSnapshot|takeSnapshot}.
     * @function Window.processingGifCompleted
     * @param {string} pathAnimatedSnapshot - The path and name of the moving snapshot GIF file.
     * @returns {Signal}
     */
/**
     * Triggered when you've successfully made a user connection.
     * @function Window.connectionAdded
     * @param {string} message - A description of the success.
     * @returns {Signal}
     */
/**
     * Triggered when you failed to make a user connection.
     * @function Window.connectionError
     * @param {string} message - A description of the error.
     * @returns {Signal}
     */
/**
     * Triggered when a message is announced by {@link Window.displayAnnouncement|displayAnnouncement}.
     * @function Window.announcement
     * @param {string} message - The message text.
     * @returns {Signal}
     */
/**
     * Triggered when the user closes a message box that was opened with {@link Window.openMessageBox|openMessageBox}.
     * @function Window.messageBoxClosed
     * @param {number} id - The ID of the message box that was closed.
     * @param {number} button - The button that the user clicked. If the user presses Esc, the Cancel button value is returned,
     *    whether or not the Cancel button is displayed in the message box.
     * @returns {Signal}
     */
/**
     * Triggered when the user chooses a directory in a {@link Window.browseDirAsync|browseDirAsync} dialog.
     * @function Window.browseDirChanged
     * @param {string} directory - The directory the user chose in the dialog.
     * @returns {Signal}
     */
/**
     * Triggered when the user chooses an asset in a {@link Window.browseAssetsAsync|browseAssetsAsync} dialog.
     * @function Window.assetsDirChanged
     * @param {string} asset - The path and name of the asset the user chose in the dialog.
     * @returns {Signal}
     */
/**
     * Triggered when the user specifies a file in a {@link Window.saveAsync|saveAsync} dialog.
     * @function Window.saveFileChanged
     * @param {string} filename - The path and name of the file that the user specified in the dialog.
     * @returns {Signal}
     */
/**
     * Triggered when the user chooses a file in a {@link Window.browseAsync|browseAsync} dialog.
     * @function Window.browseChanged
     * @param {string} filename - The path and name of the file the user chose in the dialog.
     * @returns {Signal}
     */
/**
     * Triggered when the user OKs a {@link Window.promptAsync|promptAsync} dialog.
     * @function Window.promptTextChanged
     * @param {string} text - The text the user entered in the dialog.
     * @returns {Signal}
     */
/**
     * Triggered when the position or size of the Interface window changes.
     * @function Window.geometryChanged
     * @param {Rect} geometry - The position and size of the drawable area of the Interface window.
     * @returns {Signal}
     * @example <caption>Report the position of size of the Interface window when it changes.</caption>
     * function onWindowGeometryChanged(rect) {
     *     print("Window geometry: " + JSON.stringify(rect));
     * }
     *
     * Window.geometryChanged.connect(onWindowGeometryChanged);
     */
/**
 * @property {string} name="" - A friendly name for the overlay.
 * @property {Vec3} position - The position of the overlay center. Synonyms: <code>p1</code>, <code>point</code>, and 
 *     <code>start</code>.
 * @property {Vec3} localPosition - The local position of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>position</code>.
 * @property {Quat} rotation - The orientation of the overlay. Synonym: <code>orientation</code>.
 * @property {Quat} localRotation - The orientation of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>rotation</code>.
 * @property {boolean} isSolid=false - Synonyms: <ode>solid</code>, <code>isFilled</code>, and <code>filled</code>.
 *     Antonyms: <code>isWire</code> and <code>wire</code>.
 * @property {boolean} isDashedLine=false - If <code>true</code>, a dashed line is drawn on the overlay's edges. Synonym:
 *     <code>dashed</code>.
 * @property {boolean} ignoreRayIntersection=false - If <code>true</code>, 
 *     {@link Overlays.findRayIntersection|findRayIntersection} ignores the overlay.
 * @property {boolean} drawInFront=false - If <code>true</code>, the overlay is rendered in front of other overlays that don't
 *     have <code>drawInFront</code> set to <code>true</code>, and in front of entities.
 * @property {boolean} grabbable=false - Signal to grabbing scripts whether or not this overlay can be grabbed.
 * @property {boolean} isVisibleInSecondaryCamera=false - If <code>true</code>, the overlay is rendered in secondary
 *     camera views.
 * @property {Uuid} parentID=null - The avatar, entity, or overlay that the overlay is parented to.
 * @property {number} parentJointIndex=65535 - Integer value specifying the skeleton joint that the overlay is attached to if
 *     <code>parentID</code> is an avatar skeleton. A value of <code>65535</code> means "no joint".
 */
/**
 * @property {boolean} isFacingAvatar - If <code>true</code>, the overlay is rotated to face the user's camera about an axis
 *     parallel to the user's avatar's "up" direction.
 */
/**
 * These are the properties of a <code>circle3d</code> {@link Overlays.OverlayType|OverlayType}.
 * @typedef {object} Overlays.Circle3DProperties
 *
 * @property {string} type=circle3d - Has the value <code>"circle3d"</code>. <em>Read-only.</em>
 * @property {number} pulseMax=0 - The maximum value of the pulse multiplier.
 * @property {number} pulseMin=0 - The minimum value of the pulse multiplier.
 * @property {number} pulsePeriod=1 - The duration of the color and alpha pulse, in seconds. A pulse multiplier value goes from
 *     <code>pulseMin</code> to <code>pulseMax</code>, then <code>pulseMax</code> to <code>pulseMin</code> in one period.
 * @property {number} alphaPulse=0 - If non-zero, the alpha of the overlay is pulsed: the alpha value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 * @property {number} colorPulse=0 - If non-zero, the color of the overlay is pulsed: the color value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 * @property {boolean} visible=true - If <code>true</code>, the overlay is rendered, otherwise it is not rendered.
 *
 * @property {string} name="" - A friendly name for the overlay.
 * @property {Vec3} position - The position of the overlay center. Synonyms: <code>p1</code>, <code>point</code>, and
 *     <code>start</code>.
 * @property {Vec3} localPosition - The local position of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>position</code>.
 * @property {Quat} rotation - The orientation of the overlay. Synonym: <code>orientation</code>.
 * @property {Quat} localRotation - The orientation of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>rotation</code>.
 * @property {boolean} isSolid=false - Synonyms: <ode>solid</code>, <code>isFilled</code>, and <code>filled</code>
 *     Antonyms: <code>isWire</code> and <code>wire</code>.
 * @property {boolean} isDashedLine=false - If <code>true</code>, a dashed line is drawn on the overlay's edges. Synonym:
 *     <code>dashed</code>.
 * @property {boolean} ignoreRayIntersection=false - If <code>true</code>,
 *     {@link Overlays.findRayIntersection|findRayIntersection} ignores the overlay.
 * @property {boolean} drawInFront=false - If <code>true</code>, the overlay is rendered in front of other overlays that don't
 *     have <code>drawInFront</code> set to <code>true</code>, and in front of entities.
 * @property {boolean} grabbable=false - Signal to grabbing scripts whether or not this overlay can be grabbed.
 * @property {Uuid} parentID=null - The avatar, entity, or overlay that the overlay is parented to.
 * @property {number} parentJointIndex=65535 - Integer value specifying the skeleton joint that the overlay is attached to if
 *     <code>parentID</code> is an avatar skeleton. A value of <code>65535</code> means "no joint".
 *
 * @property {Vec2} dimensions=1,1 - The dimensions of the overlay. Synonyms: <code>scale</code>, <code>size</code>.
 *     <em>Not used.</em>
 *
 * @property {number} startAt=0 - The counter-clockwise angle from the overlay's x-axis that drawing starts at, in degrees.
 * @property {number} endAt=360 - The counter-clockwise angle from the overlay's x-axis that drawing ends at, in degrees.
 * @property {number} outerRadius=1 - The outer radius of the overlay, in meters. Synonym: <code>radius</code>.
 * @property {number} innerRadius=0 - The inner radius of the overlay, in meters.
  * @property {Color} color=255,255,255 - The color of the overlay. Setting this value also sets the values of 
 *     <code>innerStartColor</code>, <code>innerEndColor</code>, <code>outerStartColor</code>, and <code>outerEndColor</code>.
 * @property {Color} startColor - Sets the values of <code>innerStartColor</code> and <code>outerStartColor</code>.
 *     <em>Write-only.</em>
 * @property {Color} endColor - Sets the values of <code>innerEndColor</code> and <code>outerEndColor</code>.
 *     <em>Write-only.</em>
 * @property {Color} innerColor - Sets the values of <code>innerStartColor</code> and <code>innerEndColor</code>.
 *     <em>Write-only.</em>
 * @property {Color} outerColor - Sets the values of <code>outerStartColor</code> and <code>outerEndColor</code>.
 *     <em>Write-only.</em>
 * @property {Color} innerStartcolor - The color at the inner start point of the overlay.
 * @property {Color} innerEndColor - The color at the inner end point of the overlay.
 * @property {Color} outerStartColor - The color at the outer start point of the overlay.
 * @property {Color} outerEndColor - The color at the outer end point of the overlay.
 * @property {number} alpha=0.5 - The opacity of the overlay, <code>0.0</code> - <code>1.0</code>. Setting this value also sets
 *     the values of <code>innerStartAlpha</code>, <code>innerEndAlpha</code>, <code>outerStartAlpha</code>, and 
 *     <code>outerEndAlpha</code>. Synonym: <code>Alpha</code>; <em>write-only</em>.
 * @property {number} startAlpha - Sets the values of <code>innerStartAlpha</code> and <code>outerStartAlpha</code>.
 *     <em>Write-only.</em>
 * @property {number} endAlpha - Sets the values of <code>innerEndAlpha</code> and <code>outerEndAlpha</code>.
 *     <em>Write-only.</em>
 * @property {number} innerAlpha - Sets the values of <code>innerStartAlpha</code> and <code>innerEndAlpha</code>.
 *     <em>Write-only.</em>
 * @property {number} outerAlpha - Sets the values of <code>outerStartAlpha</code> and <code>outerEndAlpha</code>.
 *     <em>Write-only.</em>
 * @property {number} innerStartAlpha=0 - The alpha at the inner start point of the overlay.
 * @property {number} innerEndAlpha=0 - The alpha at the inner end point of the overlay.
 * @property {number} outerStartAlpha=0 - The alpha at the outer start point of the overlay.
 * @property {number} outerEndAlpha=0 - The alpha at the outer end point of the overlay.

 * @property {boolean} hasTickMarks=false - If <code>true</code>, tick marks are drawn.
 * @property {number} majorTickMarksAngle=0 - The angle between major tick marks, in degrees.
 * @property {number} minorTickMarksAngle=0 - The angle between minor tick marks, in degrees.
 * @property {number} majorTickMarksLength=0 - The length of the major tick marks, in meters. A positive value draws tick marks
 *     outwards from the inner radius; a negative value draws tick marks inwards from the outer radius.
 * @property {number} minorTickMarksLength=0 - The length of the minor tick marks, in meters. A positive value draws tick marks
 *     outwards from the inner radius; a negative value draws tick marks inwards from the outer radius.
 * @property {Color} majorTickMarksColor=0,0,0 - The color of the major tick marks.
 * @property {Color} minorTickMarksColor=0,0,0 - The color of the minor tick marks.
 */
/**
* @namespace ContextOverlay
*/
/**
 * These are the properties of a <code>cube</code> {@link Overlays.OverlayType|OverlayType}.
 * @typedef {object} Overlays.CubeProperties
 * 
 * @property {string} type=cube - Has the value <code>"cube"</code>. <em>Read-only.</em>
 * @property {Color} color=255,255,255 - The color of the overlay.
 * @property {number} alpha=0.7 - The opacity of the overlay, <code>0.0</code> - <code>1.0</code>.
 * @property {number} pulseMax=0 - The maximum value of the pulse multiplier.
 * @property {number} pulseMin=0 - The minimum value of the pulse multiplier.
 * @property {number} pulsePeriod=1 - The duration of the color and alpha pulse, in seconds. A pulse multiplier value goes from
 *     <code>pulseMin</code> to <code>pulseMax</code>, then <code>pulseMax</code> to <code>pulseMin</code> in one period.
 * @property {number} alphaPulse=0 - If non-zero, the alpha of the overlay is pulsed: the alpha value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 * @property {number} colorPulse=0 - If non-zero, the color of the overlay is pulsed: the color value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 * @property {boolean} visible=true - If <code>true</code>, the overlay is rendered, otherwise it is not rendered.
 *
 * @property {string} name="" - A friendly name for the overlay.
 * @property {Vec3} position - The position of the overlay center. Synonyms: <code>p1</code>, <code>point</code>, and
 *     <code>start</code>.
 * @property {Vec3} localPosition - The local position of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>position</code>.
 * @property {Quat} rotation - The orientation of the overlay. Synonym: <code>orientation</code>.
 * @property {Quat} localRotation - The orientation of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>rotation</code>.
 * @property {boolean} isSolid=false - Synonyms: <ode>solid</code>, <code>isFilled</code>, and <code>filled</code>.
 *     Antonyms: <code>isWire</code> and <code>wire</code>.
 * @property {boolean} isDashedLine=false - If <code>true</code>, a dashed line is drawn on the overlay's edges. Synonym:
 *     <code>dashed</code>.
 * @property {boolean} ignoreRayIntersection=false - If <code>true</code>,
 *     {@link Overlays.findRayIntersection|findRayIntersection} ignores the overlay.
 * @property {boolean} drawInFront=false - If <code>true</code>, the overlay is rendered in front of other overlays that don't
 *     have <code>drawInFront</code> set to <code>true</code>, and in front of entities.
 * @property {boolean} grabbable=false - Signal to grabbing scripts whether or not this overlay can be grabbed.
 * @property {Uuid} parentID=null - The avatar, entity, or overlay that the overlay is parented to.
 * @property {number} parentJointIndex=65535 - Integer value specifying the skeleton joint that the overlay is attached to if
 *     <code>parentID</code> is an avatar skeleton. A value of <code>65535</code> means "no joint".
 *
 * @property {Vec3} dimensions - The dimensions of the overlay. Synonyms: <code>scale</code>, <code>size</code>.
 */
/**
 * These are the properties of a <code>grid</code> {@link Overlays.OverlayType|OverlayType}.
 * @typedef {object} Overlays.GridProperties
 *
 * @property {string} type=grid - Has the value <code>"grid"</code>. <em>Read-only.</em>
 * @property {Color} color=255,255,255 - The color of the overlay.
 * @property {number} alpha=0.7 - The opacity of the overlay, <code>0.0</code> - <code>1.0</code>.
 * @property {number} pulseMax=0 - The maximum value of the pulse multiplier.
 * @property {number} pulseMin=0 - The minimum value of the pulse multiplier.
 * @property {number} pulsePeriod=1 - The duration of the color and alpha pulse, in seconds. A pulse multiplier value goes from
 *     <code>pulseMin</code> to <code>pulseMax</code>, then <code>pulseMax</code> to <code>pulseMin</code> in one period.
 * @property {number} alphaPulse=0 - If non-zero, the alpha of the overlay is pulsed: the alpha value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 * @property {number} colorPulse=0 - If non-zero, the color of the overlay is pulsed: the color value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 * @property {boolean} visible=true - If <code>true</code>, the overlay is rendered, otherwise it is not rendered.
 *
 * @property {string} name="" - A friendly name for the overlay.
 * @property {Vec3} position - The position of the overlay center. Synonyms: <code>p1</code>, <code>point</code>, and
 *     <code>start</code>.
 * @property {Vec3} localPosition - The local position of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>position</code>.
 * @property {Quat} rotation - The orientation of the overlay. Synonym: <code>orientation</code>.
 * @property {Quat} localRotation - The orientation of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>rotation</code>.
 * @property {boolean} isSolid=false - Synonyms: <ode>solid</code>, <code>isFilled</code>, and <code>filled</code>.
 *     Antonyms: <code>isWire</code> and <code>wire</code>.
 * @property {boolean} isDashedLine=false - If <code>true</code>, a dashed line is drawn on the overlay's edges. Synonym:
 *     <code>dashed</code>.
 * @property {boolean} ignoreRayIntersection=false - If <code>true</code>,
 *     {@link Overlays.findRayIntersection|findRayIntersection} ignores the overlay.
 * @property {boolean} drawInFront=false - If <code>true</code>, the overlay is rendered in front of other overlays that don't
 *     have <code>drawInFront</code> set to <code>true</code>, and in front of entities.
 * @property {boolean} grabbable=false - Signal to grabbing scripts whether or not this overlay can be grabbed.
 * @property {Uuid} parentID=null - The avatar, entity, or overlay that the overlay is parented to.
 * @property {number} parentJointIndex=65535 - Integer value specifying the skeleton joint that the overlay is attached to if
 *     <code>parentID</code> is an avatar skeleton. A value of <code>65535</code> means "no joint".
 *
 * @property {Vec2} dimensions=1,1 - The dimensions of the overlay. Synonyms: <code>scale</code>, <code>size</code>.
 *
 * @property {boolean} followCamera=true - If <code>true</code>, the grid is always visible even as the camera moves to another
 *     position.
 * @property {number} majorGridEvery=5 - Integer number of <code>minorGridEvery</code> intervals at which to draw a thick grid 
 *     line. Minimum value = <code>1</code>.
 * @property {number} minorGridEvery=1 - Real number of meters at which to draw thin grid lines. Minimum value = 
 *     <code>0.001</code>.
 */
/**
 * These are the properties of an <code>image3d</code> {@link Overlays.OverlayType|OverlayType}.
 * @typedef {object} Overlays.Image3DProperties
 *
 * @property {string} type=image3d - Has the value <code>"image3d"</code>. <em>Read-only.</em>
 * @property {Color} color=255,255,255 - The color of the overlay.
 * @property {number} alpha=0.7 - The opacity of the overlay, <code>0.0</code> - <code>1.0</code>.
 * @property {number} pulseMax=0 - The maximum value of the pulse multiplier.
 * @property {number} pulseMin=0 - The minimum value of the pulse multiplier.
 * @property {number} pulsePeriod=1 - The duration of the color and alpha pulse, in seconds. A pulse multiplier value goes from
 *     <code>pulseMin</code> to <code>pulseMax</code>, then <code>pulseMax</code> to <code>pulseMin</code> in one period.
 * @property {number} alphaPulse=0 - If non-zero, the alpha of the overlay is pulsed: the alpha value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 * @property {number} colorPulse=0 - If non-zero, the color of the overlay is pulsed: the color value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 * @property {boolean} visible=true - If <code>true</code>, the overlay is rendered, otherwise it is not rendered.
 *
 * @property {string} name="" - A friendly name for the overlay.
 * @property {Vec3} position - The position of the overlay center. Synonyms: <code>p1</code>, <code>point</code>, and 
 *     <code>start</code>.
 * @property {Vec3} localPosition - The local position of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>position</code>.
 * @property {Quat} rotation - The orientation of the overlay. Synonym: <code>orientation</code>.
 * @property {Quat} localRotation - The orientation of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>rotation</code>.
 * @property {boolean} isSolid=false - Synonyms: <ode>solid</code>, <code>isFilled</code>, and <code>filled</code>.
 *     Antonyms: <code>isWire</code> and <code>wire</code>.
 * @property {boolean} isDashedLine=false - If <code>true</code>, a dashed line is drawn on the overlay's edges. Synonym:
 *     <code>dashed</code>.
 * @property {boolean} ignoreRayIntersection=false - If <code>true</code>, 
 *     {@link Overlays.findRayIntersection|findRayIntersection} ignores the overlay.
 * @property {boolean} drawInFront=false - If <code>true</code>, the overlay is rendered in front of other overlays that don't
 *     have <code>drawInFront</code> set to <code>true</code>, and in front of entities.
 * @property {boolean} grabbable=false - Signal to grabbing scripts whether or not this overlay can be grabbed.
 * @property {Uuid} parentID=null - The avatar, entity, or overlay that the overlay is parented to.
 * @property {number} parentJointIndex=65535 - Integer value specifying the skeleton joint that the overlay is attached to if
 *     <code>parentID</code> is an avatar skeleton. A value of <code>65535</code> means "no joint".
 *
 * @property {Vec2} dimensions=1,1 - The dimensions of the overlay. Synonyms: <code>scale</code>, <code>size</code>.
 *
 * @property {boolean} isFacingAvatar - If <code>true</code>, the overlay is rotated to face the user's camera about an axis
 *     parallel to the user's avatar's "up" direction.
 *
 * @property {string} url - The URL of the PNG or JPG image to display.
 * @property {Rect} subImage - The portion of the image to display. Defaults to the full image.
 * @property {boolean} emissive - If <code>true</code>, the overlay is displayed at full brightness, otherwise it is rendered
 *     with scene lighting.
 */
/**
 * These are the properties of an <code>image</code> {@link Overlays.OverlayType|OverlayType}.
 * @typedef {object} Overlays.ImageProperties
 *
 * @property {Rect} bounds - The position and size of the image display area, in pixels. <em>Write-only.</em>
 * @property {number} x - Integer left, x-coordinate value of the image display area = <code>bounds.x</code>.
 *     <em>Write-only.</em>
 * @property {number} y - Integer top, y-coordinate value of the image display area = <code>bounds.y</code>.
 *     <em>Write-only.</em>
 * @property {number} width - Integer width of the image display area = <code>bounds.width</code>. <em>Write-only.</em>
 * @property {number} height - Integer height of the image display area = <code>bounds.height</code>. <em>Write-only.</em>
 * @property {string} imageURL - The URL of the image file to display. The image is scaled to fit to the <code>bounds</code>.
 *     <em>Write-only.</em>
 * @property {Vec2} subImage=0,0 - Integer coordinates of the top left pixel to start using image content from.
 *     <em>Write-only.</em>
 * @property {Color} color=0,0,0 - The color to apply over the top of the image to colorize it. <em>Write-only.</em>
 * @property {number} alpha=0.0 - The opacity of the color applied over the top of the image, <code>0.0</code> - 
 *     <code>1.0</code>. <em>Write-only.</em>
 * @property {boolean} visible=true - If <code>true</code>, the overlay is rendered, otherwise it is not rendered.
 *     <em>Write-only.</em>
 */
/**
 * These are the properties of a <code>line3d</code> {@link Overlays.OverlayType|OverlayType}.
 * @typedef {object} Overlays.Line3DProperties
 * 
 * @property {string} type=line3d - Has the value <code>"line3d"</code>. <em>Read-only.</em>
 * @property {Color} color=255,255,255 - The color of the overlay.
 * @property {number} alpha=0.7 - The opacity of the overlay, <code>0.0</code> - <code>1.0</code>.
 * @property {number} pulseMax=0 - The maximum value of the pulse multiplier.
 * @property {number} pulseMin=0 - The minimum value of the pulse multiplier.
 * @property {number} pulsePeriod=1 - The duration of the color and alpha pulse, in seconds. A pulse multiplier value goes from
 *     <code>pulseMin</code> to <code>pulseMax</code>, then <code>pulseMax</code> to <code>pulseMin</code> in one period.
 * @property {number} alphaPulse=0 - If non-zero, the alpha of the overlay is pulsed: the alpha value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 * @property {number} colorPulse=0 - If non-zero, the color of the overlay is pulsed: the color value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 * @property {boolean} visible=true - If <code>true</code>, the overlay is rendered, otherwise it is not rendered.
 *
 * @property {string} name="" - A friendly name for the overlay.
 * @property {Vec3} position - The position of the overlay center. Synonyms: <code>p1</code>, <code>point</code>, and
 *     <code>start</code>.
 * @property {Vec3} localPosition - The local position of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>position</code>.
 * @property {Quat} rotation - The orientation of the overlay. Synonym: <code>orientation</code>.
 * @property {Quat} localRotation - The orientation of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>rotation</code>.
 * @property {boolean} isSolid=false - Synonyms: <ode>solid</code>, <code>isFilled</code>, and <code>filled</code>.
 *     Antonyms: <code>isWire</code> and <code>wire</code>.
 * @property {boolean} isDashedLine=false - If <code>true</code>, a dashed line is drawn on the overlay's edges. Synonym:
 *     <code>dashed</code>.
 * @property {boolean} ignoreRayIntersection=false - If <code>true</code>,
 *     {@link Overlays.findRayIntersection|findRayIntersection} ignores the overlay.
 * @property {boolean} drawInFront=false - If <code>true</code>, the overlay is rendered in front of other overlays that don't
 *     have <code>drawInFront</code> set to <code>true</code>, and in front of entities.
 * @property {boolean} grabbable=false - Signal to grabbing scripts whether or not this overlay can be grabbed.
 * @property {Uuid} parentID=null - The avatar, entity, or overlay that the overlay is parented to.
 * @property {number} parentJointIndex=65535 - Integer value specifying the skeleton joint that the overlay is attached to if
 *     <code>parentID</code> is an avatar skeleton. A value of <code>65535</code> means "no joint".
 *
 * @property {Uuid} endParentID=null - The avatar, entity, or overlay that the end point of the line is parented to.
 * @property {number} endParentJointIndex=65535 - Integer value specifying the skeleton joint that the end point of the line is
 *     attached to if <code>parentID</code> is an avatar skeleton. A value of <code>65535</code> means "no joint".
 * @property {Vec3} start - The start point of the line. Synonyms: <code>startPoint</code>, <code>p1</code>, and
 *     <code>position</code>.
 * @property {Vec3} end - The end point of the line. Synonyms: <code>endPoint</code> and <code>p2</code>.
 * @property {Vec3} localStart - The local position of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>start</code>. Synonym: <code>localPosition</code>.
 * @property {Vec3} localEnd - The local position of the overlay relative to its parent if the overlay has a
 *     <code>endParentID</code> set, otherwise the same value as <code>end</code>.
 * @property {number} length - The length of the line, in meters. This can be set after creating a line with start and end
 *     points.
 * @property {number} glow=0 - If <code>glow > 0</code>, the line is rendered with a glow.
 * @property {number} lineWidth=0.02 - If <code>glow > 0</code>, this is the width of the glow, in meters.
 */
/**
 * These are the properties of a <code>model</code> {@link Overlays.OverlayType|OverlayType}.
 * @typedef {object} Overlays.ModelProperties
 *
 * @property {string} type=sphere - Has the value <code>"model"</code>. <em>Read-only.</em>
 * @property {Color} color=255,255,255 - The color of the overlay.
 * @property {number} alpha=0.7 - The opacity of the overlay, <code>0.0</code> - <code>1.0</code>.
 * @property {number} pulseMax=0 - The maximum value of the pulse multiplier.
 * @property {number} pulseMin=0 - The minimum value of the pulse multiplier.
 * @property {number} pulsePeriod=1 - The duration of the color and alpha pulse, in seconds. A pulse multiplier value goes from
 *     <code>pulseMin</code> to <code>pulseMax</code>, then <code>pulseMax</code> to <code>pulseMin</code> in one period.
 * @property {number} alphaPulse=0 - If non-zero, the alpha of the overlay is pulsed: the alpha value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 * @property {number} colorPulse=0 - If non-zero, the color of the overlay is pulsed: the color value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 * @property {boolean} visible=true - If <code>true</code>, the overlay is rendered, otherwise it is not rendered.
 *
 * @property {string} name="" - A friendly name for the overlay.
 * @property {Vec3} position - The position of the overlay center. Synonyms: <code>p1</code>, <code>point</code>, and
 *     <code>start</code>.
 * @property {Vec3} localPosition - The local position of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>position</code>.
 * @property {Quat} rotation - The orientation of the overlay. Synonym: <code>orientation</code>.
 * @property {Quat} localRotation - The orientation of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>rotation</code>.
 * @property {boolean} isSolid=false - Synonyms: <ode>solid</code>, <code>isFilled</code>, and <code>filled</code>.
 *     Antonyms: <code>isWire</code> and <code>wire</code>.
 * @property {boolean} isDashedLine=false - If <code>true</code>, a dashed line is drawn on the overlay's edges. Synonym:
 *     <code>dashed</code>.
 * @property {boolean} ignoreRayIntersection=false - If <code>true</code>,
 *     {@link Overlays.findRayIntersection|findRayIntersection} ignores the overlay.
 * @property {boolean} drawInFront=false - If <code>true</code>, the overlay is rendered in front of other overlays that don't
 *     have <code>drawInFront</code> set to <code>true</code>, and in front of entities.
 * @property {boolean} grabbable=false - Signal to grabbing scripts whether or not this overlay can be grabbed.
 * @property {Uuid} parentID=null - The avatar, entity, or overlay that the overlay is parented to.
 * @property {number} parentJointIndex=65535 - Integer value specifying the skeleton joint that the overlay is attached to if
 *     <code>parentID</code> is an avatar skeleton. A value of <code>65535</code> means "no joint".
 *
 * @property {string} url - The URL of the FBX or OBJ model used for the overlay.
 * @property {Vec3} dimensions - The dimensions of the overlay. Synonym: <code>size</code>.
 * @property {Vec3} scale - The scale factor applied to the model's dimensions.
 * @property {object.<name, url>} textures - Maps the named textures in the model to the JPG or PNG images in the urls.
 * @property {Array.<string>} jointNames - The names of the joints - if any - in the model. <em>Read-only</em>
 * @property {Array.<Quat>} jointRotations - The relative rotations of the model's joints.
 * @property {Array.<Vec3>} jointTranslations - The relative translations of the model's joints.
 * @property {Array.<Quat>} jointOrientations - The absolute orientations of the model's joints, in world coordinates.
 *     <em>Read-only</em>
 * @property {Array.<Vec3>} jointPositions - The absolute positions of the model's joints, in world coordinates.
 *     <em>Read-only</em>
 * @property {string} animationSettings.url="" - The URL of an FBX file containing an animation to play.
 * @property {number} animationSettings.fps=0 - The frame rate (frames/sec) to play the animation at. 
 * @property {number} animationSettings.firstFrame=0 - The frame to start playing at.
 * @property {number} animationSettings.lastFrame=0 - The frame to finish playing at.
 * @property {boolean} animationSettings.running=false - Whether or not the animation is playing.
 * @property {boolean} animationSettings.loop=false - Whether or not the animation should repeat in a loop.
 * @property {boolean} animationSettings.hold=false - Whether or not when the animation finishes, the rotations and 
 *     translations of the last frame played should be maintained.
 * @property {boolean} animationSettings.allowTranslation=false - Whether or not translations contained in the animation should
 *     be played.
 */
/**
  * @property {string} type=TODO - Has the value <code>"TODO"</code>. <em>Read-only.</em>
  * @property {Color} color=255,255,255 - The color of the overlay.
  * @property {number} alpha=0.7 - The opacity of the overlay, <code>0.0</code> - <code>1.0</code>.
  * @property {number} pulseMax=0 - The maximum value of the pulse multiplier.
  * @property {number} pulseMin=0 - The minimum value of the pulse multiplier.
  * @property {number} pulsePeriod=1 - The duration of the color and alpha pulse, in seconds. A pulse multiplier value goes from 
  *     <code>pulseMin</code> to <code>pulseMax</code>, then <code>pulseMax</code> to <code>pulseMin</code> in one period.
  * @property {number} alphaPulse=0 - If non-zero, the alpha of the overlay is pulsed: the alpha value is multiplied by the 
  *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0 
  *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
  *     used.)
  * @property {number} colorPulse=0 - If non-zero, the color of the overlay is pulsed: the color value is multiplied by the 
  *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0 
  *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
  *     used.)
  * @property {boolean} visible=true - If <code>true</code>, the overlay is rendered, otherwise it is not rendered.
  */
/**
 * @property {Rect} bounds - The position and size of the rectangle. <em>Write-only.</em>
 * @property {number} x - Integer left, x-coordinate value = <code>bounds.x</code>. <em>Write-only.</em>
 * @property {number} y - Integer top, y-coordinate value = <code>bounds.y</code>. <em>Write-only.</em>
 * @property {number} width - Integer width of the rectangle = <code>bounds.width</code>. <em>Write-only.</em>
 * @property {number} height - Integer height of the rectangle = <code>bounds.height</code>. <em>Write-only.</em>
 */
/**
     * <p>An overlay may be one of the following types:</p>
     * <table>
     *   <thead>
     *     <tr><th>Value</th><th>2D/3D</th><th>Description</th></tr>
     *   </thead>
     *   <tbody>
     *     <tr><td><code>circle3d</code></td><td>3D</td><td>A circle.</td></tr>
     *     <tr><td><code>cube</code></td><td>3D</td><td>A cube. Can also use a <code>shape</code> overlay to create a 
     *     cube.</td></tr>
     *     <tr><td><code>grid</code></td><td>3D</td><td>A grid of lines in a plane.</td></tr>
     *     <tr><td><code>image</code></td><td>2D</td><td>An image. Synonym: <code>billboard</code>.</td></tr>
     *     <tr><td><code>image3d</code></td><td>3D</td><td>An image.</td></tr>
     *     <tr><td><code>line3d</code></td><td>3D</td><td>A line.</td></tr>
     *     <tr><td><code>model</code></td><td>3D</td><td>A model.</td></tr>
     *     <tr><td><code>rectangle</code></td><td>2D</td><td>A rectangle.</td></tr>
     *     <tr><td><code>rectangle3d</code></td><td>3D</td><td>A rectangle.</td></tr>
     *     <tr><td><code>shape</code></td><td>3D</td><td>A geometric shape, such as a cube, sphere, or cylinder.</td></tr>
     *     <tr><td><code>sphere</code></td><td>3D</td><td>A sphere. Can also use a <code>shape</code> overlay to create a 
     *     sphere.</td></tr>
     *     <tr><td><code>text</code></td><td>2D</td><td>Text.</td></tr>
     *     <tr><td><code>text3d</code></td><td>3D</td><td>Text.</td></tr>
     *     <tr><td><code>web3d</code></td><td>3D</td><td>Web content.</td></tr>
     *   </tbody>
     * </table>
     * <p>2D overlays are rendered on the display surface in desktop mode and on the HUD surface in HMD mode. 3D overlays are
     * rendered at a position and orientation in-world.<p>
     * <p>Each overlay type has different {@link Overlays.OverlayProperties|OverlayProperties}.</p>
     * @typedef {string} Overlays.OverlayType
     */
/**
     * <p>Different overlay types have different properties:</p>
     * <table>
     *   <thead>
     *     <tr><th>{@link Overlays.OverlayType|OverlayType}</th><th>Overlay Properties</th></tr>
     *   </thead>
     *   <tbody>
     *     <tr><td><code>circle3d</code></td><td>{@link Overlays.Circle3DProperties|Circle3DProperties}</td></tr>
     *     <tr><td><code>cube</code></td><td>{@link Overlays.CubeProperties|CubeProperties}</td></tr>
     *     <tr><td><code>grid</code></td><td>{@link Overlays.GridProperties|GridProperties}</td></tr>
     *     <tr><td><code>image</code></td><td>{@link Overlays.ImageProperties|ImageProperties}</td></tr>
     *     <tr><td><code>image3d</code></td><td>{@link Overlays.Image3DProperties|Image3DProperties}</td></tr>
     *     <tr><td><code>line3d</code></td><td>{@link Overlays.Line3DProperties|Line3DProperties}</td></tr>
     *     <tr><td><code>model</code></td><td>{@link Overlays.ModelProperties|ModelProperties}</td></tr>
     *     <tr><td><code>rectangle</code></td><td>{@link Overlays.RectangleProperties|RectangleProperties}</td></tr>
     *     <tr><td><code>rectangle3d</code></td><td>{@link Overlays.Rectangle3DProperties|Rectangle3DProperties}</td></tr>
     *     <tr><td><code>shape</code></td><td>{@link Overlays.ShapeProperties|ShapeProperties}</td></tr>
     *     <tr><td><code>sphere</code></td><td>{@link Overlays.SphereProperties|SphereProperties}</td></tr>
     *     <tr><td><code>text</code></td><td>{@link Overlays.TextProperties|TextProperties}</td></tr>
     *     <tr><td><code>text3d</code></td><td>{@link Overlays.Text3DProperties|Text3DProperties}</td></tr>
     *     <tr><td><code>web3d</code></td><td>{@link Overlays.Web3DProperties|Web3DProperties}</td></tr>
     *   </tbody>
     * </table>
     * @typedef {object} Overlays.OverlayProperties
     */
/**
 * The result of a {@link PickRay} search using {@link Overlays.findRayIntersection|findRayIntersection} or 
 * {@link Overlays.findRayIntersectionVector|findRayIntersectionVector}.
 * @typedef {object} Overlays.RayToOverlayIntersectionResult
 * @property {boolean} intersects - <code>true</code> if the {@link PickRay} intersected with a 3D overlay, otherwise
 *     <code>false</code>.
 * @property {Uuid} overlayID - The UUID of the overlay that was intersected.
 * @property {number} distance - The distance from the {@link PickRay} origin to the intersection point.
 * @property {Vec3} surfaceNormal - The normal of the overlay surface at the intersection point.
 * @property {Vec3} intersection - The position of the intersection point.
 * @property {Object} extraInfo Additional intersection details, if available.
 */
/**
 * The Overlays API provides facilities to create and interact with overlays. Overlays are 2D and 3D objects visible only to
 * yourself and that aren't persisted to the domain. They are used for UI.
 * @namespace Overlays
 * @property {Uuid} keyboardFocusOverlay - Get or set the {@link Overlays.OverlayType|web3d} overlay that has keyboard focus.
 *     If no overlay has keyboard focus, get returns <code>null</code>; set to <code>null</code> or {@link Uuid|Uuid.NULL} to 
 *     clear keyboard focus.
 */
/**
     * Add an overlay to the scene.
     * @function Overlays.addOverlay
     * @param {Overlays.OverlayType} type - The type of the overlay to add.
     * @param {Overlays.OverlayProperties} properties - The properties of the overlay to add.
     * @returns {Uuid} The ID of the newly created overlay if successful, otherwise {@link Uuid|Uuid.NULL}.
     * @example <caption>Add a cube overlay in front of your avatar.</caption>
     * var overlay = Overlays.addOverlay("cube", {
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -3 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 0.3, y: 0.3, z: 0.3 },
     *     solid: true
     * });
     */
/**
     * Create a clone of an existing overlay.
     * @function Overlays.cloneOverlay
     * @param {Uuid} overlayID - The ID of the overlay to clone.
     * @returns {Uuid} The ID of the new overlay if successful, otherwise {@link Uuid|Uuid.NULL}.
     * @example <caption>Add an overlay in front of your avatar, clone it, and move the clone to be above the 
     *     original.</caption>
     * var position = Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -3 }));
     * var original = Overlays.addOverlay("cube", {
     *     position: position,
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 0.3, y: 0.3, z: 0.3 },
     *     solid: true
     * });
     *
     * var clone = Overlays.cloneOverlay(original);
     * Overlays.editOverlay(clone, {
     *     position: Vec3.sum({ x: 0, y: 0.5, z: 0}, position)
     * });
     */
/**
     * Edit an overlay's properties.
     * @function Overlays.editOverlay
     * @param {Uuid} overlayID - The ID of the overlay to edit.
     * @param {Overlays.OverlayProperties} properties - The properties changes to make.
     * @returns {boolean} <code>true</code> if the overlay was found and edited, otherwise <code>false</code>.
     * @example <caption>Add an overlay in front of your avatar then change its color.</caption>
     * var overlay = Overlays.addOverlay("cube", {
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -3 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 0.3, y: 0.3, z: 0.3 },
     *     solid: true
     * });
     *
     * var success = Overlays.editOverlay(overlay, {
     *     color: { red: 255, green: 0, blue: 0 }
     * });
     * print("Success: " + success);
     */
/**
     * Edit multiple overlays' properties.
     * @function Overlays.editOverlays
     * @param propertiesById {object.<Uuid, Overlays.OverlayProperties>} - An object with overlay IDs as keys and
     *     {@link Overlays.OverlayProperties|OverlayProperties} edits to make as values.
     * @returns {boolean} <code>true</code> if all overlays were found and edited, otherwise <code>false</code> (some may have
     *     been found and edited).
     * @example <caption>Create two overlays in front of your avatar then change their colors.</caption>
     * var overlayA = Overlays.addOverlay("cube", {
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: -0.3, y: 0, z: -3 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 0.3, y: 0.3, z: 0.3 },
     *     solid: true
     * });
     * var overlayB = Overlays.addOverlay("cube", {
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0.3, y: 0, z: -3 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 0.3, y: 0.3, z: 0.3 },
     *     solid: true
     * });
     *
     * var overlayEdits = {};
     * overlayEdits[overlayA] = { color: { red: 255, green: 0, blue: 0 } };
     * overlayEdits[overlayB] = { color: { red: 0, green: 255, blue: 0 } };
     * var success = Overlays.editOverlays(overlayEdits);
     * print("Success: " + success);
     */
/**
     * Delete an overlay.
     * @function Overlays.deleteOverlay
     * @param {Uuid} overlayID - The ID of the overlay to delete.
     * @example <caption>Create an overlay in front of your avatar then delete it.</caption>
     * var overlay = Overlays.addOverlay("cube", {
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -3 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 0.3, y: 0.3, z: 0.3 },
     *     solid: true
     * });
     * print("Overlay: " + overlay);
     * Overlays.deleteOverlay(overlay);
     */
/**
     * Get the type of an overlay.
     * @function Overlays.getOverlayType
     * @param {Uuid} overlayID - The ID of the overlay to get the type of.
     * @returns {Overlays.OverlayType} The type of the overlay if found, otherwise an empty string.
     * @example <caption>Create an overlay in front of your avatar then get and report its type.</caption>
     * var overlay = Overlays.addOverlay("cube", {
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -3 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 0.3, y: 0.3, z: 0.3 },
     *     solid: true
     * });
     * var type = Overlays.getOverlayType(overlay);
     * print("Type: " + type);
     */
/**
     * Get the overlay script object. In particular, this is useful for accessing the event bridge for a <code>web3d</code> 
     * overlay.
     * @function Overlays.getOverlayObject
     * @param {Uuid} overlayID - The ID of the overlay to get the script object of.
     * @returns {object} The script object for the overlay if found.
     * @example <caption>Receive "hello" messages from a <code>web3d</code> overlay.</caption>
     * // HTML file: name "web3d.html".
     * <!DOCTYPE html>
     * <html>
     * <head>
     *     <title>HELLO</title>
     * </head>
     * <body>
     *     <h1>HELLO</h1></h1>
     *     <script>
     *         setInterval(function () {
     *             EventBridge.emitWebEvent("hello");
     *         }, 2000);
     *     </script>
     * </body>
     * </html>
     *
     * // Script file.
     * var web3dOverlay = Overlays.addOverlay("web3d", {
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, {x: 0, y: 0.5, z: -3 })),
     *     rotation: MyAvatar.orientation,
     *     url: Script.resolvePath("web3d.html"),
     *     alpha: 1.0
     * });
     *
     * function onWebEventReceived(event) {
     *     print("onWebEventReceived() : " + JSON.stringify(event));
     * }
     *
     * overlayObject = Overlays.getOverlayObject(web3dOverlay);
     * overlayObject.webEventReceived.connect(onWebEventReceived);
     *
     * Script.scriptEnding.connect(function () {
     *     Overlays.deleteOverlay(web3dOverlay);
     * });
     */
/**
     * Get the ID of the 2D overlay at a particular point on the screen or HUD.
     * @function Overlays.getOverlayAtPoint
     * @param {Vec2} point - The point to check for an overlay.
     * @returns {Uuid} The ID of the 2D overlay at the specified point if found, otherwise <code>null</code>.
     * @example <caption>Create a 2D overlay and add an event function that reports the overlay clicked on, if any.</caption>
     * var overlay = Overlays.addOverlay("rectangle", {
     *     bounds: { x: 100, y: 100, width: 200, height: 100 },
     *     color: { red: 255, green: 255, blue: 255 }
     * });
     * print("Created: " + overlay);
     *
     * Controller.mousePressEvent.connect(function (event) {
     *     var overlay = Overlays.getOverlayAtPoint({ x: event.x, y: event.y });
     *     print("Clicked: " + overlay);
     * });
     */
/**
     * Get the value of a 3D overlay's property.
     * @function Overlays.getProperty
     * @param {Uuid} overlayID - The ID of the overlay. <em>Must be for a 3D {@link Overlays.OverlayType|OverlayType}.</em>
     * @param {string} property - The name of the property value to get.
     * @returns {object} The value of the property if the 3D overlay and property can be found, otherwise
     *     <code>undefined</code>.
     * @example <caption>Create an overlay in front of your avatar then report its alpha property value.</caption>
     * var overlay = Overlays.addOverlay("cube", {
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -3 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 0.3, y: 0.3, z: 0.3 },
     *     solid: true
     * });
     * var alpha = Overlays.getProperty(overlay, "alpha");
     * print("Overlay alpha: " + alpha);
     */
/**
     * Get the values of an overlay's properties.
     * @function Overlays.getProperties
     * @param {Uuid} overlayID - The ID of the overlay.
     * @param {Array.<string>} properties - An array of names of properties to get the values of.
     * @returns {Overlays.OverlayProperties} The values of valid properties if the overlay can be found, otherwise 
     *     <code>undefined</code>.
     * @example <caption>Create an overlay in front of your avatar then report some of its properties.</caption>
     * var overlay = Overlays.addOverlay("cube", {
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -3 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 0.3, y: 0.3, z: 0.3 },
     *     solid: true
     * });
     * var properties = Overlays.getProperties(overlay, ["color", "alpha", "grabbable"]);
     * print("Overlay properties: " + JSON.stringify(properties));
     */
/**
     * Get the values of multiple overlays' properties.
     * @function Overlays.getOverlaysProperties
     * @param propertiesById {object.<Uuid, Array.<string>>} - An object with overlay IDs as keys and arrays of the names of 
     *     properties to get for each as values.
     * @returns {object.<Uuid, Overlays.OverlayProperties>} An object with overlay IDs as keys and
     *     {@link Overlays.OverlayProperties|OverlayProperties} as values.
     * @example <caption>Create two cube overlays in front of your avatar then get some of their properties.</caption>
     * var overlayA = Overlays.addOverlay("cube", {
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: -0.3, y: 0, z: -3 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 0.3, y: 0.3, z: 0.3 },
     *     solid: true
     * });
     * var overlayB = Overlays.addOverlay("cube", {
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0.3, y: 0, z: -3 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 0.3, y: 0.3, z: 0.3 },
     *     solid: true
     * });
     * var propertiesToGet = {};
     * propertiesToGet[overlayA] = ["color", "alpha"];
     * propertiesToGet[overlayB] = ["dimensions"];
     * var properties = Overlays.getOverlaysProperties(propertiesToGet);
     * print("Overlays properties: " + JSON.stringify(properties));
     */
/**
     * Find the closest 3D overlay intersected by a {@link PickRay}.
     * @function Overlays.findRayIntersection
     * @param {PickRay} pickRay - The PickRay to use for finding overlays.
     * @param {boolean} [precisionPicking=false] - <em>Unused</em>; exists to match Entity API.
     * @param {Array.<Uuid>} [overlayIDsToInclude=[]] - If not empty then the search is restricted to these overlays.
     * @param {Array.<Uuid>} [overlayIDsToExclude=[]] - Overlays to ignore during the search.
     * @param {boolean} [visibleOnly=false] - <em>Unused</em>; exists to match Entity API.
     * @param {boolean} [collidableOnly=false] - <em>Unused</em>; exists to match Entity API.
     * @returns {Overlays.RayToOverlayIntersectionResult} The closest 3D overlay intersected by <code>pickRay</code>, taking
     *     into account <code>overlayIDsToInclude</code> and <code>overlayIDsToExclude</code> if they're not empty.
     * @example <caption>Create a cube overlay in front of your avatar. Report 3D overlay intersection details for mouse 
     *     clicks.</caption>
     * var overlay = Overlays.addOverlay("cube", {
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -3 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 0.3, y: 0.3, z: 0.3 },
     *     solid: true
     * });
     *
     * Controller.mousePressEvent.connect(function (event) {
     *     var pickRay = Camera.computePickRay(event.x, event.y);
     *     var intersection = Overlays.findRayIntersection(pickRay);
     *     print("Intersection: " + JSON.stringify(intersection));
     * });
     */
/**
     * Return a list of 3D overlays with bounding boxes that touch a search sphere.
     * @function Overlays.findOverlays
     * @param {Vec3} center - The center of the search sphere.
     * @param {number} radius - The radius of the search sphere.
     * @returns {Uuid[]} An array of overlay IDs with bounding boxes that touch a search sphere.
     * @example <caption>Create two cube overlays in front of your avatar then search for overlays near your avatar.</caption>
     * var overlayA = Overlays.addOverlay("cube", {
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: -0.3, y: 0, z: -3 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 0.3, y: 0.3, z: 0.3 },
     *     solid: true
     * });
     * print("Overlay A: " + overlayA);
     * var overlayB = Overlays.addOverlay("cube", {
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0.3, y: 0, z: -3 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 0.3, y: 0.3, z: 0.3 },
     *     solid: true
     * });
     * print("Overlay B: " + overlayB);
     *
     * var overlaysFound = Overlays.findOverlays(MyAvatar.position, 5.0);
     * print("Overlays found: " + JSON.stringify(overlaysFound));
     */
/**
     * Check whether an overlay's assets have been loaded. For example, for an <code>image</code> overlay the result indicates
     * whether its image has been loaded.
     * @function Overlays.isLoaded
     * @param {Uuid} overlayID - The ID of the overlay to check.
     * @returns {boolean} <code>true</code> if the overlay's assets have been loaded, otherwise <code>false</code>.
     * @example <caption>Create an image overlay and report whether its image is loaded after 1s.</caption>
     * var overlay = Overlays.addOverlay("image", {
     *     bounds: { x: 100, y: 100, width: 200, height: 200 },
     *     imageURL: "https://content.highfidelity.com/DomainContent/production/Particles/wispy-smoke.png"
     * });
     * Script.setTimeout(function () {
     *     var isLoaded = Overlays.isLoaded(overlay);
     *     print("Image loaded: " + isLoaded);
     * }, 1000);
     */
/**
     * Calculates the size of the given text in the specified overlay if it is a text overlay.
     * @function Overlays.textSize
     * @param {Uuid} overlayID - The ID of the overlay to use for calculation.
     * @param {string} text - The string to calculate the size of.
     * @returns {Size} The size of the <code>text</code> if the overlay is a text overlay, otherwise
     *     <code>{ height: 0, width : 0 }</code>. If the overlay is a 2D overlay, the size is in pixels; if the overlay is a 3D
     *     overlay, the size is in meters.
     * @example <caption>Calculate the size of "hello" in a 3D text overlay.</caption>
     * var overlay = Overlays.addOverlay("text3d", {
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -2 })),
     *     rotation: MyAvatar.orientation,
     *     text: "hello",
     *     lineHeight: 0.2
     * });
     * var textSize = Overlays.textSize(overlay, "hello");
     * print("Size of \"hello\": " + JSON.stringify(textSize));
     */
/**
     * Get the width of the window or HUD.
     * @function Overlays.width
     * @returns {number} The width, in pixels, of the Interface window if in desktop mode or the HUD if in HMD mode.
     */
/**
     * Get the height of the window or HUD.
     * @function Overlays.height
     * @returns {number} The height, in pixels, of the Interface window if in desktop mode or the HUD if in HMD mode.
     */
/**
     * Check if there is an overlay of a given ID.
     * @function Overlays.isAddedOverly
     * @param {Uuid} overlayID - The ID to check.
     * @returns {boolean} <code>true</code> if an overlay with the given ID exists, <code>false</code> otherwise.
     */
/**
     * Generate a mouse press event on an overlay.
     * @function Overlays.sendMousePressOnOverlay
     * @param {Uuid} overlayID - The ID of the overlay to generate a mouse press event on.
     * @param {PointerEvent} event - The mouse press event details.
     * @example <caption>Create a 2D rectangle overlay plus a 3D cube overlay and generate mousePressOnOverlay events for the 2D
     * overlay.</caption>
     * var overlay = Overlays.addOverlay("cube", {
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -3 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 0.3, y: 0.3, z: 0.3 },
     *     solid: true
     * });
     * print("3D overlay: " + overlay);
     *
     * var overlay = Overlays.addOverlay("rectangle", {
     *     bounds: { x: 100, y: 100, width: 200, height: 100 },
     *     color: { red: 255, green: 255, blue: 255 }
     * });
     * print("2D overlay: " + overlay);
     *
     * // Overlays.mousePressOnOverlay by default applies only to 3D overlays.
     * Overlays.mousePressOnOverlay.connect(function(overlayID, event) {
     *     print("Clicked: " + overlayID);
     * });
     *
     * Controller.mousePressEvent.connect(function (event) {
     *     // Overlays.getOverlayAtPoint applies only to 2D overlays.
     *     var overlay = Overlays.getOverlayAtPoint({ x: event.x, y: event.y });
     *     if (overlay) {
     *         Overlays.sendMousePressOnOverlay(overlay, {
     *             type: "press",
     *             id: 0,
     *             pos2D: event
     *         });
     *     }
     * });
     */
/**
     * Generate a mouse release event on an overlay.
     * @function Overlays.sendMouseReleaseOnOverlay
     * @param {Uuid} overlayID - The ID of the overlay to generate a mouse release event on.
     * @param {PointerEvent} event - The mouse release event details.
     */
/**
     * Generate a mouse move event on an overlay.
     * @function Overlays.sendMouseMoveOnOverlay
     * @param {Uuid} overlayID - The ID of the overlay to generate a mouse move event on.
     * @param {PointerEvent} event - The mouse move event details.
     */
/**
     * Generate a hover enter event on an overlay.
     * @function Overlays.sendHoverEnterOverlay
     * @param {Uuid} id - The ID of the overlay to generate a hover enter event on.
     * @param {PointerEvent} event - The hover enter event details.
     */
/**
     * Generate a hover over event on an overlay.
     * @function Overlays.sendHoverOverOverlay
     * @param {Uuid} overlayID - The ID of the overlay to generate a hover over event on.
     * @param {PointerEvent} event - The hover over event details.
     */
/**
     * Generate a hover leave event on an overlay.
     * @function Overlays.sendHoverLeaveOverlay
     * @param {Uuid} overlayID - The ID of the overlay to generate a hover leave event on.
     * @param {PointerEvent} event - The hover leave event details.
     */
/**
     * Get the ID of the Web3D overlay that has keyboard focus.
     * @function Overlays.getKeyboardFocusOverlay
     * @returns {Uuid} The ID of the {@link Overlays.OverlayType|web3d} overlay that has focus, if any, otherwise 
     *     <code>null</code>.
     */
/**
     * Set the Web3D overlay that has keyboard focus.
     * @function Overlays.setKeyboardFocusOverlay
     * @param {Uuid} overlayID - The ID of the {@link Overlays.OverlayType|web3d} overlay to set keyboard focus to. Use 
     *     <code>null</code> or {@link Uuid|Uuid.NULL} to unset keyboard focus from an overlay.
     */
/**
     * Triggered when an overlay is deleted.
     * @function Overlays.overlayDeleted
     * @param {Uuid} overlayID - The ID of the overlay that was deleted.
     * @returns {Signal}
     * @example <caption>Create an overlay then delete it after 1s.</caption>
     * var overlay = Overlays.addOverlay("cube", {
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -3 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 0.3, y: 0.3, z: 0.3 },
     *     solid: true
     * });
     * print("Overlay: " + overlay);
     *
     * Overlays.overlayDeleted.connect(function(overlayID) {
     *     print("Deleted: " + overlayID);
     * });
     * Script.setTimeout(function () {
     *     Overlays.deleteOverlay(overlay);
     * }, 1000);
     */
/**
     * Triggered when a mouse press event occurs on an overlay. Only occurs for 3D overlays (unless you use 
     *     {@link Overlays.sendMousePressOnOverlay|sendMousePressOnOverlay} for a 2D overlay).
     * @function Overlays.mousePressOnOverlay
     * @param {Uuid} overlayID - The ID of the overlay the mouse press event occurred on.
     * @param {PointerEvent} event - The mouse press event details.
     * @returns {Signal}
     * @example <caption>Create a cube overlay in front of your avatar and report mouse clicks on it.</caption>
     * var overlay = Overlays.addOverlay("cube", {
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -3 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 0.3, y: 0.3, z: 0.3 },
     *     solid: true
     * });
     * print("My overlay: " + overlay);
     *
     * Overlays.mousePressOnOverlay.connect(function(overlayID, event) {
     *     if (overlayID === overlay) {
     *         print("Clicked on my overlay");
     *     }
     * });
     */
/**
     * Triggered when a mouse double press event occurs on an overlay. Only occurs for 3D overlays.
     * @function Overlays.mouseDoublePressOnOverlay
     * @param {Uuid} overlayID - The ID of the overlay the mouse double press event occurred on.
     * @param {PointerEvent} event - The mouse double press event details.
     * @returns {Signal}
     */
/**
     * Triggered when a mouse release event occurs on an overlay. Only occurs for 3D overlays (unless you use 
     *     {@link Overlays.sendMouseReleaseOnOverlay|sendMouseReleaseOnOverlay} for a 2D overlay).
     * @function Overlays.mouseReleaseOnOverlay
     * @param {Uuid} overlayID - The ID of the overlay the mouse release event occurred on.
     * @param {PointerEvent} event - The mouse release event details.
     * @returns {Signal}
     */
/**
     * Triggered when a mouse move event occurs on an overlay. Only occurs for 3D overlays (unless you use 
     *     {@link Overlays.sendMouseMoveOnOverlay|sendMouseMoveOnOverlay} for a 2D overlay).
     * @function Overlays.mouseMoveOnOverlay
     * @param {Uuid} overlayID - The ID of the overlay the mouse moved event occurred on.
     * @param {PointerEvent} event - The mouse move event details.
     * @returns {Signal}
     */
/**
     * Triggered when a mouse press event occurs on something other than a 3D overlay.
     * @function Overlays.mousePressOffOverlay
     * @returns {Signal}
     */
/**
     * Triggered when a mouse double press event occurs on something other than a 3D overlay.
     * @function Overlays.mouseDoublePressOffOverlay
     * @returns {Signal}
     */
/**
     * Triggered when a mouse cursor starts hovering over an overlay. Only occurs for 3D overlays (unless you use 
     *     {@link Overlays.sendHoverEnterOverlay|sendHoverEnterOverlay} for a 2D overlay).
     * @function Overlays.hoverEnterOverlay
     * @param {Uuid} overlayID - The ID of the overlay the mouse moved event occurred on.
     * @param {PointerEvent} event - The mouse move event details.
     * @returns {Signal}
     * @example <caption>Create a cube overlay in front of your avatar and report when you start hovering your mouse over
     *     it.</caption>
     * var overlay = Overlays.addOverlay("cube", {
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -3 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 0.3, y: 0.3, z: 0.3 },
     *     solid: true
     * });
     * print("Overlay: " + overlay);
     * Overlays.hoverEnterOverlay.connect(function(overlayID, event) {
     *     print("Hover enter: " + overlayID);
     * });
     */
/**
     * Triggered when a mouse cursor continues hovering over an overlay. Only occurs for 3D overlays (unless you use 
     *     {@link Overlays.sendHoverOverOverlay|sendHoverOverOverlay} for a 2D overlay).
     * @function Overlays.hoverOverOverlay
     * @param {Uuid} overlayID - The ID of the overlay the hover over event occurred on.
     * @param {PointerEvent} event - The hover over event details.
     * @returns {Signal}
     */
/**
     * Triggered when a mouse cursor finishes hovering over an overlay. Only occurs for 3D overlays (unless you use 
     *     {@link Overlays.sendHoverLeaveOverlay|sendHoverLeaveOverlay} for a 2D overlay).
     * @function Overlays.hoverLeaveOverlay
     * @param {Uuid} overlayID - The ID of the overlay the hover leave event occurred on.
     * @param {PointerEvent} event - The hover leave event details.
     * @returns {Signal}
     */
/**
 * @property {Vec2} dimensions=1,1 - The dimensions of the overlay. Synonyms: <code>scale</code>, <code>size</code>.
 */
/**
 * These are the properties of a <code>rectangle3d</code> {@link Overlays.OverlayType|OverlayType}.
 * @typedef {object} Overlays.Rectangle3DProperties
 * 
 * @property {string} type=rectangle3d - Has the value <code>"rectangle3d"</code>. <em>Read-only.</em>
 * @property {Color} color=255,255,255 - The color of the overlay.
 * @property {number} alpha=0.7 - The opacity of the overlay, <code>0.0</code> - <code>1.0</code>.
 * @property {number} pulseMax=0 - The maximum value of the pulse multiplier.
 * @property {number} pulseMin=0 - The minimum value of the pulse multiplier.
 * @property {number} pulsePeriod=1 - The duration of the color and alpha pulse, in seconds. A pulse multiplier value goes from
 *     <code>pulseMin</code> to <code>pulseMax</code>, then <code>pulseMax</code> to <code>pulseMin</code> in one period.
 * @property {number} alphaPulse=0 - If non-zero, the alpha of the overlay is pulsed: the alpha value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 * @property {number} colorPulse=0 - If non-zero, the color of the overlay is pulsed: the color value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 * @property {boolean} visible=true - If <code>true</code>, the overlay is rendered, otherwise it is not rendered.
 *
 * @property {string} name="" - A friendly name for the overlay.
 * @property {Vec3} position - The position of the overlay center. Synonyms: <code>p1</code>, <code>point</code>, and
 *     <code>start</code>.
 * @property {Vec3} localPosition - The local position of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>position</code>.
 * @property {Quat} rotation - The orientation of the overlay. Synonym: <code>orientation</code>.
 * @property {Quat} localRotation - The orientation of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>rotation</code>.
 * @property {boolean} isSolid=false - Synonyms: <ode>solid</code>, <code>isFilled</code>, and <code>filled</code>.
 *     Antonyms: <code>isWire</code> and <code>wire</code>.
 * @property {boolean} isDashedLine=false - If <code>true</code>, a dashed line is drawn on the overlay's edges. Synonym:
 *     <code>dashed</code>.
 * @property {boolean} ignoreRayIntersection=false - If <code>true</code>,
 *     {@link Overlays.findRayIntersection|findRayIntersection} ignores the overlay.
 * @property {boolean} drawInFront=false - If <code>true</code>, the overlay is rendered in front of other overlays that don't
 *     have <code>drawInFront</code> set to <code>true</code>, and in front of entities.
 * @property {boolean} grabbable=false - Signal to grabbing scripts whether or not this overlay can be grabbed.
 * @property {Uuid} parentID=null - The avatar, entity, or overlay that the overlay is parented to.
 * @property {number} parentJointIndex=65535 - Integer value specifying the skeleton joint that the overlay is attached to if
 *     <code>parentID</code> is an avatar skeleton. A value of <code>65535</code> means "no joint".
 * 
 * @property {Vec2} dimensions=1,1 - The dimensions of the overlay. Synonyms: <code>scale</code>, <code>size</code>.
 */
/**
 * These are the properties of a <code>rectangle</code> {@link Overlays.OverlayType|OverlayType}.
 * @typedef {object} Overlays.RectangleProperties
 *
 * @property {Rect} bounds - The position and size of the rectangle, in pixels. <em>Write-only.</em>
 * @property {number} x - Integer left, x-coordinate value = <code>bounds.x</code>. <em>Write-only.</em>
 * @property {number} y - Integer top, y-coordinate value = <code>bounds.y</code>. <em>Write-only.</em>
 * @property {number} width - Integer width of the rectangle = <code>bounds.width</code>. <em>Write-only.</em>
 * @property {number} height - Integer height of the rectangle = <code>bounds.height</code>. <em>Write-only.</em>
 *
 * @property {Color} color=0,0,0 - The color of the overlay. <em>Write-only.</em>
 * @property {number} alpha=1.0 - The opacity of the overlay, <code>0.0</code> - <code>1.0</code>. <em>Write-only.</em>
 * @property {number} borderWidth=1 - Integer width of the border, in pixels. The border is drawn within the rectangle's bounds.
 *     It is not drawn unless either <code>borderColor</code> or <code>borderAlpha</code> are specified. <em>Write-only.</em>
 * @property {number} radius=0 - Integer corner radius, in pixels. <em>Write-only.</em>
 * @property {Color} borderColor=0,0,0 - The color of the border. <em>Write-only.</em>
 * @property {number} borderAlpha=1.0 - The opacity of the border, <code>0.0</code> - <code>1.0</code>.
 *     <em>Write-only.</em>
 * @property {boolean} visible=true - If <code>true</code>, the overlay is rendered, otherwise it is not rendered.
 *     <em>Write-only.</em>
 */
/**
 * <p>A <code>shape</code> {@link Overlays.OverlayType|OverlayType} may display as one of the following geometrical shapes:</p>
 * <table>
 *   <thead>
 *     <tr><th>Value</th><th>Dimensions</th><th>Description</th></tr>
 *   </thead>
 *   <tbody>
 *     <tr><td><code>"Circle"</code></td><td>2D</td><td>A circle oriented in 3D.</td></td></tr>
 *     <tr><td><code>"Cone"</code></td><td>3D</td><td></td></tr>
 *     <tr><td><code>"Cube"</code></td><td>3D</td><td></td></tr>
 *     <tr><td><code>"Cylinder"</code></td><td>3D</td><td></td></tr>
 *     <tr><td><code>"Dodecahedron"</code></td><td>3D</td><td></td></tr>
 *     <tr><td><code>"Hexagon"</code></td><td>3D</td><td>A hexagonal prism.</td></tr>
 *     <tr><td><code>"Icosahedron"</code></td><td>3D</td><td></td></tr>
 *     <tr><td><code>"Line"</code></td><td>1D</td><td>A line oriented in 3D.</td></tr>
 *     <tr><td><code>"Octagon"</code></td><td>3D</td><td>An octagonal prism.</td></tr>
 *     <tr><td><code>"Octahedron"</code></td><td>3D</td><td></td></tr>
 *     <tr><td><code>"Quad"</code></td><td>2D</td><td>A square oriented in 3D.</tr>
 *     <tr><td><code>"Sphere"</code></td><td>3D</td><td></td></tr>
 *     <tr><td><code>"Tetrahedron"</code></td><td>3D</td><td></td></tr>
 *     <tr><td><code>"Torus"</code></td><td>3D</td><td><em>Not implemented.</em></td></tr>
 *     <tr><td><code>"Triangle"</code></td><td>3D</td><td>A triangular prism.</td></tr>
 *   </tbody>
 * </table>
 * @typedef {string} Overlays.Shape
 */
/**
 * These are the properties of a <code>shape</code> {@link Overlays.OverlayType|OverlayType}.
 * @typedef {object} Overlays.ShapeProperties
 *
 * @property {string} type=shape - Has the value <code>"shape"</code>. <em>Read-only.</em>
 * @property {Color} color=255,255,255 - The color of the overlay.
 * @property {number} alpha=0.7 - The opacity of the overlay, <code>0.0</code> - <code>1.0</code>.
 * @property {number} pulseMax=0 - The maximum value of the pulse multiplier.
 * @property {number} pulseMin=0 - The minimum value of the pulse multiplier.
 * @property {number} pulsePeriod=1 - The duration of the color and alpha pulse, in seconds. A pulse multiplier value goes from
 *     <code>pulseMin</code> to <code>pulseMax</code>, then <code>pulseMax</code> to <code>pulseMin</code> in one period.
 * @property {number} alphaPulse=0 - If non-zero, the alpha of the overlay is pulsed: the alpha value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 * @property {number} colorPulse=0 - If non-zero, the color of the overlay is pulsed: the color value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 * @property {boolean} visible=true - If <code>true</code>, the overlay is rendered, otherwise it is not rendered.
 *
 * @property {string} name="" - A friendly name for the overlay.
 * @property {Vec3} position - The position of the overlay center. Synonyms: <code>p1</code>, <code>point</code>, and
 *     <code>start</code>.
 * @property {Vec3} localPosition - The local position of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>position</code>.
 * @property {Quat} rotation - The orientation of the overlay. Synonym: <code>orientation</code>.
 * @property {Quat} localRotation - The orientation of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>rotation</code>.
 * @property {boolean} isSolid=false - Synonyms: <ode>solid</code>, <code>isFilled</code>, and <code>filled</code>.
 *     Antonyms: <code>isWire</code> and <code>wire</code>.
 * @property {boolean} isDashedLine=false - If <code>true</code>, a dashed line is drawn on the overlay's edges. Synonym:
 *     <code>dashed</code>.
 * @property {boolean} ignoreRayIntersection=false - If <code>true</code>,
 *     {@link Overlays.findRayIntersection|findRayIntersection} ignores the overlay.
 * @property {boolean} drawInFront=false - If <code>true</code>, the overlay is rendered in front of other overlays that don't
 *     have <code>drawInFront</code> set to <code>true</code>, and in front of entities.
 * @property {boolean} grabbable=false - Signal to grabbing scripts whether or not this overlay can be grabbed.
 * @property {Uuid} parentID=null - The avatar, entity, or overlay that the overlay is parented to.
 * @property {number} parentJointIndex=65535 - Integer value specifying the skeleton joint that the overlay is attached to if
 *     <code>parentID</code> is an avatar skeleton. A value of <code>65535</code> means "no joint".
 *
 * @property {Vec3} dimensions - The dimensions of the overlay. Synonyms: <code>scale</code>, <code>size</code>.
 *
 * @property {Overlays.Shape} shape=Hexagon - The geometrical shape of the overlay.
 */
/**
 * These are the properties of a <code>sphere</code> {@link Overlays.OverlayType|OverlayType}.
 * @typedef {object} Overlays.SphereProperties
 *
 * @property {string} type=sphere - Has the value <code>"sphere"</code>. <em>Read-only.</em>
 * @property {Color} color=255,255,255 - The color of the overlay.
 * @property {number} alpha=0.7 - The opacity of the overlay, <code>0.0</code> - <code>1.0</code>.
 * @property {number} pulseMax=0 - The maximum value of the pulse multiplier.
 * @property {number} pulseMin=0 - The minimum value of the pulse multiplier.
 * @property {number} pulsePeriod=1 - The duration of the color and alpha pulse, in seconds. A pulse multiplier value goes from
 *     <code>pulseMin</code> to <code>pulseMax</code>, then <code>pulseMax</code> to <code>pulseMin</code> in one period.
 * @property {number} alphaPulse=0 - If non-zero, the alpha of the overlay is pulsed: the alpha value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 * @property {number} colorPulse=0 - If non-zero, the color of the overlay is pulsed: the color value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 * @property {boolean} visible=true - If <code>true</code>, the overlay is rendered, otherwise it is not rendered.
 *
 * @property {string} name="" - A friendly name for the overlay.
 * @property {Vec3} position - The position of the overlay center. Synonyms: <code>p1</code>, <code>point</code>, and
 *     <code>start</code>.
 * @property {Vec3} localPosition - The local position of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>position</code>.
 * @property {Quat} rotation - The orientation of the overlay. Synonym: <code>orientation</code>.
 * @property {Quat} localRotation - The orientation of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>rotation</code>.
 * @property {boolean} isSolid=false - Synonyms: <ode>solid</code>, <code>isFilled</code>, and <code>filled</code>.
 *     Antonyms: <code>isWire</code> and <code>wire</code>.
 * @property {boolean} isDashedLine=false - If <code>true</code>, a dashed line is drawn on the overlay's edges. Synonym:
 *     <code>dashed</code>.
 * @property {boolean} ignoreRayIntersection=false - If <code>true</code>,
 *     {@link Overlays.findRayIntersection|findRayIntersection} ignores the overlay.
 * @property {boolean} drawInFront=false - If <code>true</code>, the overlay is rendered in front of other overlays that don't
 *     have <code>drawInFront</code> set to <code>true</code>, and in front of entities.
 * @property {boolean} grabbable=false - Signal to grabbing scripts whether or not this overlay can be grabbed.
 * @property {Uuid} parentID=null - The avatar, entity, or overlay that the overlay is parented to.
 * @property {number} parentJointIndex=65535 - Integer value specifying the skeleton joint that the overlay is attached to if
 *     <code>parentID</code> is an avatar skeleton. A value of <code>65535</code> means "no joint".
 *
 * @property {Vec3} dimensions - The dimensions of the overlay. Synonyms: <code>scale</code>, <code>size</code>.
 */
/**
 * These are the properties of a <code>text3d</code> {@link Overlays.OverlayType|OverlayType}.
 * @typedef {object} Overlays.Text3DProperties
 *
 * @property {string} type=text3d - Has the value <code>"text3d"</code>. <em>Read-only.</em>
 * @property {Color} color=255,255,255 - The color of the overlay.
 * @property {number} alpha=0.7 - The opacity of the overlay, <code>0.0</code> - <code>1.0</code>.
 * @property {number} pulseMax=0 - The maximum value of the pulse multiplier.
 * @property {number} pulseMin=0 - The minimum value of the pulse multiplier.
 * @property {number} pulsePeriod=1 - The duration of the color and alpha pulse, in seconds. A pulse multiplier value goes from
 *     <code>pulseMin</code> to <code>pulseMax</code>, then <code>pulseMax</code> to <code>pulseMin</code> in one period.
 * @property {number} alphaPulse=0 - If non-zero, the alpha of the overlay is pulsed: the alpha value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 * @property {number} colorPulse=0 - If non-zero, the color of the overlay is pulsed: the color value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 * @property {boolean} visible=true - If <code>true</code>, the overlay is rendered, otherwise it is not rendered.
 *
 * @property {string} name="" - A friendly name for the overlay.
 * @property {Vec3} position - The position of the overlay center. Synonyms: <code>p1</code>, <code>point</code>, and
 *     <code>start</code>.
 * @property {Vec3} localPosition - The local position of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>position</code>.
 * @property {Quat} rotation - The orientation of the overlay. Synonym: <code>orientation</code>.
 * @property {Quat} localRotation - The orientation of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>rotation</code>.
 * @property {boolean} isSolid=false - Synonyms: <ode>solid</code>, <code>isFilled</code>, and <code>filled</code>.
 *     Antonyms: <code>isWire</code> and <code>wire</code>.
 * @property {boolean} isDashedLine=false - If <code>true</code>, a dashed line is drawn on the overlay's edges. Synonym:
 *     <code>dashed</code>.
 * @property {boolean} ignoreRayIntersection=false - If <code>true</code>,
 *     {@link Overlays.findRayIntersection|findRayIntersection} ignores the overlay.
 * @property {boolean} drawInFront=false - If <code>true</code>, the overlay is rendered in front of other overlays that don't
 *     have <code>drawInFront</code> set to <code>true</code>, and in front of entities.
 * @property {boolean} grabbable=false - Signal to grabbing scripts whether or not this overlay can be grabbed.
 * @property {Uuid} parentID=null - The avatar, entity, or overlay that the overlay is parented to.
 * @property {number} parentJointIndex=65535 - Integer value specifying the skeleton joint that the overlay is attached to if
 *     <code>parentID</code> is an avatar skeleton. A value of <code>65535</code> means "no joint".
 *
 * @property {Vec2} dimensions=1,1 - The dimensions of the overlay. Synonyms: <code>scale</code>, <code>size</code>.
 *
 * @property {boolean} isFacingAvatar - If <code>true</code>, the overlay is rotated to face the user's camera about an axis
 *     parallel to the user's avatar's "up" direction.
 *
 * @property {string} text="" - The text to display. Text does not automatically wrap; use <code>\n</code> for a line break.
 * @property {number} textAlpha=1 - The text alpha value.
 * @property {Color} backgroundColor=0,0,0 - The background color.
 * @property {number} backgroundAlpha=0.7 - The background alpha value.
 * @property {number} lineHeight=1 - The height of a line of text in meters.
 * @property {number} leftMargin=0.1 - The left margin, in meters.
 * @property {number} topMargin=0.1 - The top margin, in meters.
 * @property {number} rightMargin=0.1 - The right margin, in meters.
 * @property {number} bottomMargin=0.1 - The bottom margin, in meters.
 */
/**
 * These are the properties of a <code>text</code> {@link Overlays.OverlayType|OverlayType}.
 * @typedef {object} Overlays.TextProperties
 *
 * @property {Rect} bounds - The position and size of the rectangle, in pixels. <em>Write-only.</em>
 * @property {number} x - Integer left, x-coordinate value = <code>bounds.x</code>. <em>Write-only.</em>
 * @property {number} y - Integer top, y-coordinate value = <code>bounds.y</code>. <em>Write-only.</em>
 * @property {number} width - Integer width of the rectangle = <code>bounds.width</code>. <em>Write-only.</em>
 * @property {number} height - Integer height of the rectangle = <code>bounds.height</code>. <em>Write-only.</em>
 *
 * @property {number} margin=0 - Sets the <code>leftMargin</code> and <code>topMargin</code> values, in pixels.
 *     <em>Write-only.</em>
 * @property {number} leftMargin=0 - The left margin's size, in pixels. This value is also used for the right margin. 
 *     <em>Write-only.</em>
 * @property {number} topMargin=0 - The top margin's size, in pixels. This value is also used for the bottom margin. 
 *     <em>Write-only.</em>
 * @property {string} text="" - The text to display. Text does not automatically wrap; use <code>\n</code> for a line break. Text
 *     is clipped to the <code>bounds</code>. <em>Write-only.</em>
 * @property {number} font.size=18 - The size of the text, in pixels. <em>Write-only.</em>
 * @property {number} lineHeight=18 - The height of a line of text, in pixels. <em>Write-only.</em>
 * @property {Color} color=255,255,255 - The color of the text. Synonym: <code>textColor</code>. <em>Write-only.</em>
 * @property {number} alpha=1.0 - The opacity of the overlay, <code>0.0</code> - <code>1.0</code>. <em>Write-only.</em>
 * @property {Color} backgroundColor=0,0,0 - The color of the background rectangle. <em>Write-only.</em>
 * @property {number} backgroundAlpha=0.7 - The opacity of the background rectangle. <em>Write-only.</em>
 * @property {boolean} visible=true - If <code>true</code>, the overlay is rendered, otherwise it is not rendered.
 *     <em>Write-only.</em>
 */
/**
 * @typedef
 * @property {Vec3} dimensions - The dimensions of the overlay. Synonyms: <code>scale</code>, <code>size</code>.
 */
/**
 * These are the properties of a <code>web3d</code> {@link Overlays.OverlayType|OverlayType}.
 * @typedef {object} Overlays.Web3DProperties
 *
 * @property {string} type=web3d - Has the value <code>"web3d"</code>. <em>Read-only.</em>
 * @property {Color} color=255,255,255 - The color of the overlay.
 * @property {number} alpha=0.7 - The opacity of the overlay, <code>0.0</code> - <code>1.0</code>.
 * @property {number} pulseMax=0 - The maximum value of the pulse multiplier.
 * @property {number} pulseMin=0 - The minimum value of the pulse multiplier.
 * @property {number} pulsePeriod=1 - The duration of the color and alpha pulse, in seconds. A pulse multiplier value goes from
 *     <code>pulseMin</code> to <code>pulseMax</code>, then <code>pulseMax</code> to <code>pulseMin</code> in one period.
 * @property {number} alphaPulse=0 - If non-zero, the alpha of the overlay is pulsed: the alpha value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 * @property {number} colorPulse=0 - If non-zero, the color of the overlay is pulsed: the color value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 * @property {boolean} visible=true - If <code>true</code>, the overlay is rendered, otherwise it is not rendered.
 *
 * @property {string} name="" - A friendly name for the overlay.
 * @property {Vec3} position - The position of the overlay center. Synonyms: <code>p1</code>, <code>point</code>, and 
 *     <code>start</code>.
 * @property {Vec3} localPosition - The local position of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>position</code>.
 * @property {Quat} rotation - The orientation of the overlay. Synonym: <code>orientation</code>.
 * @property {Quat} localRotation - The orientation of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>rotation</code>.
 * @property {boolean} isSolid=false - Synonyms: <ode>solid</code>, <code>isFilled</code>, and <code>filled</code>.
 *     Antonyms: <code>isWire</code> and <code>wire</code>.
 * @property {boolean} isDashedLine=false - If <code>true</code>, a dashed line is drawn on the overlay's edges. Synonym:
 *     <code>dashed</code>.
 * @property {boolean} ignoreRayIntersection=false - If <code>true</code>, 
 *     {@link Overlays.findRayIntersection|findRayIntersection} ignores the overlay.
 * @property {boolean} drawInFront=false - If <code>true</code>, the overlay is rendered in front of other overlays that don't
 *     have <code>drawInFront</code> set to <code>true</code>, and in front of entities.
 * @property {boolean} grabbable=false - Signal to grabbing scripts whether or not this overlay can be grabbed.
 * @property {Uuid} parentID=null - The avatar, entity, or overlay that the overlay is parented to.
 * @property {number} parentJointIndex=65535 - Integer value specifying the skeleton joint that the overlay is attached to if
 *     <code>parentID</code> is an avatar skeleton. A value of <code>65535</code> means "no joint".
 *
 * @property {boolean} isFacingAvatar - If <code>true</code>, the overlay is rotated to face the user's camera about an axis
 *     parallel to the user's avatar's "up" direction.
 *
 * @property {string} url - The URL of the Web page to display.
 * @property {string} scriptURL="" - The URL of a JavaScript file to inject into the Web page.
 * @property {number} dpi=30 - The dots per inch to display the Web page at, on the overlay.
 * @property {Vec2} dimensions=1,1 - The size of the overlay to display the Web page on, in meters. Synonyms: 
 *     <code>scale</code>, <code>size</code>.
 * @property {number} maxFPS=10 - The maximum update rate for the Web overlay content, in frames/second.
 * @property {boolean} showKeyboardFocusHighlight=true - If <code>true</code>, the Web overlay is highlighted when it has
 *     keyboard focus.
 * @property {string} inputMode=Touch - The user input mode to use - either <code>"Touch"</code> or <code>"Mouse"</code>.
 */
/**
 * The Picks API lets you create and manage objects for repeatedly calculating intersections in different ways.
 *
 * @namespace Picks
 * @property PICK_NOTHING {number} A filter flag.  Don't intersect with anything.
 * @property PICK_ENTITIES {number} A filter flag.  Include entities when intersecting.
 * @property PICK_OVERLAYS {number} A filter flag.  Include overlays when intersecting.
 * @property PICK_AVATARS {number} A filter flag.  Include avatars when intersecting.
 * @property PICK_HUD {number} A filter flag.  Include the HUD sphere when intersecting in HMD mode.
 * @property PICK_COARSE {number} A filter flag.  Pick against coarse meshes, instead of exact meshes.
 * @property PICK_INCLUDE_INVISIBLE {number} A filter flag.  Include invisible objects when intersecting.
 * @property PICK_INCLUDE_NONCOLLIDABLE {number} A filter flag.  Include non-collidable objects when intersecting.
 * @property INTERSECTED_NONE {number} An intersection type.  Intersected nothing with the given filter flags.
 * @property INTERSECTED_ENTITY {number} An intersection type.  Intersected an entity.
 * @property INTERSECTED_OVERLAY {number} An intersection type.  Intersected an overlay.
 * @property INTERSECTED_AVATAR {number} An intersection type.  Intersected an avatar.
 * @property INTERSECTED_HUD {number} An intersection type.  Intersected the HUD sphere.
 */
/**
     * A set of properties that can be passed to {@link Picks.createPick} to create a new Pick.
     *
     * Different {@link Picks.PickType}s use different properties, and within one PickType, the properties you choose can lead to a wide range of behaviors.  For example,
     *   with PickType.Ray, depending on which optional parameters you pass, you could create a Static Ray Pick, a Mouse Ray Pick, or a Joint Ray Pick.
     *
     * @typedef {Object} Picks.PickProperties
     * @property {boolean} [enabled=false] If this Pick should start enabled or not.  Disabled Picks do not updated their pick results.
     * @property {number} [filter=Picks.PICK_NOTHING] The filter for this Pick to use, constructed using filter flags combined using bitwise OR.
     * @property {float} [maxDistance=0.0] The max distance at which this Pick will intersect.  0.0 = no max.  < 0.0 is invalid.
     * @property {string} [joint] Only for Joint or Mouse Ray Picks.  If "Mouse", it will create a Ray Pick that follows the system mouse, in desktop or HMD.
     *   If "Avatar", it will create a Joint Ray Pick that follows your avatar's head.  Otherwise, it will create a Joint Ray Pick that follows the given joint, if it
     *   exists on your current avatar.
     * @property {Vec3} [posOffset=Vec3.ZERO] Only for Joint Ray Picks.  A local joint position offset, in meters.  x = upward, y = forward, z = lateral
     * @property {Vec3} [dirOffset=Vec3.UP] Only for Joint Ray Picks.  A local joint direction offset.  x = upward, y = forward, z = lateral
     * @property {Vec3} [position] Only for Static Ray Picks.  The world-space origin of the ray.
     * @property {Vec3} [direction=-Vec3.UP] Only for Static Ray Picks.  The world-space direction of the ray.
     * @property {number} [hand=-1] Only for Stylus Picks.  An integer.  0 == left, 1 == right.  Invalid otherwise.
     */
/**
     * Adds a new Pick.
     * @function Picks.createPick
     * @param {Picks.PickType} type A PickType that specifies the method of picking to use
     * @param {Picks.PickProperties} properties A PickProperties object, containing all the properties for initializing this Pick
     * @returns {number} The ID of the created Pick.  Used for managing the Pick.  0 if invalid.
     */
/**
     * Enables a Pick.
     * @function Picks.enablePick
     * @param {number} uid The ID of the Pick, as returned by {@link Picks.createPick}.
     */
/**
     * Disables a Pick.
     * @function Picks.disablePick
     * @param {number} uid The ID of the Pick, as returned by {@link Picks.createPick}.
     */
/**
     * Removes a Pick.
     * @function Picks.removePick
     * @param {number} uid The ID of the Pick, as returned by {@link Picks.createPick}.
     */
/**
     * An intersection result for a Ray Pick.
     *
     * @typedef {Object} Picks.RayPickResult
     * @property {number} type The intersection type.
     * @property {bool} intersects If there was a valid intersection (type != INTERSECTED_NONE)
     * @property {Uuid} objectID The ID of the intersected object.  Uuid.NULL for the HUD or invalid intersections.
     * @property {float} distance The distance to the intersection point from the origin of the ray.
     * @property {Vec3} intersection The intersection point in world-space.
     * @property {Vec3} surfaceNormal The surface normal at the intersected point.  All NANs if type == INTERSECTED_HUD.
     * @property {Variant} extraInfo Additional intersection details when available for Model objects.
     * @property {PickRay} searchRay The PickRay that was used.  Valid even if there was no intersection.
     */
/**
     * An intersection result for a Stylus Pick.
     *
     * @typedef {Object} Picks.StylusPickResult
     * @property {number} type The intersection type.
     * @property {bool} intersects If there was a valid intersection (type != INTERSECTED_NONE)
     * @property {Uuid} objectID The ID of the intersected object.  Uuid.NULL for the HUD or invalid intersections.
     * @property {float} distance The distance to the intersection point from the origin of the ray.
     * @property {Vec3} intersection The intersection point in world-space.
     * @property {Vec3} surfaceNormal The surface normal at the intersected point.  All NANs if type == INTERSECTED_HUD.
     * @property {Variant} extraInfo Additional intersection details when available for Model objects.
     * @property {StylusTip} stylusTip The StylusTip that was used.  Valid even if there was no intersection.
     */
/**
     * Get the most recent pick result from this Pick.  This will be updated as long as the Pick is enabled.
     * @function Picks.getPrevPickResult
     * @param {number} uid The ID of the Pick, as returned by {@link Picks.createPick}.
     * @returns {PickResult} The most recent intersection result.  This will be slightly different for different PickTypes.  See {@link Picks.RayPickResult} and {@link Picks.StylusPickResult}.
     */
/**
     * Sets whether or not to use precision picking.
     * @function Picks.setPrecisionPicking
     * @param {number} uid The ID of the Pick, as returned by {@link Picks.createPick}.
     * @param {boolean} precisionPicking Whether or not to use precision picking
     */
/**
     * Sets a list of Entity IDs, Overlay IDs, and/or Avatar IDs to ignore during intersection.  Not used by Stylus Picks.
     * @function Picks.setIgnoreItems
     * @param {number} uid The ID of the Pick, as returned by {@link Picks.createPick}.
     * @param {Uuid[]} ignoreItems A list of IDs to ignore.
     */
/**
     * Sets a list of Entity IDs, Overlay IDs, and/or Avatar IDs to include during intersection, instead of intersecting with everything.  Stylus
     *   Picks <b>only</b> intersect with objects in their include list.
     * @function Picks.setIncludeItems
     * @param {number} uid The ID of the Pick, as returned by {@link Picks.createPick}.
     * @param {Uuid[]} includeItems A list of IDs to include.
     */
/**
     * Check if a Pick is associated with the left hand.
     * @function Picks.isLeftHand
     * @param {number} uid The ID of the Pick, as returned by {@link Picks.createPick}.
     * @returns {boolean} True if the Pick is a Joint Ray Pick with joint == "_CONTROLLER_LEFTHAND" or "_CAMERA_RELATIVE_CONTROLLER_LEFTHAND", or a Stylus Pick with hand == 0.
     */
/**
     * Check if a Pick is associated with the right hand.
     * @function Picks.isRightHand
     * @param {number} uid The ID of the Pick, as returned by {@link Picks.createPick}.
     * @returns {boolean} True if the Pick is a Joint Ray Pick with joint == "_CONTROLLER_RIGHTHAND" or "_CAMERA_RELATIVE_CONTROLLER_RIGHTHAND", or a Stylus Pick with hand == 1.
     */
/**
     * Check if a Pick is associated with the system mouse.
     * @function Picks.isMouse
     * @param {number} uid The ID of the Pick, as returned by {@link Picks.createPick}.
     * @returns {boolean} True if the Pick is a Mouse Ray Pick, false otherwise.
     */
/**
     * The max number of usec to spend per frame updating Pick results.
     * @typedef {number} Picks.perFrameTimeBudget
     */
/**
 * The Pointers API lets you create and manage objects for repeatedly calculating intersections in different ways, as well as the visual representation of those objects.
 *  Pointers can also be configured to automatically generate PointerEvents.
 *
 * @namespace Pointers
 */
/**
     * A set of properties that can be passed to {@link Pointers.createPointer} to create a new Pointer.  Also contains the relevant {@link Picks.PickProperties} to define the underlying Pick.
     *
     * Different {@link PickType}s use different properties, and within one PickType, the properties you choose can lead to a wide range of behaviors.  For example,
     *   with PickType.Ray, depending on which optional parameters you pass, you could create a Static Ray Pointer, a Mouse Ray Pointer, or a Joint Ray Pointer.
     *
     * @typedef {Object} Pointers.PointerProperties
     * @property {boolean} [hover=false] If this Pointer should generate hover events.
     * @property {boolean} [faceAvatar=false] Ray Pointers only.  If true, the end of the Pointer will always rotate to face the avatar.
     * @property {boolean} [centerEndY=true] Ray Pointers only.  If false, the end of the Pointer will be moved up by half of its height.
     * @property {boolean} [lockEnd=false] Ray Pointers only.  If true, the end of the Pointer will lock on to the center of the object at which the laser is pointing.
     * @property {boolean} [distanceScaleEnd=false] Ray Pointers only.  If true, the dimensions of the end of the Pointer will scale linearly with distance.
     * @property {boolean} [scaleWithAvatar=false] Ray Pointers only.  If true, the width of the Pointer's path will scale linearly with your avatar's scale.
     * @property {Pointers.RayPointerRenderState[]} [renderStates] Ray Pointers only.  A list of different visual states to switch between.
     * @property {Pointers.DefaultRayPointerRenderState[]} [defaultRenderStates] Ray Pointers only.  A list of different visual states to use if there is no intersection.
     * @property {Pointers.Trigger[]} [triggers] Ray Pointers only.  A list of different triggers mechanisms that control this Pointer's click event generation.
     */
/**
     * A set of properties used to define the visual aspect of a Ray Pointer in the case that the Pointer is intersecting something.
     *
     * @typedef {Object} Pointers.RayPointerRenderState
     * @property {string} name The name of this render state, used by {@link Pointers.setRenderState} and {@link Pointers.editRenderState}
     * @property {OverlayProperties} [start] All of the properties you would normally pass to {@Overlays.addOverlay}, plus the type (as a <code>type</code> field).
     * An overlay to represent the beginning of the Ray Pointer, if desired.
     * @property {OverlayProperties} [path] All of the properties you would normally pass to {@Overlays.addOverlay}, plus the type (as a <code>type</code> field), which <b>must</b> be "line3d".
     * An overlay to represent the path of the Ray Pointer, if desired.
     * @property {OverlayProperties} [end] All of the properties you would normally pass to {@Overlays.addOverlay}, plus the type (as a <code>type</code> field).
     * An overlay to represent the end of the Ray Pointer, if desired.
     */
/**
     * A set of properties used to define the visual aspect of a Ray Pointer in the case that the Pointer is not intersecting something.  Same as a {@link Pointers.RayPointerRenderState},
     * but with an additional distance field.
     *
     * @typedef {Object} Pointers.DefaultRayPointerRenderState
     * @augments Pointers.RayPointerRenderState
     * @property {number} distance The distance at which to render the end of this Ray Pointer, if one is defined.
     */
/**
     * A trigger mechanism for Ray Pointers.
     *
     * @typedef {Object} Pointers.Trigger
     * @property {Controller.Action} action This can be a built-in Controller action, like Controller.Standard.LTClick, or a function that evaluates to >= 1.0 when you want to trigger <code>button</code>.
     * @property {string} button Which button to trigger.  "Primary", "Secondary", "Tertiary", and "Focus" are currently supported.  Only "Primary" will trigger clicks on web surfaces.  If "Focus" is triggered,
     * it will try to set the entity or overlay focus to the object at which the Pointer is aimed.  Buttons besides the first three will still trigger events, but event.button will be "None".
     */
/**
     * Adds a new Pointer
     * @function Pointers.createPointer
     * @param {Picks.PickType} type A PickType that specifies the method of picking to use
     * @param {Pointers.PointerProperties} properties A PointerProperties object, containing all the properties for initializing this Pointer <b>and</b> the {@link Picks.PickProperties} for the Pick that
     *   this Pointer will use to do its picking.
     * @returns {number} The ID of the created Pointer.  Used for managing the Pointer.  0 if invalid.
     *
     * @example <caption>Create a left hand Ray Pointer that triggers events on left controller trigger click and changes color when it's intersecting something.</caption>
     *
     * var end = {
     *     type: "sphere",
     *     dimensions: {x:0.5, y:0.5, z:0.5},
     *     solid: true,
     *     color: {red:0, green:255, blue:0},
     *     ignoreRayIntersection: true
     * };
     * var end2 = {
     *     type: "sphere",
     *     dimensions: {x:0.5, y:0.5, z:0.5},
     *     solid: true,
     *     color: {red:255, green:0, blue:0},
     *     ignoreRayIntersection: true
     * };
     *
     * var renderStates = [ {name: "test", end: end} ];
     * var defaultRenderStates = [ {name: "test", distance: 10.0, end: end2} ];
     * var pointer = Pointers.createPointer(PickType.Ray, {
     *     joint: "_CAMERA_RELATIVE_CONTROLLER_LEFTHAND",
     *     filter: Picks.PICK_OVERLAYS | Picks.PICK_ENTITIES | Picks.PICK_INCLUDE_NONCOLLIDABLE,
     *     renderStates: renderStates,
     *     defaultRenderStates: defaultRenderStates,
     *     distanceScaleEnd: true,
     *     triggers: [ {action: Controller.Standard.LTClick, button: "Focus"}, {action: Controller.Standard.LTClick, button: "Primary"} ],
     *     hover: true,
     *     enabled: true
     * });
     * Pointers.setRenderState(pointer, "test");
     */
/**
     * Enables a Pointer.
     * @function Pointers.enablePointer
     * @param {number} uid The ID of the Pointer, as returned by {@link Pointers.createPointer}.
     */
/**
     * Disables a Pointer.
     * @function Pointers.disablePointer
     * @param {number} uid The ID of the Pointer, as returned by {@link Pointers.createPointer}.
     */
/**
     * Removes a Pointer.
     * @function Pointers.removePointer
     * @param {number} uid The ID of the Pointer, as returned by {@link Pointers.createPointer}.
     */
/**
     * Edit some visual aspect of a Pointer.  Currently only supported for Ray Pointers.
     * @function Pointers.editRenderState
     * @param {number} uid The ID of the Pointer, as returned by {@link Pointers.createPointer}.
     * @param {string} renderState The name of the render state you want to edit.
     * @param {RenderState} properties The new properties for <code>renderState</code>.  For Ray Pointers, a {@link Pointers.RayPointerRenderState}.
     */
/**
     * Set the render state of a Pointer.  For Ray Pointers, this means switching between their {@link Pointers.RayPointerRenderState}s, or "" to turn off rendering and hover/trigger events.
     *  For Stylus Pointers, there are three built-in options: "events on" (render and send events, the default), "events off" (render but don't send events), and "disabled" (don't render, don't send events).
     * @function Pointers.setRenderState
     * @param {number} uid The ID of the Pointer, as returned by {@link Pointers.createPointer}.
     * @param {string} renderState The name of the render state to which you want to switch.
     */
/**
     * Get the most recent pick result from this Pointer.  This will be updated as long as the Pointer is enabled, regardless of the render state.
     * @function Pointers.getPrevPickResult
     * @param {number} uid The ID of the Pointer, as returned by {@link Pointers.createPointer}.
     * @returns {PickResult} The most recent intersection result.  This will be slightly different for different PickTypes.  See {@link Picks.RayPickResult} and {@link Picks.StylusPickResult}.
     */
/**
     * Sets whether or not to use precision picking.
     * @function Pointers.setPrecisionPicking
     * @param {number} uid The ID of the Pointer, as returned by {@link Pointers.createPointer}.
     * @param {boolean} precisionPicking Whether or not to use precision picking
     */
/**
     * Sets the length of this Pointer.  No effect on Stylus Pointers.
     * @function Pointers.setLength
     * @param {number} uid The ID of the Pointer, as returned by {@link Pointers.createPointer}.
     * @param {float} length The desired length of the Pointer.
     */
/**
     * Sets a list of Entity IDs, Overlay IDs, and/or Avatar IDs to ignore during intersection.  Not used by Stylus Pointers.
     * @function Pointers.setIgnoreItems
     * @param {number} uid The ID of the Pointer, as returned by {@link Pointers.createPointer}.
     * @param {Uuid[]} ignoreItems A list of IDs to ignore.
     */
/**
     * Sets a list of Entity IDs, Overlay IDs, and/or Avatar IDs to include during intersection, instead of intersecting with everything.  Stylus
     *   Pointers <b>only</b> intersect with objects in their include list.
     * @function Pointers.setIncludeItems
     * @param {number} uid The ID of the Pointer, as returned by {@link Pointers.createPointer}.
     * @param {Uuid[]} includeItems A list of IDs to include.
     */
/**
     * Lock a Pointer onto a specific object (overlay, entity, or avatar).  Optionally, provide an offset in object-space, otherwise the Pointer will lock on to the center of the object.
     *   Not used by Stylus Pointers.
     * @function Pointers.setLockEndUUID
     * @param {number} uid The ID of the Pointer, as returned by {@link Pointers.createPointer}.
     * @param {QUuid} objectID The ID of the object to which to lock on.
     * @param {boolean} isOverlay False for entities or avatars, true for overlays
     * @param {Mat4} [offsetMat] The offset matrix to use if you do not want to lock on to the center of the object.
     */
/**
     * Check if a Pointer is associated with the left hand.
     * @function Pointers.isLeftHand
     * @param {number} uid The ID of the Pointer, as returned by {@link Pointers.createPointer}.
     * @returns {boolean} True if the Pointer is a Joint Ray Pointer with joint == "_CONTROLLER_LEFTHAND" or "_CAMERA_RELATIVE_CONTROLLER_LEFTHAND", or a Stylus Pointer with hand == 0
     */
/**
     * Check if a Pointer is associated with the right hand.
     * @function Pointers.isRightHand
     * @param {number} uid The ID of the Pointer, as returned by {@link Pointers.createPointer}.
     * @returns {boolean} True if the Pointer is a Joint Ray Pointer with joint == "_CONTROLLER_RIGHTHAND" or "_CAMERA_RELATIVE_CONTROLLER_RIGHTHAND", or a Stylus Pointer with hand == 1
     */
/**
     * Check if a Pointer is associated with the system mouse.
     * @function Pointers.isMouse
     * @param {number} uid The ID of the Pointer, as returned by {@link Pointers.createPointer}.
     * @returns {boolean} True if the Pointer is a Mouse Ray Pointer, false otherwise.
     */
/**
     * @namespace AnimationCache
     * @augments ResourceCache
     */
/**
     * Returns animation resource for particular animation
     * @function AnimationCache.getAnimation
     * @param url {string} url to load
     * @return {Resource} animation
     */
/**
     * returns the minimum scale allowed for this avatar in the current domain.
     * This value can change as the user changes avatars or when changing domains.
     * @function AvatarData.getDomainMinScale
     * @returns {number} minimum scale allowed for this avatar in the current domain.
     */
/**
     * returns the maximum scale allowed for this avatar in the current domain.
     * This value can change as the user changes avatars or when changing domains.
     * @function AvatarData.getDomainMaxScale
     * @returns {number} maximum scale allowed for this avatar in the current domain.
     */
/**
     * Provides read only access to the current eye height of the avatar.
     * This height is only an estimate and might be incorrect for avatars that are missing standard joints.
     * @function AvatarData.getEyeHeight
     * @returns {number} eye height of avatar in meters
     */
/**
     * Provides read only access to the current height of the avatar.
     * This height is only an estimate and might be incorrect for avatars that are missing standard joints.
     * @function AvatarData.getHeight
     * @returns {number} height of avatar in meters
     */
/**
     * @typedef {object} Graphics.MaterialLayer
     * @property {Material} material - This layer's material.
     * @property {number} priority - The priority of this layer.  If multiple materials are applied to a mesh part, only the highest priority layer is used.
     */
/**
 * The experimental Graphics API <em>(experimental)</em> lets you query and manage certain graphics-related structures (like underlying meshes and textures) from scripting.
 * @namespace Graphics
 */
/**
     * Returns a model reference object associated with the specified UUID ({@link EntityID}, {@link OverlayID}, or {@link AvatarID}).
     *
     * @function Graphics.getModel
     * @param {UUID} entityID - The objectID of the model whose meshes are to be retrieved.
     * @return {Graphics.Model} the resulting Model object
     */
/**
     * Create a new Mesh / Mesh Part with the specified data buffers.
     *
     * @function Graphics.newMesh
     * @param {Graphics.IFSData} ifsMeshData Index-Faced Set (IFS) arrays used to create the new mesh.
     * @return {Graphics.Mesh} the resulting Mesh / Mesh Part object
     */
/**
    * @typedef {object} Graphics.IFSData
    * @property {string} [name] - mesh name (useful for debugging / debug prints).
    * @property {number[]} indices - vertex indices to use for the mesh faces.
    * @property {Vec3[]} vertices - vertex positions (model space)
    * @property {Vec3[]} [normals] - vertex normals (normalized)
    * @property {Vec3[]} [colors] - vertex colors (normalized)
    * @property {Vec2[]} [texCoords0] - vertex texture coordinates (normalized)
    */
/**
     * @typedef {object} Graphics.Mesh
     * @property {Graphics.MeshPart[]} parts - Array of submesh part references.
     * @property {string[]} attributeNames - Vertex attribute names (color, normal, etc.)
     * @property {number} numParts - The number of parts contained in the mesh.
     * @property {number} numIndices - Total number of vertex indices in the mesh.
     * @property {number} numVertices - Total number of vertices in the Mesh.
     * @property {number} numAttributes - Number of currently defined vertex attributes.
     */
/**
     * @typedef {object} Graphics.MeshPart
     * @property {number} partIndex - The part index (within the containing Mesh).
     * @property {Graphics.Topology} topology - element interpretation (currently only 'triangles' is supported).
     * @property {string[]} attributeNames - Vertex attribute names (color, normal, etc.)
     * @property {number} numIndices - Number of vertex indices that this mesh part refers to.
     * @property {number} numVerticesPerFace - Number of vertices per face (eg: 3 when topology is 'triangles').
     * @property {number} numFaces - Number of faces represented by the mesh part (numIndices / numVerticesPerFace).
     * @property {number} numVertices - Total number of vertices in the containing Mesh.
     * @property {number} numAttributes - Number of currently defined vertex attributes.
     */
/**
     * @typedef {object} Graphics.Model
     * @property {Uuid} objectID - UUID of corresponding inworld object (if model is associated)
     * @property {number} numMeshes - The number of submeshes contained in the model.
     * @property {Graphics.Mesh[]} meshes - Array of submesh references.
     * @property {Object.<string, Graphics.MaterialLayer[]>} materialLayers - Map of materials layer lists.  You can look up a material layer list by mesh part number or by material name.
     * @property {string[]} materialNames - Array of all the material names used by the mesh parts of this model, in order (e.g. materialNames[0] is the name of the first mesh part's material).
     */
/**
 * Ambient light is defined by the following properties.
 * @typedef {object} Entities.AmbientLight
 * @property {number} ambientIntensity=0.5 - The intensity of the light.
 * @property {string} ambientURL="" - A cube map image that defines the color of the light coming from each direction. If 
 *     <code>""</code> then the entity's {@link Entities.Skybox|Skybox} <code>url</code> property value is used, unless that also is <code>""</code> in which 
 *     case the entity's <code>ambientLightMode</code> property is set to <code>"inherit"</code>.
 */
/**
 * The AnimationProperties are used to configure an animation.
 * @typedef Entities.AnimationProperties
 * @property {string} url="" - The URL of the FBX file that has the animation.
 * @property {number} fps=30 - The speed in frames/s that the animation is played at.
 * @property {number} firstFrame=0 - The first frame to play in the animation.
 * @property {number} lastFrame=100000 - The last frame to play in the animation.
 * @property {number} currentFrame=0 - The current frame being played in the animation.
 * @property {boolean} running=false - If <code>true</code> then the animation should play.
 * @property {boolean} loop=true - If <code>true</code> then the animation should be continuously repeated in a loop.
 * @property {boolean} hold=false - If <code>true</code> then the rotations and translations of the last frame played should be
 *     maintained when the animation stops playing.
 */
/**
* <p>An entity action may be one of the following types:</p>
* <table>
*   <thead>
*     <tr><th>Value</th><th>Type</th><th>Description</th><th>Arguments</th></tr>
*   </thead>
*   <tbody>
*     <tr><td><code>"far-grab"</code></td><td>Avatar action</td>
*       <td>Moves and rotates an entity to a target position and orientation, optionally relative to another entity. Collisions 
*       between the entity and the user's avatar are disabled during the far-grab.</td>
*       <td>{@link Entities.ActionArguments-FarGrab}</td></tr>
*     <tr><td><code>"hold"</code></td><td>Avatar action</td>
*       <td>Positions and rotates an entity relative to an avatar's hand. Collisions between the entity and the user's avatar 
*       are disabled during the hold.</td>
*       <td>{@link Entities.ActionArguments-Hold}</td></tr>
*     <tr><td><code>"offset"</code></td><td>Object action</td>
*       <td>Moves an entity so that it is a set distance away from a target point.</td>
*       <td>{@link Entities.ActionArguments-Offset}</td></tr>
*     <tr><td><code>"tractor"</code></td><td>Object action</td>
*       <td>Moves and rotates an entity to a target position and orientation, optionally relative to another entity.</td>
*       <td>{@link Entities.ActionArguments-Tractor}</td></tr>
*     <tr><td><code>"travel-oriented"</code></td><td>Object action</td>
*       <td>Orients an entity to align with its direction of travel.</td>
*       <td>{@link Entities.ActionArguments-TravelOriented}</td></tr>
*     <tr><td><code>"hinge"</code></td><td>Object constraint</td>
*       <td>Lets an entity pivot about an axis or connects two entities with a hinge joint.</td>
*       <td>{@link Entities.ActionArguments-Hinge}</td></tr>
*     <tr><td><code>"slider"</code></td><td>Object constraint</td>
*       <td>Lets an entity slide and rotate along an axis, or connects two entities that slide and rotate along a shared 
*       axis.</td>
*       <td>{@link Entities.ActionArguments-Slider|ActionArguments-Slider}</td></tr>
*     <tr><td><code>"cone-twist"</code></td><td>Object constraint</td>
*       <td>Connects two entities with a joint that can move through a cone and can twist.</td>
*       <td>{@link Entities.ActionArguments-ConeTwist}</td></tr>
*     <tr><td><code>"ball-socket"</code></td><td>Object constraint</td>
*       <td>Connects two entities with a ball and socket joint.</td>
*       <td>{@link Entities.ActionArguments-BallSocket}</td></tr>
*     <tr><td><code>"spring"</code></td><td colspan="3">Synonym for <code>"tractor"</code>. <em>Legacy value.</em></td></tr>
*   </tbody>
* </table>
* @typedef {string} Entities.ActionType
*/
/**
 * Different entity types have different properties: some common to all entities (listed below) and some specific to each 
 * {@link Entities.EntityType|EntityType} (linked to below). The properties are accessed as an object of property names and 
 * values.
 *
 * @typedef {object} Entities.EntityProperties
 * @property {Uuid} id - The ID of the entity. <em>Read-only.</em>
 * @property {string} name="" - A name for the entity. Need not be unique.
 * @property {Entities.EntityType} type - The entity type. You cannot change the type of an entity after it's created. (Though 
 *     its value may switch among <code>"Box"</code>, <code>"Shape"</code>, and <code>"Sphere"</code> depending on changes to 
 *     the <code>shape</code> property set for entities of these types.) <em>Read-only.</em>
 * @property {boolean} clientOnly=false - If <code>true</code> then the entity is an avatar entity; otherwise it is a server
 *     entity. An avatar entity follows you to each domain you visit, rendering at the same world coordinates unless it's 
 *     parented to your avatar. <em>Value cannot be changed after the entity is created.</em><br />
 *     The value can also be set at entity creation by using the <code>clientOnly</code> parameter in 
 *     {@link Entities.addEntity}.
 * @property {Uuid} owningAvatarID=Uuid.NULL - The session ID of the owning avatar if <code>clientOnly</code> is 
 *     <code>true</code>, otherwise {@link Uuid|Uuid.NULL}. <em>Read-only.</em>
 *
 * @property {string} created - The UTC date and time that the entity was created, in ISO 8601 format as
 *     <code>yyyy-MM-ddTHH:mm:ssZ</code>. <em>Read-only.</em>
 * @property {number} age - The age of the entity in seconds since it was created. <em>Read-only.</em>
 * @property {string} ageAsText - The age of the entity since it was created, formatted as <code>h hours m minutes s 
 *     seconds</code>.
 * @property {number} lifetime=-1 - How long an entity lives for, in seconds, before being automatically deleted. A value of
 *     <code>-1</code> means that the entity lives for ever.
 * @property {number} lastEdited - When the entity was last edited, expressed as the number of microseconds since
 *     1970-01-01T00:00:00 UTC. <em>Read-only.</em>
 * @property {Uuid} lastEditedBy - The session ID of the avatar or agent that most recently created or edited the entity.
 *     <em>Read-only.</em>
 *
 * @property {boolean} locked=false - Whether or not the entity can be edited or deleted. If <code>true</code> then the 
 *     entity's properties other than <code>locked</code> cannot be changed, and the entity cannot be deleted.
 * @property {boolean} visible=true - Whether or not the entity is rendered. If <code>true</code> then the entity is rendered.
 * @property {boolean} canCastShadows=true - Whether or not the entity casts shadows. Currently applicable only to 
 *     {@link Entities.EntityType|Model} and {@link Entities.EntityType|Shape} entities. Shadows are cast if inside a 
 *     {@link Entities.EntityType|Zone} entity with <code>castShadows</code> enabled in its {@link Entities.EntityProperties-Zone|keyLight} property.
 *
 * @property {Vec3} position=0,0,0 - The position of the entity.
 * @property {Quat} rotation=0,0,0,1 - The orientation of the entity with respect to world coordinates.
 * @property {Vec3} registrationPoint=0.5,0.5,0.5 - The point in the entity that is set to the entity's position and is rotated 
 *      about, {@link Vec3|Vec3.ZERO} &ndash; {@link Vec3|Vec3.ONE}. A value of {@link Vec3|Vec3.ZERO} is the entity's
 *      minimum x, y, z corner; a value of {@link Vec3|Vec3.ONE} is the entity's maximum x, y, z corner.
 *
 * @property {Vec3} naturalPosition=0,0,0 - The center of the entity's unscaled mesh model if it has one, otherwise
 *     {@link Vec3|Vec3.ZERO}. <em>Read-only.</em>
 * @property {Vec3} naturalDimensions - The dimensions of the entity's unscaled mesh model if it has one, otherwise 
 *     {@link Vec3|Vec3.ONE}. <em>Read-only.</em>
 *
 * @property {Vec3} velocity=0,0,0 - The linear velocity of the entity in m/s with respect to world coordinates.
 * @property {number} damping=0.39347 - How much to slow down the linear velocity of an entity over time, <code>0.0</code> 
 *     &ndash; <code>1.0</code>. A higher damping value slows down the entity more quickly. The default value is for an 
 *     exponential decay timescale of 2.0s, where it takes 2.0s for the movement to slow to <code>1/e = 0.368</code> of its 
 *     initial value.
 * @property {Vec3} angularVelocity=0,0,0 - The angular velocity of the entity in rad/s with respect to its axes, about its
 *     registration point.
 * @property {number} angularDamping=0.39347 - How much to slow down the angular velocity of an entity over time, 
 *     <code>0.0</code> &ndash; <code>1.0</code>. A higher damping value slows down the entity more quickly. The default value 
 *     is for an exponential decay timescale of 2.0s, where it takes 2.0s for the movement to slow to <code>1/e = 0.368</code> 
 *     of its initial value.
 *
 * @property {Vec3} gravity=0,0,0 - The acceleration due to gravity in m/s<sup>2</sup> that the entity should move with, in 
 *     world coordinates. Set to <code>{ x: 0, y: -9.8, z: 0 }</code> to simulate Earth's gravity. Gravity is applied to an 
 *     entity's motion only if its <code>dynamic</code> property is <code>true</code>. If changing an entity's 
 *     <code>gravity</code> from {@link Vec3|Vec3.ZERO}, you need to give it a small <code>velocity</code> in order to kick off 
 *     physics simulation.
 *     The <code>gravity</code> value is applied in addition to the <code>acceleration</code> value.
 * @property {Vec3} acceleration=0,0,0 - A general acceleration in m/s<sup>2</sup> that the entity should move with, in world 
 *     coordinates. The acceleration is applied to an entity's motion only if its <code>dynamic</code> property is 
 *     <code>true</code>. If changing an entity's <code>acceleration</code> from {@link Vec3|Vec3.ZERO}, you need to give it a 
 *     small <code>velocity</code> in order to kick off physics simulation.
 *     The <code>acceleration</code> value is applied in addition to the <code>gravity</code> value.
 * @property {number} restitution=0.5 - The "bounciness" of an entity when it collides, <code>0.0</code> &ndash; 
 *     <code>0.99</code>. The higher the value, the more bouncy.
 * @property {number} friction=0.5 - How much to slow down an entity when it's moving against another, <code>0.0</code> &ndash; 
 *     <code>10.0</code>. The higher the value, the more quickly it slows down. Examples: <code>0.1</code> for ice, 
 *     <code>0.9</code> for sandpaper.
 * @property {number} density=1000 - The density of the entity in kg/m<sup>3</sup>, <code>100</code> for balsa wood &ndash; 
 *     <code>10000</code> for silver. The density is used in conjunction with the entity's bounding box volume to work out its 
 *     mass in the application of physics.
 *
 * @property {boolean} collisionless=false - Whether or not the entity should collide with items per its 
 *     <code>collisionMask<code> property. If <code>true</code> then the entity does not collide.
 * @property {boolean} ignoreForCollisions=false - Synonym for <code>collisionless</code>.
 * @property {Entities.CollisionMask} collisionMask=31 - What types of items the entity should collide with.
 * @property {string} collidesWith="static,dynamic,kinematic,myAvatar,otherAvatar," - Synonym for <code>collisionMask</code>,
 *     in text format.
 * @property {string} collisionSoundURL="" - The sound to play when the entity experiences a collision. Valid file formats are
 *     as per the {@link SoundCache} object.
 * @property {boolean} dynamic=false - Whether or not the entity should be affected by collisions. If <code>true</code> then 
 *     the entity's movement is affected by collisions.
 * @property {boolean} collisionsWillMove=false - Synonym for <code>dynamic</code>.
 *
 * @property {string} href="" - A "hifi://" metaverse address that a user is taken to when they click on the entity.
 * @property {string} description="" - A description of the <code>href</code> property value.
 *
 * @property {string} userData="" - Used to store extra data about the entity in JSON format. WARNING: Other apps such as the 
 *     Create app can also use this property, so make sure you handle data stored by other apps &mdash; edit only your bit and 
 *     leave the rest of the data intact. You can use <code>JSON.parse()</code> to parse the string into a JavaScript object 
 *     which you can manipulate the properties of, and use <code>JSON.stringify()</code> to convert the object into a string to 
 *     put in the property.
 *
 * @property {string} script="" - The URL of the client entity script, if any, that is attached to the entity.
 * @property {number} scriptTimestamp=0 - Intended to be used to indicate when the client entity script was loaded. Should be 
 *     an integer number of milliseconds since midnight GMT on January 1, 1970 (e.g., as supplied by <code>Date.now()</code>. 
 *     If you update the property's value, the <code>script</code> is re-downloaded and reloaded. This is how the "reload" 
 *     button beside the "script URL" field in properties tab of the Create app works.
 * @property {string} serverScripts="" - The URL of the server entity script, if any, that is attached to the entity.
 *
 * @property {Uuid} parentID=Uuid.NULL - The ID of the entity or avatar that this entity is parented to. {@link Uuid|Uuid.NULL} 
 *     if the entity is not parented.
 * @property {number} parentJointIndex=65535 - The joint of the entity or avatar that this entity is parented to. Use 
 *     <code>65535</code> or <code>-1</code> to parent to the entity or avatar's position and orientation rather than a joint.
 * @property {Vec3} localPosition=0,0,0 - The position of the entity relative to its parent if the entity is parented, 
 *     otherwise the same value as <code>position</code>. If the entity is parented to an avatar and is <code>clientOnly</code> 
 *     so that it scales with the avatar, this value remains the original local position value while the avatar scale changes.
 * @property {Quat} localRotation=0,0,0,1 - The rotation of the entity relative to its parent if the entity is parented, 
 *     otherwise the same value as <code>rotation</code>.
 * @property {Vec3} localVelocity=0,0,0 - The velocity of the entity relative to its parent if the entity is parented, 
 *     otherwise the same value as <code>velocity</code>.
 * @property {Vec3} localAngularVelocity=0,0,0 - The angular velocity of the entity relative to its parent if the entity is 
 *     parented, otherwise the same value as <code>position</code>.
 * @property {Vec3} localDimensions - The dimensions of the entity. If the entity is parented to an avatar and is 
 *     <code>clientOnly</code> so that it scales with the avatar, this value remains the original dimensions value while the 
 *     avatar scale changes.
 *
 * @property {Entities.BoundingBox} boundingBox - The axis-aligned bounding box that tightly encloses the entity. 
 *     <em>Read-only.</em>
 * @property {AACube} queryAACube - The axis-aligned cube that determines where the entity lives in the entity server's octree. 
 *     The cube may be considerably larger than the entity in some situations, e.g., when the entity is grabbed by an avatar: 
 *     the position of the entity is determined through avatar mixer updates and so the AA cube is expanded in order to reduce 
 *     unnecessary entity server updates. Scripts should not change this property's value.
 *
 * @property {string} actionData="" - Base-64 encoded compressed dump of the actions associated with the entity. This property
 *     is typically not used in scripts directly; rather, functions that manipulate an entity's actions update it.
 *     The size of this property increases with the number of actions. Because this property value has to fit within a High 
 *     Fidelity datagram packet there is a limit to the number of actions that an entity can have, and edits which would result 
 *     in overflow are rejected.
 *     <em>Read-only.</em>
 * @property {Entities.RenderInfo} renderInfo - Information on the cost of rendering the entity. Currently information is only 
 *     provided for <code>Model</code> entities. <em>Read-only.</em>
 *
 * @property {string} itemName="" - Certifiable name of the Marketplace item.
 * @property {string} itemDescription="" - Certifiable description of the Marketplace item.
 * @property {string} itemCategories="" - Certifiable category of the Marketplace item.
 * @property {string} itemArtist="" - Certifiable  artist that created the Marketplace item.
 * @property {string} itemLicense="" - Certifiable license URL for the Marketplace item.
 * @property {number} limitedRun=4294967295 - Certifiable maximum integer number of editions (copies) of the Marketplace item 
 *     allowed to be sold.
 * @property {number} editionNumber=0 - Certifiable integer edition (copy) number or the Marketplace item. Each copy sold in 
 *     the Marketplace is numbered sequentially, starting at 1.
 * @property {number} entityInstanceNumber=0 - Certifiable integer instance number for identical entities in a Marketplace 
 *     item. A Marketplace item may have identical parts. If so, then each is numbered sequentially with an instance number.
 * @property {string} marketplaceID="" - Certifiable UUID for the Marketplace item, as used in the URL of the item's download
 *     and its Marketplace Web page.
 * @property {string} certificateID="" - Hash of the entity's static certificate JSON, signed by the artist's private key.
 * @property {number} staticCertificateVersion=0 - The version of the method used to generate the <code>certificateID</code>.
 *
 * @see The different entity types have additional properties as follows:
 * @see {@link Entities.EntityProperties-Box|EntityProperties-Box}
 * @see {@link Entities.EntityProperties-Light|EntityProperties-Light}
 * @see {@link Entities.EntityProperties-Line|EntityProperties-Line}
 * @see {@link Entities.EntityProperties-Material|EntityProperties-Material}
 * @see {@link Entities.EntityProperties-Model|EntityProperties-Model}
 * @see {@link Entities.EntityProperties-ParticleEffect|EntityProperties-ParticleEffect}
 * @see {@link Entities.EntityProperties-PolyLine|EntityProperties-PolyLine}
 * @see {@link Entities.EntityProperties-PolyVox|EntityProperties-PolyVox}
 * @see {@link Entities.EntityProperties-Shape|EntityProperties-Shape}
 * @see {@link Entities.EntityProperties-Sphere|EntityProperties-Sphere}
 * @see {@link Entities.EntityProperties-Text|EntityProperties-Text}
 * @see {@link Entities.EntityProperties-Web|EntityProperties-Web}
 * @see {@link Entities.EntityProperties-Zone|EntityProperties-Zone}
 */
/**
 * The <code>"Box"</code> {@link Entities.EntityType|EntityType} is the same as the <code>"Shape"</code>
 * {@link Entities.EntityType|EntityType} except that its <code>shape</code> value is always set to <code>"Cube"</code>
 * when the entity is created. If its <code>shape</code> property value is subsequently changed then the entity's 
 * <code>type</code> will be reported as <code>"Sphere"</code> if the <code>shape</code> is set to <code>"Sphere"</code>, 
 * otherwise it will be reported as <code>"Shape"</code>.
 * @typedef {object} Entities.EntityProperties-Box
 */
/**
 * The <code>"Light"</code> {@link Entities.EntityType|EntityType} adds local lighting effects.
 * It has properties in addition to the common {@link Entities.EntityProperties|EntityProperties}.
 * @typedef {object} Entities.EntityProperties-Light
 * @property {Vec3} dimensions=0.1,0.1,0.1 - The dimensions of the entity. Entity surface outside these dimensions are not lit 
 *     by the light.
 * @property {Color} color=255,255,255 - The color of the light emitted.
 * @property {number} intensity=1 - The brightness of the light.
 * @property {number} falloffRadius=0.1 - The distance from the light's center at which intensity is reduced by 25%.
 * @property {boolean} isSpotlight=false - If <code>true</code> then the light is directional, emitting along the entity's
 *     local negative z-axis; otherwise the light is a point light which emanates in all directions.
 * @property {number} exponent=0 - Affects the softness of the spotlight beam: the higher the value the softer the beam.
 * @property {number} cutoff=1.57 - Affects the size of the spotlight beam: the higher the value the larger the beam.
 * @example <caption>Create a spotlight pointing at the ground.</caption>
 * Entities.addEntity({
 *     type: "Light",
 *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0.5, z: -4 })),
 *     rotation: Quat.fromPitchYawRollDegrees(-75, 0, 0),
 *     dimensions: { x: 5, y: 5, z: 5 },
 *     intensity: 100,
 *     falloffRadius: 0.3,
 *     isSpotlight: true,
 *     exponent: 20,
 *     cutoff: 30,
 *     lifetime: 300  // Delete after 5 minutes.
 * });
 */
/**
 * The <code>"Line"</code> {@link Entities.EntityType|EntityType} draws thin, straight lines between a sequence of two or more 
 * points.
 * It has properties in addition to the common {@link Entities.EntityProperties|EntityProperties}.
 * @typedef {object} Entities.EntityProperties-Line
 * @property {Vec3} dimensions=0.1,0.1,0.1 - The dimensions of the entity. Must be sufficient to contain all the 
 *     <code>linePoints</code>.
 * @property {Vec3[]} linePoints=[]] - The sequence of points to draw lines between. The values are relative to the entity's
 *     position. A maximum of 70 points can be specified. The property's value is set only if all the <code>linePoints</code> 
 *     lie within the entity's <code>dimensions</code>.
 * @property {number} lineWidth=2 - <em>Currently not used.</em>
 * @property {Color} color=255,255,255 - The color of the line.
 * @example <caption>Draw lines in a "V".</caption>
 * var entity = Entities.addEntity({
 *     type: "Line",
 *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0.75, z: -5 })),
 *     rotation: MyAvatar.orientation,
 *     dimensions: { x: 2, y: 2, z: 1 },
 *     linePoints: [
 *         { x: -1, y: 1, z: 0 },
 *         { x: 0, y: -1, z: 0 },
 *         { x: 1, y: 1, z: 0 },
 *     ],
 *     color: { red: 255, green: 0, blue: 0 },
 *     lifetime: 300  // Delete after 5 minutes.
 * });
 */
/**
 * The <code>"Material"</code> {@link Entities.EntityType|EntityType} modifies the existing materials on
 * {@link Entities.EntityType|Model} entities, {@link Entities.EntityType|Shape} entities (albedo only), 
 * {@link Overlays.OverlayType|model overlays}, and avatars.
 * It has properties in addition to the common {@link Entities.EntityProperties|EntityProperties}.<br />
 * To apply a material to an entity or overlay, set the material entity's <code>parentID</code> property to the entity or 
 * overlay's ID.
 * To apply a material to an avatar, set the material entity's <code>parentID</code> property to the avatar's session UUID.
 * To apply a material to your avatar such that it persists across domains and log-ins, create the material as an avatar entity 
 * by setting the <code>clientOnly</code> parameter in {@link Entities.addEntity} to <code>true</code>.
 * Material entities render as non-scalable spheres if they don't have their parent set.
 * @typedef {object} Entities.EntityProperties-Material
 * @property {string} materialURL="" - URL to a {@link MaterialResource}. If you append <code>?name</code> to the URL, the 
 *     material with that name in the {@link MaterialResource} will be applied to the entity. <br />
 *     Alternatively, set the property value to <code>"userData"</code> to use the {@link Entities.EntityProperties|userData} 
 *     entity property to live edit the material resource values.
 * @property {number} priority=0 - The priority for applying the material to its parent. Only the highest priority material is 
 *     applied, with materials of the same priority randomly assigned. Materials that come with the model have a priority of 
 *     <code>0</code>.
 * @property {string|number} parentMaterialName="0" - Selects the submesh or submeshes within the parent to apply the material 
 *     to. If in the format <code>"mat::string"</code>, all submeshes with material name <code>"string"</code> are replaced. 
 *     Otherwise the property value is parsed as an unsigned integer, specifying the mesh index to modify. Invalid values are 
 *     parsed to <code>0</code>.
 * @property {string} materialMappingMode="uv" - How the material is mapped to the entity. Either <code>"uv"</code> or 
 *     <code>"projected"</code>. <em>Currently, only <code>"uv"</code> is supported.
 * @property {Vec2} materialMappingPos=0,0 - Offset position in UV-space of the top left of the material, range 
 *     <code>{ x: 0, y: 0 }</code> &ndash; <code>{ x: 1, y: 1 }</code>.
 * @property {Vec2} materialMappingScale=1,1 - How much to scale the material within the parent's UV-space.
 * @property {number} materialMappingRot=0 - How much to rotate the material within the parent's UV-space, in degrees.
 * @example <caption>Color a sphere using a Material entity.</caption>
 * var entityID = Entities.addEntity({
 *     type: "Sphere",
 *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -5 })),
 *     dimensions: { x: 1, y: 1, z: 1 },
 *     color: { red: 128, green: 128, blue: 128 },
 *     lifetime: 300  // Delete after 5 minutes.
 * });
 *
 * var materialID = Entities.addEntity({
 *     type: "Material",
 *     parentID: entityID,
 *     materialURL: "userData",
 *     priority: 1,
 *     userData: JSON.stringify({
 *         materials: {
 *             // Can only set albedo on a Shape entity.
 *             // Value overrides entity's "color" property.
 *             albedo: [1.0, 0, 0]
 *         }
 *     }),
 * });
 */
/**
 * The <code>"Model"</code> {@link Entities.EntityType|EntityType} displays an FBX or OBJ model.
 * It has properties in addition to the common {@link Entities.EntityProperties|EntityProperties}.
 * @typedef {object} Entities.EntityProperties-Model
 * @property {Vec3} dimensions=0.1,0.1,0.1 - The dimensions of the entity. When adding an entity, if no <code>dimensions</code> 
 *     value is specified then the model is automatically sized to its 
 *     <code>{@link Entities.EntityProperties|naturalDimensions}</code>.
 * @property {Color} color=255,255,255 - <em>Currently not used.</em>
 * @property {string} modelURL="" - The URL of the FBX of OBJ model. Baked FBX models' URLs end in ".baked.fbx".<br />
 *     Note: If the name ends with <code>"default-image-model.fbx"</code> then the entity is considered to be an "Image" 
 *     entity, in which case the <code>textures</code> property should be set per the example.
 * @property {string} textures="" - A JSON string of texture name, URL pairs used when rendering the model in place of the
 *     model's original textures. Use a texture name from the <code>originalTextures</code> property to override that texture. 
 *     Only the texture names and URLs to be overridden need be specified; original textures are used where there are no 
 *     overrides. You can use <code>JSON.stringify()</code> to convert a JavaScript object of name, URL pairs into a JSON 
 *     string.
 * @property {string} originalTextures="{}" - A JSON string of texture name, URL pairs used in the model. The property value is 
 *     filled in after the entity has finished rezzing (i.e., textures have loaded). You can use <code>JSON.parse()</code> to 
 *     parse the JSON string into a JavaScript object of name, URL pairs. <em>Read-only.</em>
 *
 * @property {ShapeType} shapeType="none" - The shape of the collision hull used if collisions are enabled.
 * @property {string} compoundShapeURL="" - The OBJ file to use for the compound shape if <code>shapeType</code> is
 *     <code>"compound"</code>.
 *
 * @property {Entities.AnimationProperties} animation - An animation to play on the model.
 *
 * @property {Quat[]} jointRotations=[]] - Joint rotations applied to the model; <code>[]</code> if none are applied or the 
 *     model hasn't loaded. The array indexes are per {@link Entities.getJointIndex|getJointIndex}. Rotations are relative to 
 *     each joint's parent.<br />
 *     Joint rotations can be set by {@link Entities.setLocalJointRotation|setLocalJointRotation} and similar functions, or by
 *     setting the value of this property. If you set a joint rotation using this property you also need to set the 
 *     corresponding <code>jointRotationsSet</code> value to <code>true</code>.
 * @property {boolean[]} jointRotationsSet=[]] - <code>true</code> values for joints that have had rotations applied, 
 *     <code>false</code> otherwise; <code>[]</code> if none are applied or the model hasn't loaded. The array indexes are per 
 *     {@link Entities.getJointIndex|getJointIndex}.
 * @property {Vec3[]} jointTranslations=[]] - Joint translations applied to the model; <code>[]</code> if none are applied or 
 *     the model hasn't loaded. The array indexes are per {@link Entities.getJointIndex|getJointIndex}. Rotations are relative 
 *     to each joint's parent.<br />
 *     Joint translations can be set by {@link Entities.setLocalJointTranslation|setLocalJointTranslation} and similar 
 *     functions, or by setting the value of this property. If you set a joint translation using this property you also need to 
 *     set the corresponding <code>jointTranslationsSet</code> value to <code>true</code>.
 * @property {boolean[]} jointTranslationsSet=[]] - <code>true</code> values for joints that have had translations applied, 
 *     <code>false</code> otherwise; <code>[]</code> if none are applied or the model hasn't loaded. The array indexes are per 
 *     {@link Entities.getJointIndex|getJointIndex}.
 * @property {boolean} relayParentJoints=false - If <code>true</code> and the entity is parented to an avatar, then the 
 *     avatar's joint rotations are applied to the entity's joints.
 *
 * @example <caption>Rez a Vive tracker puck.</caption>
 * var entity = Entities.addEntity({
 *     type: "Model",
 *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0.75, z: -2 })),
 *     rotation: MyAvatar.orientation,
 *     modelURL: "http://content.highfidelity.com/seefo/production/puck-attach/vive_tracker_puck.obj",
 *     dimensions: { x: 0.0945, y: 0.0921, z: 0.0423 },
 *     lifetime: 300  // Delete after 5 minutes.
 * });
 * @example <caption>Create an "Image" entity like you can in the Create app.</caption>
 * var IMAGE_MODEL = "https://hifi-content.s3.amazonaws.com/DomainContent/production/default-image-model.fbx";
 * var DEFAULT_IMAGE = "https://hifi-content.s3.amazonaws.com/DomainContent/production/no-image.jpg";
 * var entity = Entities.addEntity({
 *     type: "Model",
 *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0.5, z: -3 })),
 *     rotation: MyAvatar.orientation,
 *     dimensions: {
 *         x: 0.5385,
 *         y: 0.2819,
 *         z: 0.0092
 *     },
 *     shapeType: "box",
 *     collisionless: true,
 *     modelURL: IMAGE_MODEL,
 *     textures: JSON.stringify({ "tex.picture": DEFAULT_IMAGE }),
 *     lifetime: 300  // Delete after 5 minutes
 * });
 */
/**
 * The <code>"ParticleEffect"</code> {@link Entities.EntityType|EntityType} displays a particle system that can be used to 
 * simulate things such as fire, smoke, snow, magic spells, etc. The particles emanate from an ellipsoid or part thereof.
 * It has properties in addition to the common {@link Entities.EntityProperties|EntityProperties}.
 * @typedef {object} Entities.EntityProperties-ParticleEffect

 * @property {boolean} isEmitting=true - If <code>true</code> then particles are emitted.
 * @property {number} maxParticles=1000 - The maximum number of particles to render at one time. Older particles are deleted if 
 *     necessary when new ones are created.
 * @property {number} lifespan=3s - How long, in seconds, each particle lives.
 * @property {number} emitRate=15 - The number of particles per second to emit.
 * @property {number} emitSpeed=5 - The speed, in m/s, that each particle is emitted at.
 * @property {number} speedSpread=1 - The spread in speeds at which particles are emitted at. If <code>emitSpeed == 5</code> 
 *     and <code>speedSpread == 1</code>, particles will be emitted with speeds in the range 4m/s &ndash; 6m/s.
 * @property {vec3} emitAcceleration=0,-9.8,0 - The acceleration that is applied to each particle during its lifetime. The 
 *     default is Earth's gravity value.
 * @property {vec3} accelerationSpread=0,0,0 - The spread in accelerations that each particle is given. If
 *     <code>emitAccelerations == {x: 0, y: -9.8, z: 0}</code> and <code>accelerationSpread ==
 *     {x: 0, y: 1, z: 0}</code>, each particle will have an acceleration in the range, <code>{x: 0, y: -10.8, z: 0}</code> 
 *     &ndash; <code>{x: 0, y: -8.8, z: 0}</code>.
 * @property {Vec3} dimensions - The dimensions of the particle effect, i.e., a bounding box containing all the particles
 *     during their lifetimes, assuming that <code>emitterShouldTrail</code> is <code>false</code>. <em>Read-only.</em>
 * @property {boolean} emitterShouldTrail=false - If <code>true</code> then particles are "left behind" as the emitter moves,
 *     otherwise they stay with the entity's dimensions.
 *
 * @property {Quat} emitOrientation=-0.707,0,0,0.707 - The orientation of particle emission relative to the entity's axes. By
 *     default, particles emit along the entity's local z-axis, and <code>azimuthStart</code> and <code>azimuthFinish</code> 
 *     are relative to the entity's local x-axis. The default value is a rotation of -90 degrees about the local x-axis, i.e., 
 *     the particles emit vertically.
 * @property {vec3} emitDimensions=0,0,0 - The dimensions of the ellipsoid from which particles are emitted.
 * @property {number} emitRadiusStart=1 - The starting radius within the ellipsoid at which particles start being emitted;
 *     range <code>0.0</code> &ndash; <code>1.0</code> for the ellipsoid center to the ellipsoid surface, respectively.
 *     Particles are emitted from the portion of the ellipsoid that lies between <code>emitRadiusStart</code> and the 
 *     ellipsoid's surface.
 * @property {number} polarStart=0 - The angle in radians from the entity's local z-axis at which particles start being emitted 
 *     within the ellipsoid; range <code>0</code> &ndash; <code>Math.PI</code>. Particles are emitted from the portion of the 
 *     ellipsoid that lies between <code>polarStart<code> and <code>polarFinish</code>.
 * @property {number} polarFinish=0 - The angle in radians from the entity's local z-axis at which particles stop being emitted 
 *     within the ellipsoid; range <code>0</code> &ndash; <code>Math.PI</code>. Particles are emitted from the portion of the 
 *     ellipsoid that lies between <code>polarStart<code> and <code>polarFinish</code>.
 * @property {number} azimuthStart=-Math.PI - The angle in radians from the entity's local x-axis about the entity's local 
 *     z-axis at which particles start being emitted; range <code>-Math.PI</code> &ndash; <code>Math.PI</code>. Particles are 
 *     emitted from the portion of the ellipsoid that lies between <code>azimuthStart<code> and <code>azimuthFinish</code>.
 * @property {number} azimuthFinish=Math.PI - The angle in radians from the entity's local x-axis about the entity's local 
 *     z-axis at which particles stop being emitted; range <code>-Math.PI</code> &ndash; <code>Math.PI</code>. Particles are 
 *     emitted from the portion of the ellipsoid that lies between <code>azimuthStart<code> and <code>azimuthFinish</code>.
 *
 * @property {string} textures="" - The URL of a JPG or PNG image file to display for each particle. If you want transparency, 
 *     use PNG format.
 * @property {number} particleRadius=0.025 - The radius of each particle at the middle of its life.
 * @property {number} radiusStart=0.025 - The radius of each particle at the start of its life. If not explicitly set, the 
 *     <code>particleRadius</code> value is used.
 * @property {number} radiusFinish=0.025 - The radius of each particle at the end of its life. If not explicitly set, the 
 *     <code>particleRadius</code> value is used.
 * @property {number} radiusSpread=0 - <em>Currently not used.</em>
 * @property {Color} color=255,255,255 - The color of each particle at the middle of its life.
 * @property {Color} colorStart=255,255,255 - The color of each particle at the start of its life. If not explicitly set, the 
 *     <code>color</code> value is used.
 * @property {Color} colorFinish=255,255,255 - The color of each particle at the end of its life. If not explicitly set, the 
 *     <code>color</code> value is used.
 * @property {Color} colorSpread=0,0,0 - <em>Currently not used.</em>
 * @property {number} alpha=1 - The alpha of each particle at the middle of its life.
 * @property {number} alphaStart=1 - The alpha of each particle at the start of its life. If not explicitly set, the 
 *     <code>alpha</code> value is used.
 * @property {number} alphaFinish=1 - The alpha of each particle at the end of its life. If not explicitly set, the 
 *     <code>alpha</code> value is used.
 * @property {number} alphaSpread=0 - <em>Currently not used.</em>
 *
 * @property {ShapeType} shapeType="none" - <em>Currently not used.</em> <em>Read-only.</em>
 *
 * @example <caption>Create a ball of green smoke.</caption>
 * particles = Entities.addEntity({
 *     type: "ParticleEffect",
 *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0.5, z: -4 })),
 *     lifespan: 5,
 *     emitRate: 10,
 *     emitSpeed: 0.02,
 *     speedSpread: 0.01,
 *     emitAcceleration: { x: 0, y: 0.02, z: 0 },
 *     polarFinish: Math.PI,
 *     textures: "https://content.highfidelity.com/DomainContent/production/Particles/wispy-smoke.png",
 *     particleRadius: 0.1,
 *     color: { red: 0, green: 255, blue: 0 },
 *     alphaFinish: 0,
 *     lifetime: 300  // Delete after 5 minutes.
 * });
 */
/**
 * The <code>"PolyLine"</code> {@link Entities.EntityType|EntityType} draws textured, straight lines between a sequence of 
 * points.
 * It has properties in addition to the common {@link Entities.EntityProperties|EntityProperties}.
 * @typedef {object} Entities.EntityProperties-PolyLine
 * @property {Vec3} dimensions=1,1,1 - The dimensions of the entity, i.e., the size of the bounding box that contains the 
 *     lines drawn.
 * @property {Vec3[]} linePoints=[]] - The sequence of points to draw lines between. The values are relative to the entity's
 *     position. A maximum of 70 points can be specified. Must be specified in order for the entity to render.
 * @property {Vec3[]} normals=[]] - The normal vectors for the line's surface at the <code>linePoints</code>. The values are 
 *     relative to the entity's orientation. Must be specified in order for the entity to render.
 * @property {number[]} strokeWidths=[]] - The widths, in m, of the line at the <code>linePoints</code>. Must be specified in 
 *     order for the entity to render.
 * @property {number} lineWidth=2 - <em>Currently not used.</code>
 * @property {Vec3[]} strokeColors=[]] - <em>Currently not used.</em>
 * @property {Color} color=255,255,255 - The base color of the line, which is multiplied with the color of the texture for
 *     rendering.
 * @property {string} textures="" - The URL of a JPG or PNG texture to use for the lines. If you want transparency, use PNG
 *     format.
 * @property {boolean} isUVModeStretch=true - If <code>true</code>, the texture is stretched to fill the whole line, otherwise 
 *     the texture repeats along the line.
 * @example <caption>Draw a textured "V".</caption>
 * var entity = Entities.addEntity({
 *     type: "PolyLine",
 *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0.75, z: -5 })),
 *     rotation: MyAvatar.orientation,
 *     linePoints: [
 *         { x: -1, y: 0.5, z: 0 },
 *         { x: 0, y: 0, z: 0 },
 *         { x: 1, y: 0.5, z: 0 }
 *     ],
 *     normals: [
 *         { x: 0, y: 0, z: 1 },
 *         { x: 0, y: 0, z: 1 },
 *         { x: 0, y: 0, z: 1 }
 *     ],
 *     strokeWidths: [ 0.1, 0.1, 0.1 ],
 *     color: { red: 255, green: 0, blue: 0 },  // Use just the red channel from the image.
 *     textures: "http://hifi-production.s3.amazonaws.com/DomainContent/Toybox/flowArts/trails.png",
 *     isUVModeStretch: true,
 *     lifetime: 300  // Delete after 5 minutes.
 * });
 */
/**
 * The <code>"PolyVox"</code> {@link Entities.EntityType|EntityType} displays a set of textured voxels. 
 * It has properties in addition to the common {@link Entities.EntityProperties|EntityProperties}.
 * If you have two or more neighboring PolyVox entities of the same size abutting each other, you can display them as joined by
 * configuring their <code>voxelSurfaceStyle</code> and neighbor ID properties.<br />
 * PolyVox entities uses a library from <a href="http://www.volumesoffun.com/">Volumes of Fun</a>. Their
 * <a href="http://www.volumesoffun.com/polyvox-documentation/">library documentation</a> may be useful to read.
 * @typedef {object} Entities.EntityProperties-PolyVox
 * @property {Vec3} dimensions=0.1,0.1,0.1 - The dimensions of the entity.
 * @property {Vec3} voxelVolumeSize=32,32,32 - Integer number of voxels along each axis of the entity, in the range 
 *     <code>1,1,1</code> to <code>128,128,128</code>. The dimensions of each voxel is 
 *     <code>dimensions / voxelVolumesize</code>.
 * @property {string} voxelData="ABAAEAAQAAAAHgAAEAB42u3BAQ0AAADCoPdPbQ8HFAAAAPBuEAAAAQ==" - Base-64 encoded compressed dump of 
 *     the PolyVox data. This property is typically not used in scripts directly; rather, functions that manipulate a PolyVox 
 *     entity update it.<br />
 *     The size of this property increases with the size and complexity of the PolyVox entity, with the size depending on how 
 *     the particular entity's voxels compress. Because this property value has to fit within a High Fidelity datagram packet 
 *     there is a limit to the size and complexity of a PolyVox entity, and edits which would result in an overflow are 
 *     rejected.
 * @property {Entities.PolyVoxSurfaceStyle} voxelSurfaceStyle=2 - The style of rendering the voxels' surface and how 
 *     neighboring PolyVox entities are joined.
 * @property {string} xTextureURL="" - URL of the texture to map to surfaces perpendicular to the entity's local x-axis. JPG or
 *     PNG format. If no texture is specified the surfaces display white.
 * @property {string} yTextureURL="" - URL of the texture to map to surfaces perpendicular to the entity's local y-axis. JPG or 
 *     PNG format. If no texture is specified the surfaces display white.
 * @property {string} zTextureURL="" - URL of the texture to map to surfaces perpendicular to the entity's local z-axis. JPG or 
 *     PNG format. If no texture is specified the surfaces display white.
 * @property {Uuid} xNNeighborID=Uuid.NULL - ID of the neighboring PolyVox entity in the entity's -ve local x-axis direction, 
 *     if you want them joined. Set to {@link Uuid|Uuid.NULL} if there is none or you don't want to join them.
 * @property {Uuid} yNNeighborID=Uuid.NULL - ID of the neighboring PolyVox entity in the entity's -ve local y-axis direction, 
 *     if you want them joined. Set to {@link Uuid|Uuid.NULL} if there is none or you don't want to join them.
 * @property {Uuid} zNNeighborID=Uuid.NULL - ID of the neighboring PolyVox entity in the entity's -ve local z-axis direction, 
 *     if you want them joined. Set to {@link Uuid|Uuid.NULL} if there is none or you don't want to join them.
 * @property {Uuid} xPNeighborID=Uuid.NULL - ID of the neighboring PolyVox entity in the entity's +ve local x-axis direction, 
 *     if you want them joined. Set to {@link Uuid|Uuid.NULL} if there is none or you don't want to join them.
 * @property {Uuid} yPNeighborID=Uuid.NULL - ID of the neighboring PolyVox entity in the entity's +ve local y-axis direction, 
 *     if you want them joined. Set to {@link Uuid|Uuid.NULL} if there is none or you don't want to join them.
 * @property {Uuid} zPNeighborID=Uuid.NULL - ID of the neighboring PolyVox entity in the entity's +ve local z-axis direction, 
 *     if you want them joined. Set to {@link Uuid|Uuid.NULL} if there is none or you don't want to join them.
 * @example <caption>Create a textured PolyVox sphere.</caption>
 * var position = Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0.5, z: -8 }));
 * var texture = "http://public.highfidelity.com/cozza13/tuscany/Concrete2.jpg";
 * var polyVox = Entities.addEntity({
 *     type: "PolyVox",
 *     position: position,
 *     dimensions: { x: 2, y: 2, z: 2 },
 *     xTextureURL: texture,
 *     yTextureURL: texture,
 *     zTextureURL: texture,
 *     lifetime: 300  // Delete after 5 minutes.
 * });
 * Entities.setVoxelSphere(polyVox, position, 0.8, 255);
 */
/**
 * The <code>"Shape"</code> {@link Entities.EntityType|EntityType} displays an entity of a specified <code>shape</code>.
 * It has properties in addition to the common {@link Entities.EntityProperties|EntityProperties}.
 * @typedef {object} Entities.EntityProperties-Shape
 * @property {Entities.Shape} shape="Sphere" - The shape of the entity.
 * @property {Vec3} dimensions=0.1,0.1,0.1 - The dimensions of the entity.
 * @property {Color} color=255,255,255 - The color of the entity.
 * @example <caption>Create a cylinder.</caption>
 * var shape = Entities.addEntity({
 *     type: "Shape",
 *     shape: "Cylinder",
 *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -5 })),
 *     dimensions: { x: 0.4, y: 0.6, z: 0.4 },
 *     lifetime: 300  // Delete after 5 minutes.
 * });
 */
/**
 * The <code>"Sphere"</code> {@link Entities.EntityType|EntityType} is the same as the <code>"Shape"</code>
 * {@link Entities.EntityType|EntityType} except that its <code>shape</code> value is always set to <code>"Sphere"</code>
 * when the entity is created. If its <code>shape</code> property value is subsequently changed then the entity's 
 * <code>type</code> will be reported as <code>"Box"</code> if the <code>shape</code> is set to <code>"Cube"</code>, 
 * otherwise it will be reported as <code>"Shape"</code>.
 * @typedef {object} Entities.EntityProperties-Sphere
 */
/**
 * The <code>"Text"</code> {@link Entities.EntityType|EntityType} displays a 2D rectangle of text in the domain.
 * It has properties in addition to the common {@link Entities.EntityProperties|EntityProperties}.
 * @typedef {object} Entities.EntityProperties-Text
 * @property {Vec3} dimensions=0.1,0.1,0.01 - The dimensions of the entity.
 * @property {string} text="" - The text to display on the face of the entity. Text wraps if necessary to fit. New lines can be
 *     created using <code>\n</code>. Overflowing lines are not displayed.
 * @property {number} lineHeight=0.1 - The height of each line of text (thus determining the font size).
 * @property {Color} textColor=255,255,255 - The color of the text.
 * @property {Color} backgroundColor=0,0,0 - The color of the background rectangle.
 * @property {boolean} faceCamera=false - If <code>true</code>, the entity is oriented to face each user's camera (i.e., it
 *     differs for each user present).
 * @example <caption>Create a text entity.</caption>
 * var text = Entities.addEntity({
 *     type: "Text",
 *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -5 })),
 *     dimensions: { x: 0.6, y: 0.3, z: 0.01 },
 *     lineHeight: 0.12,
 *     text: "Hello\nthere!",
 *     faceCamera: true,
 *     lifetime: 300  // Delete after 5 minutes.
 * });
 */
/**
 * The <code>"Web"</code> {@link Entities.EntityType|EntityType} displays a browsable Web page. Each user views their own copy 
 * of the Web page: if one user navigates to another page on the entity, other users do not see the change; if a video is being 
 * played, users don't see it in sync.
 * The entity has properties in addition to the common {@link Entities.EntityProperties|EntityProperties}.
 * @typedef {object} Entities.EntityProperties-Web
 * @property {Vec3} dimensions=0.1,0.1,0.01 - The dimensions of the entity.
 * @property {string} sourceUrl="" - The URL of the Web page to display. This value does not change as you or others navigate 
 *     on the Web entity.
 * @property {number} dpi=30 - The resolution to display the page at, in dots per inch. If you convert this to dots per meter 
 *     (multiply by 1 / 0.0254 = 39.3701) then multiply <code>dimensions.x</code> and <code>dimensions.y</code> by that value 
 *     you get the resolution in pixels.
 * @example <caption>Create a Web entity displaying at 1920 x 1080 resolution.</caption>
 * var METERS_TO_INCHES = 39.3701;
 * var entity = Entities.addEntity({
 *     type: "Web",
 *     sourceUrl: "https://highfidelity.com/",
 *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0.75, z: -4 })),
 *     rotation: MyAvatar.orientation,
 *     dimensions: {
 *         x: 3,
 *         y: 3 * 1080 / 1920,
 *         z: 0.01
 *     },
 *     dpi: 1920 / (3 * METERS_TO_INCHES),
 *     lifetime: 300  // Delete after 5 minutes.
 * });
 */
/**
 * The <code>"Zone"</code> {@link Entities.EntityType|EntityType} is a volume of lighting effects and avatar permissions.
 * Avatar interaction events such as {@link Entities.enterEntity} are also often used with a Zone entity.
 * It has properties in addition to the common {@link Entities.EntityProperties|EntityProperties}.
 * @typedef {object} Entities.EntityProperties-Zone
 * @property {Vec3} dimensions=0.1,0.1,0.1 - The size of the volume in which the zone's lighting effects and avatar permissions 
 *     have effect.
 *
 * @property {ShapeType} shapeType="box" - The shape of the volume in which the zone's lighting effects and avatar 
 *     permissions have effect. Reverts to the default value if set to <code>"none"</code>, or set to <code>"compound"</code> 
 *     and <code>compoundShapeURL</code> is <code>""</code>.
  * @property {string} compoundShapeURL="" - The OBJ file to use for the compound shape if <code>shapeType</code> is 
 *     <code>"compound"</code>.
 *
 * @property {string} keyLightMode="inherit" - Configures the key light in the zone. Possible values:<br />
 *     <code>"inherit"</code>: The key light from any enclosing zone continues into this zone.<br />
 *     <code>"disabled"</code>: The key light from any enclosing zone and the key light of this zone are disabled in this 
 *         zone.<br />
 *     <code>"enabled"</code>: The key light properties of this zone are enabled, overriding the key light of from any 
 *         enclosing zone.
 * @property {Entities.KeyLight} keyLight - The key light properties of the zone.
 *
 * @property {string} ambientLightMode="inherit" - Configures the ambient light in the zone. Possible values:<br />
 *     <code>"inherit"</code>: The ambient light from any enclosing zone continues into this zone.<br />
 *     <code>"disabled"</code>: The ambient light from any enclosing zone and the ambient light of this zone are disabled in 
 *         this zone.<br />
 *     <code>"enabled"</code>: The ambient light properties of this zone are enabled, overriding the ambient light from any 
 *         enclosing zone.
 * @property {Entities.AmbientLight} ambientLight - The ambient light properties of the zone.
 *
 * @property {string} skyboxMode="inherit" - Configures the skybox displayed in the zone. Possible values:<br />
 *     <code>"inherit"</code>: The skybox from any enclosing zone is dislayed in this zone.<br />
 *     <code>"disabled"</code>: The skybox from any enclosing zone and the skybox of this zone are disabled in this zone.<br />
 *     <code>"enabled"</code>: The skybox properties of this zone are enabled, overriding the skybox from any enclosing zone.
 * @property {Entities.Skybox} skybox - The skybox properties of the zone.
 *
 * @property {string} hazeMode="inherit" - Configures the haze in the zone. Possible values:<br />
 *     <code>"inherit"</code>: The haze from any enclosing zone continues into this zone.<br />
 *     <code>"disabled"</code>: The haze from any enclosing zone and the haze of this zone are disabled in this zone.<br />
 *     <code>"enabled"</code>: The haze properties of this zone are enabled, overriding the haze from any enclosing zone.
 * @property {Entities.Haze} haze - The haze properties of the zone.
 *
 * @property {boolean} flyingAllowed=true - If <code>true</code> then visitors can fly in the zone; otherwise they cannot.
 * @property {boolean} ghostingAllowed=true - If <code>true</code> then visitors with avatar collisions turned off will not 
 *     collide with content in the zone; otherwise visitors will always collide with content in the zone.
 
 * @property {string} filterURL="" - The URL of a JavaScript file that filters changes to properties of entities within the 
 *     zone. It is periodically executed for each entity in the zone. It can, for example, be used to not allow changes to 
 *     certain properties.<br />
 * <pre>
 * function filter(properties) {
 *     // Test and edit properties object values,
 *     // e.g., properties.modelURL, as required.
 *     return properties;
 * }
 * </pre>
 *
 * @example <caption>Create a zone that casts a red key light along the x-axis.</caption>
 * var zone = Entities.addEntity({
 *     type: "Zone",
 *     position: MyAvatar.position,
 *     dimensions: { x: 100, y: 100, z: 100 },
 *     keyLightMode: "enabled",
 *     keyLight: {
 *         "color": { "red": 255, "green": 0, "blue": 0 },
 *         "direction": { "x": 1, "y": 0, "z": 0 }
 *     },
 *     lifetime: 300  // Delete after 5 minutes.
 * });
 */
/**
     * The axis-aligned bounding box of an entity.
     * @typedef Entities.BoundingBox
     * @property {Vec3} brn - The bottom right near (minimum axes values) corner of the AA box.
     * @property {Vec3} tfl - The top far left (maximum axes values) corner of the AA box.
     * @property {Vec3} center - The center of the AA box.
     * @property {Vec3} dimensions - The dimensions of the AA box.
     */
/**
         * Information on how an entity is rendered. Properties are only filled in for <code>Model</code> entities; other 
         * entity types have an empty object, <code>{}</code>.
         * @typedef {object} Entities.RenderInfo
         * @property {number} verticesCount - The number of vertices in the entity.
         * @property {number} texturesCount  - The number of textures in the entity.
         * @property {number} textureSize - The total size of the textures in the entity, in bytes.
         * @property {boolean} hasTransparent - Is <code>true</code> if any of the textures has transparency.
         * @property {number} drawCalls - The number of draw calls required to render the entity.
         */
/**
     * <p>A <code>BoxFace</code> specifies the face of an axis-aligned (AA) box.
     * <table>
     *   <thead>
     *     <tr><th>Value</th><th>Description</th></tr>
     *   </thead>
     *   <tbody>
     *     <tr><td><code>"MIN_X_FACE"</code></td><td>The minimum x-axis face.</td></tr>
     *     <tr><td><code>"MAX_X_FACE"</code></td><td>The maximum x-axis face.</td></tr>
     *     <tr><td><code>"MIN_Y_FACE"</code></td><td>The minimum y-axis face.</td></tr>
     *     <tr><td><code>"MAX_Y_FACE"</code></td><td>The maximum y-axis face.</td></tr>
     *     <tr><td><code>"MIN_Z_FACE"</code></td><td>The minimum z-axis face.</td></tr>
     *     <tr><td><code>"MAX_Z_FACE"</code></td><td>The maximum z-axis face.</td></tr>
     *     <tr><td><code>"UNKNOWN_FACE"</code></td><td>Unknown value.</td></tr>
     *   </tbody>
     * </table>
     * @typedef {string} BoxFace
     */
/**
 * The result of a {@link PickRay} search using {@link Entities.findRayIntersection|findRayIntersection} or 
 * {@link Entities.findRayIntersectionBlocking|findRayIntersectionBlocking}.
 * @typedef {object} Entities.RayToEntityIntersectionResult
 * @property {boolean} intersects - <code>true</code> if the {@link PickRay} intersected an entity, otherwise 
 *     <code>false</code>.
 * @property {boolean} accurate - Is always <code>true</code>.
 * @property {Uuid} entityID - The ID if the entity intersected, if any, otherwise <code>null</code>.
 * @property {number} distance - The distance from the {@link PickRay} origin to the intersection point.
 * @property {Vec3} intersection - The intersection point.
 * @property {Vec3} surfaceNormal - The surface normal of the entity at the intersection point.
 * @property {BoxFace} face - The face of the entity's axis-aligned box that the ray intersects.
 * @property {object} extraInfo - Extra information depending on the entity intersected. Currently, only <code>Model</code> 
 *     entities provide extra information, and the information provided depends on the <code>precisionPicking</code> parameter 
 *     value that the search function was called with.
 */
/**
 * The Entities API provides facilities to create and interact with entities. Entities are 2D and 3D objects that are visible
 * to everyone and typically are persisted to the domain. For Interface scripts, the entities available are those that 
 * Interface has displayed and so knows about.
 *
 * @namespace Entities
 * @property {Uuid} keyboardFocusEntity - Get or set the {@link Entities.EntityType|Web} entity that has keyboard focus.
 *     If no entity has keyboard focus, get returns <code>null</code>; set to <code>null</code> or {@link Uuid|Uuid.NULL} to 
 *     clear keyboard focus.
 */
/**
     * Check whether or not you can change the <code>locked</code> property of entities. Locked entities have their 
     * <code>locked</code> property set to <code>true</code> and cannot be edited or deleted. Whether or not you can change 
     * entities' <code>locked</code> properties is configured in the domain server's permissions.
     * @function Entities.canAdjustLocks
     * @returns {boolean} <code>true</code> if the client can change the <code>locked</code> property of entities,
     *     otherwise <code>false</code>.
     * @example <caption>Set an entity's <code>locked</code> property to true if you can.</caption>
     * if (Entities.canAdjustLocks()) {
     *     Entities.editEntity(entityID, { locked: true });
     * } else {
     *     Window.alert("You do not have the permissions to set an entity locked!");
     * }
     */
/**
     * Check whether or not you can rez (create) new entities in the domain.
     * @function Entities.canRez
     * @returns {boolean} <code>true</code> if the domain server will allow the script to rez (create) new entities, 
     *     otherwise <code>false</code>.
     */
/**
     * Check whether or not you can rez (create) new temporary entities in the domain. Temporary entities are entities with a
     * finite <code>lifetime</code> property value set.
     * @function Entities.canRezTmp
     * @returns {boolean} <code>true</code> if the domain server will allow the script to rez (create) new temporary
     *     entities, otherwise <code>false</code>.
     */
/**
     * Check whether or not you can rez (create) new certified entities in the domain. Certified entities are entities that have
     * PoP certificates.
     * @function Entities.canRezCertified
     * @returns {boolean} <code>true</code> if the domain server will allow the script to rez (create) new certified
     *     entities, otherwise <code>false</code>.
     */
/**
     * Check whether or not you can rez (create) new temporary certified entities in the domain. Temporary entities are entities
     * with a finite  <code>lifetime</code> property value set. Certified entities are entities that have PoP certificates.
     * @function Entities.canRezTmpCertified
     * @returns {boolean} <code>true</code> if the domain server will allow the script to rez (create) new temporary 
     *     certified entities, otherwise <code>false</code>.
     */
/**
     * Check whether or not you can make changes to the asset server's assets.
     * @function Entities.canWriteAssets
     * @returns {boolean} <code>true</code> if the domain server will allow the script to make changes to the asset server's 
     *     assets, otherwise <code>false</code>.
     */
/**
     * Check whether or not you can replace the domain's content set.
     * @function Entities.canReplaceContent
     * @returns {boolean} <code>true</code> if the domain server will allow the script to replace the domain's content set, 
     *     otherwise <code>false</code>.
     */
/**
     * Add a new entity with specified properties.
     * @function Entities.addEntity
     * @param {Entities.EntityProperties} properties - The properties of the entity to create.
     * @param {boolean} [clientOnly=false] - If <code>true</code>, or if <code>clientOnly</code> is set <code>true</code> in 
     *     the properties, the entity is created as an avatar entity; otherwise it is created on the server. An avatar entity 
     *     follows you to each domain you visit, rendering at the same world coordinates unless it's parented to your avatar.
     * @returns {Uuid} The ID of the entity if successfully created, otherwise {@link Uuid|Uuid.NULL}.
     * @example <caption>Create a box entity in front of your avatar.</caption>
     * var entityID = Entities.addEntity({
     *     type: "Box",
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -5 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 0.5, y: 0.5, z: 0.5 }
     * });
     * print("Entity created: " + entityID);
     */
/**
     * Get the properties of an entity.
     * @function Entities.getEntityProperties
     * @param {Uuid} entityID - The ID of the entity to get the properties of.
     * @param {string[]} [desiredProperties=[]] - Array of the names of the properties to get. If the array is empty,
     *     all properties are returned.
     * @returns {Entities.EntityProperties} The properties of the entity if the entity can be found, otherwise an empty object.
     * @example <caption>Report the color of a new box entity.</caption>
     * var entityID = Entities.addEntity({
     *     type: "Box",
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -5 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 0.5, y: 0.5, z: 0.5 }
     * });
     * var properties = Entities.getEntityProperties(entityID, ["color"]);
     * print("Entity color: " + JSON.stringify(properties.color));
     */
/**
     * Update an entity with specified properties.
     * @function Entities.editEntity
     * @param {Uuid} entityID - The ID of the entity to edit.
     * @param {Entities.EntityProperties} properties - The properties to update the entity with.
     * @returns {Uuid} The ID of the entity if the edit was successful, otherwise <code>null</code>.
     * @example <caption>Change the color of an entity.</caption>
     * var entityID = Entities.addEntity({
     *     type: "Box",
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -5 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 0.5, y: 0.5, z: 0.5 }
     * });
     * var properties = Entities.getEntityProperties(entityID, ["color"]);
     * print("Entity color: " + JSON.stringify(properties.color));
     *
     * Entities.editEntity(entityID, {
     *     color: { red: 255, green: 0, blue: 0 }
     * });
     * properties = Entities.getEntityProperties(entityID, ["color"]);
     * print("Entity color: " + JSON.stringify(properties.color));
     */
/**
     * Delete an entity.
     * @function Entities.deleteEntity
     * @param {Uuid} entityID - The ID of the entity to delete.
     * @example <caption>Delete an entity a few seconds after creating it.</caption>
     * var entityID = Entities.addEntity({
     *     type: "Box",
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -5 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 0.5, y: 0.5, z: 0.5 }
     * });
     *
     * Script.setTimeout(function () {
     *     Entities.deleteEntity(entityID);
     * }, 3000);
     */
/**
     * Call a method in a client entity script from a client script or client entity script, or call a method in a server 
     * entity script from a server entity script. The entity script method must be exposed as a property in the target client 
     * entity script. Additionally, if calling a server entity script, the server entity script must include the method's name 
     * in an exposed property called <code>remotelyCallable</code> that is an array of method names that can be called.
     * @function Entities.callEntityMethod
     * @param {Uuid} entityID - The ID of the entity to call the method in.
     * @param {string} method - The name of the method to call.
     * @param {string[]} [parameters=[]] - The parameters to call the specified method with.
     */
/**
     * Call a method in a server entity script from a client script or client entity script. The entity script method must be 
     * exposed as a property in the target server entity script. Additionally, the target server entity script must include the 
     * method's name in an exposed property called <code>remotelyCallable</code> that is an array of method names that can be 
     * called.
     * @function Entities.callEntityServerMethod
     * @param {Uuid} entityID - The ID of the entity to call the method in.
     * @param {string} method - The name of the method to call.
     * @param {string[]} [parameters=[]] - The parameters to call the specified method with.
     */
/**
     * Call a method in a specific user's client entity script from a server entity script. The entity script method must be 
     * exposed as a property in the target client entity script.
     * @function Entities.callEntityClientMethod
     * @param {Uuid} clientSessionID - The session ID of the user to call the method in.
     * @param {Uuid} entityID - The ID of the entity to call the method in.
     * @param {string} method - The name of the method to call.
     * @param {string[]} [parameters=[]] - The parameters to call the specified method with.
     */
/**
     * Find the entity with a position closest to a specified point and within a specified radius.
     * @function Entities.findClosestEntity
     * @param {Vec3} center - The point about which to search.
     * @param {number} radius - The radius within which to search.
     * @returns {Uuid} The ID of the entity that is closest to the <code>center</code> and within the <code>radius</code> if
     *     there is one, otherwise <code>null</code>.
     * @example <caption>Find the closest entity within 10m of your avatar.</caption>
     * var entityID = Entities.findClosestEntity(MyAvatar.position, 10);
     * print("Closest entity: " + entityID);
     */
/**
     * Find all entities that intersect a sphere defined by a center point and radius.
     * @function Entities.findEntities
     * @param {Vec3} center - The point about which to search.
     * @param {number} radius - The radius within which to search.
     * @returns {Uuid[]} An array of entity IDs that were found that intersect the search sphere. The array is empty if no 
     *     entities could be found.
     * @example <caption>Report how many entities are within 10m of your avatar.</caption>
     * var entityIDs = Entities.findEntities(MyAvatar.position, 10);
     * print("Number of entities within 10m: " + entityIDs.length);
     */
/**
     * Find all entities whose axis-aligned boxes intersect a search axis-aligned box defined by its minimum coordinates corner
     * and dimensions.
     * @function Entities.findEntitiesInBox
     * @param {Vec3} corner - The corner of the search AA box with minimum co-ordinate values.
     * @param {Vec3} dimensions - The dimensions of the search AA box.
     * @returns {Uuid[]} An array of entity IDs whose AA boxes intersect the search AA box. The array is empty if no entities 
     *     could be found.
     */
/**
     * Find all entities whose axis-aligned boxes intersect a search frustum.
     * @function Entities.findEntitiesInFrustum
     * @param {ViewFrustum} frustum - The frustum to search in. The <code>position</code>, <code>orientation</code>, 
     *     <code>projection</code>, and <code>centerRadius</code> properties must be specified.
     * @returns {Uuid[]} An array of entity IDs axis-aligned boxes intersect the frustum. The array is empty if no entities 
     *     could be found.
     * @example <caption>Report the number of entities in view.</caption>
     * var entityIDs = Entities.findEntitiesInFrustum(Camera.frustum);
     * print("Number of entities in view: " + entityIDs.length);
     */
/**
     * Find all entities of a particular type that intersect a sphere defined by a center point and radius.
     * @function Entities.findEntitiesByType
     * @param {Entities.EntityType} entityType - The type of entity to search for.
     * @param {Vec3} center - The point about which to search.
     * @param {number} radius - The radius within which to search.
     * @returns {Uuid[]} An array of entity IDs of the specified type that intersect the search sphere. The array is empty if 
     *     no entities could be found.
     * @example <caption>Report the number of Model entities within 10m of your avatar.</caption>
     * var entityIDs = Entities.findEntitiesByType("Model", MyAvatar.position, 10);
     * print("Number of Model entities within 10m: " + entityIDs.length);
     */
/**
     * Find the first entity intersected by a {@link PickRay}. <code>Light</code> and <code>Zone</code> entities are not 
     * intersected unless they've been configured as pickable using {@link Entities.setLightsArePickable|setLightsArePickable}
     * and {@link Entities.setZonesArePickable|setZonesArePickable}, respectively.<br />
     * @function Entities.findRayIntersection
     * @param {PickRay} pickRay - The PickRay to use for finding entities.
     * @param {boolean} [precisionPicking=false] - If <code>true</code> and the intersected entity is a <code>Model</code> 
     *     entity, the result's <code>extraInfo</code> property includes more information than it otherwise would.
     * @param {Uuid[]} [entitiesToInclude=[]] - If not empty then the search is restricted to these entities.
     * @param {Uuid[]} [entitiesToDiscard=[]] - Entities to ignore during the search.
     * @param {boolean} [visibleOnly=false] - If <code>true</code> then only entities that are 
     *     <code>{@link Entities.EntityProperties|visible}<code> are searched.
     * @param {boolean} [collideableOnly=false] - If <code>true</code> then only entities that are not 
     *     <code>{@link Entities.EntityProperties|collisionless}</code> are searched.
     * @returns {Entities.RayToEntityIntersectionResult} The result of the search for the first intersected entity.
     * @example <caption>Find the entity directly in front of your avatar.</caption>
     * var pickRay = {
     *     origin: MyAvatar.position,
     *     direction: Quat.getFront(MyAvatar.orientation)
     * };
     *
     * var intersection = Entities.findRayIntersection(pickRay, true);
     * if (intersection.intersects) {
     *     print("Entity in front of avatar: " + intersection.entityID);
     * } else {
     *     print("No entity in front of avatar.");
     * }
     */
/**
     * Find the first entity intersected by a {@link PickRay}. <code>Light</code> and <code>Zone</code> entities are not 
     * intersected unless they've been configured as pickable using {@link Entities.setLightsArePickable|setLightsArePickable} 
     * and {@link Entities.setZonesArePickable|setZonesArePickable}, respectively.<br />
     * This is a synonym for {@link Entities.findRayIntersection|findRayIntersection}.
     * @function Entities.findRayIntersectionBlocking
     * @param {PickRay} pickRay - The PickRay to use for finding entities.
     * @param {boolean} [precisionPicking=false] - If <code>true</code> and the intersected entity is a <code>Model</code>
     *     entity, the result's <code>extraInfo</code> property includes more information than it otherwise would.
     * @param {Uuid[]} [entitiesToInclude=[]] - If not empty then the search is restricted to these entities.
     * @param {Uuid[]} [entitiesToDiscard=[]] - Entities to ignore during the search.
     * @deprecated This function is deprecated and will soon be removed. Use 
     *    {@link Entities.findRayIntersection|findRayIntersection} instead; it blocks and performs the same function.
     */
/**
     * Reloads an entity's server entity script such that the latest version re-downloaded.
     * @function Entities.reloadServerScripts
     * @param {Uuid} entityID - The ID of the entity to reload the server entity script of.
     * @returns {boolean} <code>true</code> if the reload request was successfully sent to the server, otherwise 
     *     <code>false</code>.
     */
/**
     * Gets the status of server entity script attached to an entity
     * @function Entities.getServerScriptStatus
     * @property {Uuid} entityID - The ID of the entity to get the server entity script status for.
     * @property {Entities~getServerScriptStatusCallback} callback - The function to call upon completion.
     * @returns {boolean} <code>true</code> always.
     */
/**
     * Called when {@link Entities.getServerScriptStatus} is complete.
     * @callback Entities~getServerScriptStatusCallback
     * @param {boolean} success - <code>true</code> if the server entity script status could be obtained, otherwise 
     *     <code>false</code>.
     * @param {boolean} isRunning - <code>true</code> if there is a server entity script running, otherwise <code>false</code>.
     * @param {string} status - <code>"running"</code> if there is a server entity script running, otherwise an error string.
     * @param {string} errorInfo - <code>""</code> if there is a server entity script running, otherwise it may contain extra 
     *     information on the error.
     */
/**
    * Get metadata for certain entity properties such as <code>script</code> and <code>serverScripts</code>.
    * @function Entities.queryPropertyMetadata
    * @param {Uuid} entityID - The ID of the entity to get the metadata for.
    * @param {string} property - The property name to get the metadata for.
    * @param {Entities~queryPropertyMetadataCallback} callback - The function to call upon completion.
    * @returns {boolean} <code>true</code> if the request for metadata was successfully sent to the server, otherwise 
    *     <code>false</code>.
    * @throws Throws an error if <code>property</code> is not handled yet or <code>callback</code> is not a function.
    */
/**
    * Get metadata for certain entity properties such as <code>script</code> and <code>serverScripts</code>.
    * @function Entities.queryPropertyMetadata
    * @param {Uuid} entityID - The ID of the entity to get the metadata for.
    * @param {string} property - The property name to get the metadata for.
    * @param {object} scope - The "<code>this</code>" context that the callback will be executed within.
    * @param {Entities~queryPropertyMetadataCallback} callback - The function to call upon completion.
    * @returns {boolean} <code>true</code> if the request for metadata was successfully sent to the server, otherwise 
    *     <code>false</code>.
    * @throws Throws an error if <code>property</code> is not handled yet or <code>callback</code> is not a function.
    */
/**
    * Called when {@link Entities.queryPropertyMetadata} is complete.
    * @callback Entities~queryPropertyMetadataCallback
    * @param {string} error - <code>undefined</code> if there was no error, otherwise an error message.
    * @param {object} result - The metadata for the requested entity property if there was no error, otherwise
    *     <code>undefined</code>.
    */
/**
     * Set whether or not ray picks intersect the bounding box of {@link Entities.EntityType|Light} entities. By default, Light 
     * entities are not intersected. The setting lasts for the Interface session. Ray picks are done using 
     *     {@link Entities.findRayIntersection|findRayIntersection} or 
     *     {@link Entities.findRayIntersectionBlocking|findRayIntersectionBlocking}, or the {@link Picks} and {@link RayPick} 
     *     APIs.
     * @function Entities.setLightsArePickable
     * @param {boolean} value - Set <code>true</code> to make ray picks intersect the bounding box of 
     *     {@link Entities.EntityType|Light} entities, otherwise <code>false</code>.
     */
/**
     * Get whether or not ray picks intersect the bounding box of {@link Entities.EntityType|Light} entities. Ray picks are 
     *     done using {@link Entities.findRayIntersection|findRayIntersection} or 
     *     {@link Entities.findRayIntersectionBlocking|findRayIntersectionBlocking}, or the {@link Picks} and {@link RayPick} 
     *     APIs.
     * @function Entities.getLightsArePickable
     * @returns {boolean} <code>true</code> if ray picks intersect the bounding box of {@link Entities.EntityType|Light} 
     *     entities, otherwise <code>false</code>.
     */
/**
     * Set whether or not ray picks intersect the bounding box of {@link Entities.EntityType|Zone} entities. By default, Light 
     * entities are not intersected. The setting lasts for the Interface session. Ray picks are done using 
     *     {@link Entities.findRayIntersection|findRayIntersection} or 
     *     {@link Entities.findRayIntersectionBlocking|findRayIntersectionBlocking}, or the {@link Picks} and {@link RayPick} 
     *     APIs.
     * @function Entities.setZonesArePickable
     * @param {boolean} value - Set <code>true</code> to make ray picks intersect the bounding box of 
     *     {@link Entities.EntityType|Zone} entities, otherwise <code>false</code>.
     */
/**
     * Get whether or not ray picks intersect the bounding box of {@link Entities.EntityType|Zone} entities. Ray picks are 
     *     done using {@link Entities.findRayIntersection|findRayIntersection} or 
     *     {@link Entities.findRayIntersectionBlocking|findRayIntersectionBlocking}, or the {@link Picks} and {@link RayPick} 
     *     APIs.
     * @function Entities.getZonesArePickable
     * @returns {boolean} <code>true</code> if ray picks intersect the bounding box of {@link Entities.EntityType|Zone} 
     *      entities, otherwise <code>false</code>.
     */
/**
     * Set whether or not {@link Entities.EntityType|Zone} entities' boundaries should be drawn. <em>Currently not used.</em>
     * @function Entities.setDrawZoneBoundaries
     * @param {boolean} value - Set to <code>true</code> if {@link Entities.EntityType|Zone} entities' boundaries should be 
     *     drawn, otherwise <code>false</code>.
     */
/**
    * Get whether or not {@link Entities.EntityType|Zone} entities' boundaries should be drawn. <em>Currently not used.</em>
    * @function Entities.getDrawZoneBoundaries
    * @returns {boolean} <code>true</code> if {@link Entities.EntityType|Zone} entities' boundaries should be drawn, 
    *    otherwise <code>false</code>.
    */
/**
     * Set the values of all voxels in a spherical portion of a {@link Entities.EntityType|PolyVox} entity.
     * @function Entities.setVoxelSphere
     * @param {Uuid} entityID - The ID of the {@link Entities.EntityType|PolyVox} entity.
     * @param {Vec3} center - The center of the sphere of voxels to set, in world coordinates.
     * @param {number} radius - The radius of the sphere of voxels to set, in world coordinates.
     * @param {number} value - If <code>value % 256 == 0</code> then each voxel is cleared, otherwise each voxel is set.
     * @example <caption>Create a PolyVox sphere.</caption>
     * var position = Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0.5, z: -8 }));
     * var polyVox = Entities.addEntity({
     *     type: "PolyVox",
     *     position: position,
     *     dimensions: { x: 2, y: 2, z: 2 },
     *     voxelVolumeSize: { x: 32, y: 32, z: 32 },
     *     lifetime: 300  // Delete after 5 minutes.
     * });
     * Entities.setVoxelSphere(polyVox, position, 0.9, 255);
     */
/**
     * Set the values of all voxels in a capsule-shaped portion of a {@link Entities.EntityType|PolyVox} entity.
     * @function Entities.setVoxelCapsule
     * @param {Uuid} entityID - The ID of the {@link Entities.EntityType|PolyVox} entity.
     * @param {Vec3} start - The center of the sphere of voxels to set, in world coordinates.
     * @param {Vec3} end - The center of the sphere of voxels to set, in world coordinates.
     * @param {number} radius - The radius of the capsule cylinder and spherical ends, in world coordinates.
     * @param {number} value - If <code>value % 256 == 0</code> then each voxel is cleared, otherwise each voxel is set.
     * @example <caption>Create a PolyVox capsule shape.</caption>
     * var position = Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0.5, z: -8 }));
     * var polyVox = Entities.addEntity({
     *     type: "PolyVox",
     *     position: position,
     *     dimensions: { x: 2, y: 2, z: 2 },
     *     voxelVolumeSize: { x: 32, y: 32, z: 32 },
     *     lifetime: 300  // Delete after 5 minutes.
     * });
     * var startPosition = Vec3.sum({ x: -0.5, y: 0, z: 0 }, position);
     * var endPosition = Vec3.sum({ x: 0.5, y: 0, z: 0 }, position);
     * Entities.setVoxelCapsule(polyVox, startPosition, endPosition, 0.5, 255);
     */
/**
     * Set the value of a particular voxels in a {@link Entities.EntityType|PolyVox} entity.
     * @function Entities.setVoxel
     * @param {Uuid} entityID - The ID of the {@link Entities.EntityType|PolyVox} entity.
     * @param {Vec3} position - The position relative to the minimum axes values corner of the entity. The 
     *     <code>position</code> coordinates are rounded to the nearest integer to get the voxel coordinate. The minimum axes 
     *     corner voxel is <code>{ x: 0, y: 0, z: 0 }</code>.
     * @param {number} value - If <code>value % 256 == 0</code> then voxel is cleared, otherwise the voxel is set.
     * @example <caption>Create a cube PolyVox entity and clear the minimum axes corner voxel.</caption>
     * var entity = Entities.addEntity({
     *     type: "PolyVox",
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0.5, z: -8 })),
     *     dimensions: { x: 2, y: 2, z: 2 },
     *     voxelVolumeSize: { x: 16, y: 16, z: 16 },
     *     lifetime: 300  // Delete after 5 minutes.
     * });
     * Entities.setAllVoxels(entity, 1);
     * Entities.setVoxel(entity, { x: 0, y: 0, z: 0 }, 0);
     */
/**
     * Set the values of all voxels in a {@link Entities.EntityType|PolyVox} entity.
     * @function Entities.setAllVoxels
     * @param {Uuid} entityID - The ID of the {@link Entities.EntityType|PolyVox} entity.
     * @param {number} value - If <code>value % 256 == 0</code> then each voxel is cleared, otherwise each voxel is set.
     * @example <caption>Create a PolyVox cube.</caption>
     * var entity = Entities.addEntity({
     *     type: "PolyVox",
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0.5, z: -8 })),
     *     dimensions: { x: 2, y: 2, z: 2 },
     *     voxelVolumeSize: { x: 16, y: 16, z: 16 },
     *     lifetime: 300  // Delete after 5 minutes.
     * });
     * Entities.setAllVoxels(entity, 1);
     */
/**
     * Set the values of all voxels in a cubic portion of a {@link Entities.EntityType|PolyVox} entity.
     * @function Entities.setVoxelsInCuboid
     * @param {Uuid} entityID - The ID of the {@link Entities.EntityType|PolyVox} entity.
     * @param {Vec3} lowPosition - The position of the minimum axes value corner of the cube of voxels to set, in voxel 
     *     coordinates.
     * @param {Vec3} cuboidSize - The size of the cube of voxels to set, in voxel coordinates.
     * @param {number} value - If <code>value % 256 == 0</code> then each voxel is cleared, otherwise each voxel is set.
     * @example <caption>Create a PolyVox cube and clear the voxels in one corner.</caption>
     * var polyVox = Entities.addEntity({
     *     type: "PolyVox",
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0.5, z: -8 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 2, y: 2, z: 2 },
     *     voxelVolumeSize: { x: 16, y: 16, z: 16 },
     *     lifetime: 300  // Delete after 5 minutes.
     * });
     * Entities.setAllVoxels(polyVox, 1);
     * var cuboidPosition = { x: 12, y: 12, z: 12 };
     * var cuboidSize = { x: 4, y: 4, z: 4 };
     * Entities.setVoxelsInCuboid(polyVox, cuboidPosition, cuboidSize, 0);
     */
/**
     * Convert voxel coordinates in a {@link Entities.EntityType|PolyVox} entity to world coordinates. Voxel coordinates are 
     * relative to the minimum axes values corner of the entity with a scale of <code>Vec3.ONE</code> being the dimensions of 
     * each voxel.
     * @function Entities.voxelCoordsToWorldCoords
     * @param {Uuid} entityID - The ID of the {@link Entities.EntityType|PolyVox} entity.
     * @param {Vec3} voxelCoords - The voxel coordinates. May be fractional and outside the entity's bounding box.
     * @returns {Vec3} The world coordinates of the <code>voxelCoords</code> if the <code>entityID</code> is a 
     *     {@link Entities.EntityType|PolyVox} entity, otherwise {@link Vec3|Vec3.ZERO}.
     * @example <caption>Create a PolyVox cube with the 0,0,0 voxel replaced by a sphere.</caption>
     * // Cube PolyVox with 0,0,0 voxel missing.
     * var polyVox = Entities.addEntity({
     *     type: "PolyVox",
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0.5, z: -8 })),
     *     dimensions: { x: 2, y: 2, z: 2 },
     *     voxelVolumeSize: { x: 16, y: 16, z: 16 },
     *     lifetime: 300  // Delete after 5 minutes.
     * });
     * Entities.setAllVoxels(polyVox, 1);
     * Entities.setVoxel(polyVox, { x: 0, y: 0, z: 0 }, 0);
     *
     * // Red sphere in 0,0,0 corner position.
     * var cornerPosition = Entities.voxelCoordsToWorldCoords(polyVox, { x: 0, y: 0, z: 0 });
     * var voxelDimensions = Vec3.multiply(2 / 16, Vec3.ONE);
     * var sphere = Entities.addEntity({
     *     type: "Sphere",
     *     position: Vec3.sum(cornerPosition, Vec3.multiply(0.5, voxelDimensions)),
     *     dimensions: voxelDimensions,
     *     color: { red: 255, green: 0, blue: 0 },
     *     lifetime: 300  // Delete after 5 minutes.
     * });
     */
/**
     * Convert world coordinates to voxel coordinates in a {@link Entities.EntityType|PolyVox} entity. Voxel coordinates are 
     * relative to the minimum axes values corner of the entity, with a scale of <code>Vec3.ONE</code> being the dimensions of 
     * each voxel.
     * @function Entities.worldCoordsToVoxelCoords
     * @param {Uuid} entityID - The ID of the {@link Entities.EntityType|PolyVox} entity.
     * @param {Vec3} worldCoords - The world coordinates. May be outside the entity's bounding box.
     * @returns {Vec3} The voxel coordinates of the <code>worldCoords</code> if the <code>entityID</code> is a 
     *     {@link Entities.EntityType|PolyVox} entity, otherwise {@link Vec3|Vec3.ZERO}. The value may be fractional.
     */
/**
     * Convert voxel coordinates in a {@link Entities.EntityType|PolyVox} entity to local coordinates relative to the minimum 
     * axes value corner of the entity, with the scale being the same as world coordinates.
     * @function Entities.voxelCoordsToLocalCoords
     * @param {Uuid} entityID - The ID of the {@link Entities.EntityType|PolyVox} entity.
     * @param {Vec3} voxelCoords - The voxel coordinates. May be fractional and outside the entity's bounding box.
     * @returns {Vec3} The local coordinates of the <code>voxelCoords</code> if the <code>entityID</code> is a 
     *     {@link Entities.EntityType|PolyVox} entity, otherwise {@link Vec3|Vec3.ZERO}.
     * @example <caption>Get the world dimensions of a voxel in a PolyVox entity.</caption>
     * var polyVox = Entities.addEntity({
     *     type: "PolyVox",
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0.5, z: -8 })),
     *     dimensions: { x: 2, y: 2, z: 2 },
     *     voxelVolumeSize: { x: 16, y: 16, z: 16 },
     *     lifetime: 300  // Delete after 5 minutes.
     * });
     * var voxelDimensions = Entities.voxelCoordsToLocalCoords(polyVox, Vec3.ONE);
     * print("Voxel dimensions: " + JSON.stringify(voxelDimensions));
     */
/**
     * Convert local coordinates to voxel coordinates in a {@link Entities.EntityType|PolyVox} entity. Local coordinates are 
     * relative to the minimum axes value corner of the entity, with the scale being the same as world coordinates.
     * @function Entities.localCoordsToVoxelCoords
     * @param {Uuid} entityID - The ID of the {@link Entities.EntityType|PolyVox} entity.
     * @param {Vec3} localCoords - The local coordinates. May be outside the entity's bounding box.
     * @returns {Vec3} The voxel coordinates of the <code>worldCoords</code> if the <code>entityID</code> is a 
     *     {@link Entities.EntityType|PolyVox} entity, otherwise {@link Vec3|Vec3.ZERO}. The value may be fractional.
     */
/**
     * Set the <code>linePoints</code> property of a {@link Entities.EntityType|Line} entity.
     * @function Entities.setAllPoints
     * @param {Uuid} entityID - The ID of the {@link Entities.EntityType|Line} entity.
     * @param {Vec3[]} points - The array of points to set the entity's <code>linePoints</code> property to.
     * @returns {boolean} <code>true</code> if the entity's property was updated, otherwise <code>false</code>. The property 
     *     may fail to be updated if the entity does not exist, the entity is not a {@link Entities.EntityType|Line} entity, 
     *     one of the points is outside the entity's dimensions, or the number of points is greater than the maximum allowed.
     * @example <caption>Change the shape of a Line entity.</caption>
     * // Draw a horizontal line between two points.
     * var entity = Entities.addEntity({
     *     type: "Line",
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0.75, z: -5 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 2, y: 2, z: 1 },
     *     linePoints: [
     *         { x: -1, y: 0, z: 0 },
     *         { x:1, y: -0, z: 0 }
     *     ],
     *     color: { red: 255, green: 0, blue: 0 },
     *     lifetime: 300  // Delete after 5 minutes.
     * });
     *
     * // Change the line to be a "V".
     * Script.setTimeout(function () {
     *     Entities.setAllPoints(entity, [
     *         { x: -1, y: 1, z: 0 },
     *         { x: 0, y: -1, z: 0 },
     *         { x: 1, y: 1, z: 0 },
     *     ]);
     * }, 2000);
     */
/**
     * Append a point to a {@link Entities.EntityType|Line} entity.
     * @function Entities.appendPoint
     * @param {Uuid} entityID - The ID of the {@link Entities.EntityType|Line} entity.
     * @param {Vec3} point - The point to add to the line. The coordinates are relative to the entity's position.
     * @returns {boolean} <code>true</code> if the point was added to the line, otherwise <code>false</code>. The point may 
     *     fail to be added if the entity does not exist, the entity is not a {@link Entities.EntityType|Line} entity, the 
     *     point is outside the entity's dimensions, or the maximum number of points has been reached.
     * @example <caption>Append a point to a Line entity.</caption>
     * // Draw a line between two points.
     * var entity = Entities.addEntity({
     *     type: "Line",
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0.75, z: -5 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 2, y: 2, z: 1 },
     *     linePoints: [
     *         { x: -1, y: 1, z: 0 },
     *         { x: 0, y: -1, z: 0 }
     *     ],
     *     color: { red: 255, green: 0, blue: 0 },
     *     lifetime: 300  // Delete after 5 minutes.
     * });
     *
     * // Add a third point to create a "V".
     * Entities.appendPoint(entity, { x: 1, y: 1, z: 0 });
     */
/**
     * Dumps debug information about all entities in Interface's local in-memory tree of entities it knows about &mdash; domain
     * and client-only &mdash; to the program log.
     * @function Entities.dumpTree
     */
/**
     * Add an action to an entity. An action is registered with the physics engine and is applied every physics simulation 
     * step. Any entity may have more than one action associated with it, but only as many as will fit in an entity's 
     * <code>actionData</code> property.
     * @function Entities.addAction
     * @param {Entities.ActionType} actionType - The type of action.
     * @param {Uuid} entityID - The ID of the entity to add the action to.
     * @param {Entities.ActionArguments} arguments - Configure the action.
     * @returns {Uuid} The ID of the action added if successfully added, otherwise <code>null</code>.
     * @example <caption>Constrain a cube to move along a vertical line.</caption>
     * var entityID = Entities.addEntity({
     *     type: "Box",
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0.75, z: -5 })),
     *     dimensions: { x: 0.5, y: 0.5, z: 0.5 },
     *     dynamic: true,
     *     collisionless: false,
     *     userData: "{ \"grabbableKey\": { \"grabbable\": true, \"kinematic\": false } }",
     *     lifetime: 300  // Delete after 5 minutes.
     * });
     *
     * var actionID = Entities.addAction("slider", entityID, {
     *     axis: { x: 0, y: 1, z: 0 },
     *     linearLow: 0,
     *     linearHigh: 0.6
     * });
     */
/**
     * Update an entity action.
     * @function Entities.updateAction
     * @param {Uuid} entityID - The ID of the entity with the action to update.
     * @param {Uuid} actionID - The ID of the action to update.
     * @param {Entities.ActionArguments} arguments - The arguments to update.
     * @returns {boolean} <code>true</code> if the update was successful, otherwise <code>false</code>.
     */
/**
     * Delete an action from an entity.
     * @function Entities.deleteAction
     * @param {Uuid} entityID - The ID of entity to delete the action from.
     * @param {Uuid} actionID - The ID of the action to delete.
     * @returns {boolean} <code>true</code> if the update was successful, otherwise <code>false</code>.
     */
/**
     * Get the IDs of the actions that  are associated with an entity.
     * @function Entities.getActionIDs
     * @param {Uuid} entityID - The entity to get the action IDs for.
     * @returns {Uuid[]} An array of action IDs if any are found, otherwise an empty array.
     */
/**
     * Get the arguments of an action.
     * @function Entities.getActionArguments
     * @param {Uuid} entityID - The ID of the entity with the action.
     * @param {Uuid} actionID - The ID of the action to get the arguments of.
     * @returns {Entities.ActionArguments} The arguments of the requested action if found, otherwise an empty object.
     */
/**
     * Get the translation of a joint in a {@link Entities.EntityType|Model} entity relative to the entity's position and 
     * orientation.
     * @function Entities.getAbsoluteJointTranslationInObjectFrame
     * @param {Uuid} entityID - The ID of the entity.
     * @param {number} jointIndex - The integer index of the joint.
     * @returns {Vec3} The translation of the joint relative to the entity's position and orientation if the entity is a
     *     {@link Entities.EntityType|Model} entity, the entity is loaded, and the joint index is valid; otherwise 
     *     <code>{@link Vec3(0)|Vec3.ZERO}</code>.
     */
/**
     * Get the translation of a joint in a {@link Entities.EntityType|Model} entity relative to the entity's position and 
     * orientation.
     * @function Entities.getAbsoluteJointRotationInObjectFrame
     * @param {Uuid} entityID - The ID of the entity.
     * @param {number} jointIndex - The integer index of the joint.
     * @returns {Quat} The rotation of the joint relative to the entity's orientation if the entity is a
     *     {@link Entities.EntityType|Model} entity, the entity is loaded, and the joint index is valid; otherwise 
     *     <code>{@link Quat(0)|Quat.IDENTITY}</code>.
     * @example <caption>Compare the local and absolute rotations of an avatar model's left hand joint.</caption>
     * entityID = Entities.addEntity({
     *     type: "Model",
     *     modelURL: "https://hifi-content.s3.amazonaws.com/milad/production/Examples/Models/Avatars/blue_suited.fbx",
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -5 })),
     *     rotation: MyAvatar.orientation,
     *     lifetime: 300  // Delete after 5 minutes.
     * });
     *
     * Script.setTimeout(function () {
     *     // Joint data aren't available until after the model has loaded.
     *     var index = Entities.getJointIndex(entityID, "LeftHand");
     *     var localRotation = Entities.getLocalJointRotation(entityID, index);
     *     var absoluteRotation = Entities.getAbsoluteJointRotationInObjectFrame(entityID, index);
     *     print("Left hand local rotation: " + JSON.stringify(Quat.safeEulerAngles(localRotation)));
     *     print("Left hand absolute rotation: " + JSON.stringify(Quat.safeEulerAngles(absoluteRotation)));
     * }, 2000);
     */
/**
     * Set the translation of a joint in a {@link Entities.EntityType|Model} entity relative to the entity's position and 
     * orientation.
     * @function Entities.setAbsoluteJointTranslationInObjectFrame
     * @param {Uuid} entityID - The ID of the entity.
     * @param {number} jointIndex - The integer index of the joint.
     * @param {Vec3} translation - The translation to set the joint to relative to the entity's position and orientation.
     * @returns {boolean} <code>true</code>if the entity is a {@link Entities.EntityType|Model} entity, the entity is loaded, 
     *     the joint index is valid, and the translation is different to the joint's current translation; otherwise 
     *     <code>false</code>.
     */
/**
     * Set the rotation of a joint in a {@link Entities.EntityType|Model} entity relative to the entity's position and 
     * orientation.
     * @function Entities.setAbsoluteJointRotationInObjectFrame
     * @param {Uuid} entityID - The ID of the entity.
     * @param {number} jointIndex - The integer index of the joint.
     * @param {Quat} rotation - The rotation to set the joint to relative to the entity's orientation.
     * @returns {boolean} <code>true</code> if the entity is a {@link Entities.EntityType|Model} entity, the entity is loaded, 
     *     the joint index is valid, and the rotation is different to the joint's current rotation; otherwise <code>false</code>.
     * @example <caption>Raise an avatar model's left palm.</caption>
     * entityID = Entities.addEntity({
     *     type: "Model",
     *     modelURL: "https://hifi-content.s3.amazonaws.com/milad/production/Examples/Models/Avatars/blue_suited.fbx",
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -5 })),
     *     rotation: MyAvatar.orientation,
     *     lifetime: 300  // Delete after 5 minutes.
     * });
     *
     * Script.setTimeout(function () {
     *     // Joint data aren't available until after the model has loaded.
     *     var index = Entities.getJointIndex(entityID, "LeftHand");
     *     var absoluteRotation = Entities.getAbsoluteJointRotationInObjectFrame(entityID, index);
     *     absoluteRotation = Quat.multiply(Quat.fromPitchYawRollDegrees(0, 0, 90), absoluteRotation);
     *     var success = Entities.setAbsoluteJointRotationInObjectFrame(entityID, index, absoluteRotation);
     *     print("Success: " + success);
     * }, 2000);
     */
/**
     * Get the local translation of a joint in a {@link Entities.EntityType|Model} entity.
     * @function Entities.getLocalJointTranslation
     * @param {Uuid} entityID - The ID of the entity.
     * @param {number} jointIndex - The integer index of the joint.
     * @returns {Vec3} The local translation of the joint if the entity is a {@link Entities.EntityType|Model} entity, the 
     *     entity is loaded, and the joint index is valid; otherwise <code>{@link Vec3(0)|Vec3.ZERO}</code>.
     */
/**
     * Get the local rotation of a joint in a {@link Entities.EntityType|Model} entity.
     * @function Entities.getLocalJointRotation
     * @param {Uuid} entityID - The ID of the entity.
     * @param {number} jointIndex - The integer index of the joint.
     * @returns {Quat} The local rotation of the joint if the entity is a {@link Entities.EntityType|Model} entity, the entity 
     *     is loaded, and the joint index is valid; otherwise <code>{@link Quat(0)|Quat.IDENTITY}</code>.
     * @example <caption>Report the local rotation of an avatar model's head joint.</caption>
     * entityID = Entities.addEntity({
     *     type: "Model",
     *     modelURL: "https://hifi-content.s3.amazonaws.com/milad/production/Examples/Models/Avatars/blue_suited.fbx",
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -5 })),
     *     rotation: MyAvatar.orientation,
     *     lifetime: 300  // Delete after 5 minutes.
     * });
     *
     * Script.setTimeout(function () {
     *     // Joint data aren't available until after the model has loaded.
     *     var index = Entities.getJointIndex(entityID, "Head");
     *     var rotation = Entities.getLocalJointRotation(entityID,  index);
     *     print("Head local rotation: " + JSON.stringify(Quat.safeEulerAngles(rotation)));
     * }, 2000);
     */
/**
     * Set the local translation of a joint in a {@link Entities.EntityType|Model} entity.
     * @function Entities.setLocalJointTranslation
     * @param {Uuid} entityID - The ID of the entity.
     * @param {number} jointIndex - The integer index of the joint.
     * @param {Vec3} translation - The local translation to set the joint to.
     * @returns {boolean} <code>true</code>if the entity is a {@link Entities.EntityType|Model} entity, the entity is loaded, 
     *     the joint index is valid, and the translation is different to the joint's current translation; otherwise 
     *     <code>false</code>.
     */
/**
     * Set the local rotation of a joint in a {@link Entities.EntityType|Model} entity.
     * @function Entities.setLocalJointRotation
     * @param {Uuid} entityID - The ID of the entity.
     * @param {number} jointIndex - The integer index of the joint.
     * @param {Quat} rotation - The local rotation to set the joint to.
     * @returns {boolean} <code>true</code> if the entity is a {@link Entities.EntityType|Model} entity, the entity is loaded, 
     *     the joint index is valid, and the rotation is different to the joint's current rotation; otherwise <code>false</code>.
     * @example <caption>Make an avatar model turn its head left.</caption>
     * entityID = Entities.addEntity({
     *     type: "Model",
     *     modelURL: "https://hifi-content.s3.amazonaws.com/milad/production/Examples/Models/Avatars/blue_suited.fbx",
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -5 })),
     *     rotation: MyAvatar.orientation,
     *     lifetime: 300  // Delete after 5 minutes.
     * });
     *
     * Script.setTimeout(function () {
     *     // Joint data aren't available until after the model has loaded.
     *     var index = Entities.getJointIndex(entityID, "Head");
     *     var rotation = Quat.fromPitchYawRollDegrees(0, 60, 0);
     *     var success = Entities.setLocalJointRotation(entityID, index, rotation);
     *     print("Success: " + success);
     * }, 2000);
     */
/**
     * Set the local translations of joints in a {@link Entities.EntityType|Model} entity.
     * @function Entities.setLocalJointTranslations
     * @param {Uuid} entityID - The ID of the entity.
     * @param {Vec3[]} translations - The local translations to set the joints to.
     * @returns {boolean} <code>true</code>if the entity is a {@link Entities.EntityType|Model} entity, the entity is loaded, 
     *     the model has joints, and at least one of the translations is different to the model's current translations; 
     *     otherwise <code>false</code>.
     */
/**
     * Set the local rotations of joints in a {@link Entities.EntityType|Model} entity.
     * @function Entities.setLocalJointRotations
     * @param {Uuid} entityID - The ID of the entity.
     * @param {Quat[]} rotations - The local rotations to set the joints to.
     * @returns {boolean} <code>true</code> if the entity is a {@link Entities.EntityType|Model} entity, the entity is loaded, 
     *     the model has joints, and at least one of the rotations is different to the model's current rotations; otherwise 
     *     <code>false</code>.
     * @example <caption>Raise both palms of an avatar model.</caption>
     * entityID = Entities.addEntity({
     *     type: "Model",
     *     modelURL: "https://hifi-content.s3.amazonaws.com/milad/production/Examples/Models/Avatars/blue_suited.fbx",
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -5 })),
     *     rotation: MyAvatar.orientation,
     *     lifetime: 300  // Delete after 5 minutes.
     * });
     *
     * Script.setTimeout(function () {
     *     // Joint data aren't available until after the model has loaded.
     *
     *     // Get all the joint rotations.
     *     var jointNames = Entities.getJointNames(entityID);
     *     var jointRotations = [];
     *     for (var i = 0, length = jointNames.length; i < length; i++) {
     *         var index = Entities.getJointIndex(entityID, jointNames[i]);
     *         jointRotations.push(Entities.getLocalJointRotation(entityID, index));
     *     }
     *
     *     // Raise both palms.
     *     var index = jointNames.indexOf("LeftHand");
     *     jointRotations[index] = Quat.multiply(Quat.fromPitchYawRollDegrees(-90, 0, 0), jointRotations[index]);
     *     index = jointNames.indexOf("RightHand");
     *     jointRotations[index] = Quat.multiply(Quat.fromPitchYawRollDegrees(-90, 0, 0), jointRotations[index]);
     *
     *     // Update all the joint rotations.
     *     var success = Entities.setLocalJointRotations(entityID, jointRotations);
     *     print("Success: " + success);
     * }, 2000);
     */
/**
     * Set the local rotations and translations of joints in a {@link Entities.EntityType|Model} entity. This is the same as 
     * calling both {@link Entities.setLocalJointRotations|setLocalJointRotations} and 
     * {@link Entities.setLocalJointTranslations|setLocalJointTranslations} at the same time.
     * @function Entities.setLocalJointsData
     * @param {Uuid} entityID - The ID of the entity.
     * @param {Quat[]} rotations - The local rotations to set the joints to.
     * @param {Vec3[]} translations - The local translations to set the joints to.
     * @returns {boolean} <code>true</code> if the entity is a {@link Entities.EntityType|Model} entity, the entity is loaded,
     *     the model has joints, and at least one of the rotations or translations is different to the model's current values; 
     *     otherwise <code>false</code>.
     */
/**
     * Get the index of a named joint in a {@link Entities.EntityType|Model} entity.
     * @function Entities.getJointIndex
     * @param {Uuid} entityID - The ID of the entity.
     * @param {string} name - The name of the joint.
     * @returns {number} The integer index of the joint if the entity is a {@link Entities.EntityType|Model} entity, the entity 
     *     is loaded, and the joint is present; otherwise <code>-1</code>. The joint indexes are in order per 
     *     {@link Entities.getJointNames|getJointNames}.
     * @example <caption>Report the index of a model's head joint.</caption>
     * entityID = Entities.addEntity({
     *     type: "Model",
     *     modelURL: "https://hifi-content.s3.amazonaws.com/milad/production/Examples/Models/Avatars/blue_suited.fbx",
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -5 })),
     *     rotation: MyAvatar.orientation,
     *     lifetime: 300  // Delete after 5 minutes.
     * });
     *
     * Script.setTimeout(function () {
     *     // Joint data aren't available until after the model has loaded.
     *     var index = Entities.getJointIndex(entityID, "Head");
     *     print("Head joint index: " + index);
     * }, 2000);
     */
/**
     * Get the names of all the joints in a {@link Entities.EntityType|Model} entity.
     * @function Entities.getJointNames
     * @param {Uuid} entityID - The ID of the {@link Entities.EntityType|Model} entity.
     * @returns {string[]} The names of all the joints in the entity if it is a {@link Entities.EntityType|Model} entity and 
     *     is loaded, otherwise an empty array. The joint names are in order per {@link Entities.getJointIndex|getJointIndex}.
     * @example <caption>Report a model's joint names.</caption>
     * entityID = Entities.addEntity({
     *     type: "Model",
     *     modelURL: "https://hifi-content.s3.amazonaws.com/milad/production/Examples/Models/Avatars/blue_suited.fbx",
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -5 })),
     *     rotation: MyAvatar.orientation,
     *     lifetime: 300  // Delete after 5 minutes.
     * });
     *
     * Script.setTimeout(function () {
     *     // Joint data aren't available until after the model has loaded.
     *     var jointNames = Entities.getJointNames(entityID);
     *     print("Joint names: " + JSON.stringify(jointNames));
     * }, 2000);
     */
/**
     * Get the IDs of entities, overlays, and avatars that are directly parented to an entity. To get all descendants of an 
     * entity, recurse on the IDs returned by the function.
     * @function Entities.getChildrenIDs
     * @param {Uuid} parentID - The ID of the entity to get the children IDs of.
     * @returns {Uuid[]} An array of entity, overlay, and avatar IDs that are parented directly to the <code>parentID</code> 
     *     entity. Does not include children's children, etc. The array is empty if no children can be found or 
     *     <code>parentID</code> cannot be found.
     * @example <caption>Report the children of an entity.</caption>
     * function createEntity(description, position, parent) {
     *     var entity = Entities.addEntity({
     *         type: "Sphere",
     *         position: position,
     *         dimensions: Vec3.HALF,
     *         parentID: parent,
     *         lifetime: 300  // Delete after 5 minutes.
     *     });
     *     print(description + ": " + entity);
     *     return entity;
     * }
     *
     * var position = Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 2, z: -5 }));
     * var root = createEntity("Root", position, Uuid.NULL);
     * var child = createEntity("Child", Vec3.sum(position, { x: 0, y: -1, z: 0 }), root);
     * var grandChild = createEntity("Grandchild", Vec3.sum(position, { x: 0, y: -2, z: 0 }), child);
     *
     * var children = Entities.getChildrenIDs(root);
     * print("Children of root: " + JSON.stringify(children));  // Only the child entity.
     */
/**
     * Get the IDs of entities, overlays, and avatars that are directly parented to an entity, overlay, or avatar model's joint.
     * @function Entities.getChildrenIDsOfJoint
     * @param {Uuid} parentID - The ID of the entity, overlay, or avatar to get the children IDs of.
     * @param {number} jointIndex - Integer number of the model joint to get the children IDs of.
     * @returns {Uuid[]} An array of entity, overlay, and avatar IDs that are parented directly to the <code>parentID</code> 
     *     entity, overlay, or avatar at the <code>jointIndex</code> joint. Does not include children's children, etc. The 
     *     array is empty if no children can be found or <code>parentID</code> cannot be found.
     * @example <caption>Report the children of your avatar's right hand.</caption>
     * function createEntity(description, position, parent) {
     *     var entity = Entities.addEntity({
     *         type: "Sphere",
     *         position: position,
     *         dimensions: Vec3.HALF,
     *         parentID: parent,
     *         lifetime: 300  // Delete after 5 minutes.
     *     });
     *     print(description + ": " + entity);
     *     return entity;
     * }
     *
     * var position = Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 2, z: -5 }));
     * var root = createEntity("Root", position, Uuid.NULL);
     * var child = createEntity("Child", Vec3.sum(position, { x: 0, y: -1, z: 0 }), root);
     *
     * Entities.editEntity(root, {
     *     parentID: MyAvatar.sessionUUID,
     *     parentJointIndex: MyAvatar.getJointIndex("RightHand")
     * });
     *
     * var children = Entities.getChildrenIDsOfJoint(MyAvatar.sessionUUID, MyAvatar.getJointIndex("RightHand"));
     * print("Children of hand: " + JSON.stringify(children));  // Only the root entity.
     */
/**
     * Check whether an entity or overlay has an entity as an ancestor (parent, parent's parent, etc.).
     * @function Entities.isChildOfParent
     * @param {Uuid} childID - The ID of the child entity or overlay to test for being a child, grandchild, etc.
     * @param {Uuid} parentID - The ID of the parent entity to test for being a parent, grandparent, etc.
     * @returns {boolean} <code>true</code> if the <code>childID</code> entity or overlay has the <code>parentID</code> entity 
     *     as a parent or grandparent etc., otherwise <code>false</code>.
     * @example <caption>Check that a grandchild entity is a child of its grandparent.</caption>
     * function createEntity(description, position, parent) {
     *     var entity = Entities.addEntity({
     *         type: "Sphere",
     *         position: position,
     *         dimensions: Vec3.HALF,
     *         parentID: parent,
     *         lifetime: 300  // Delete after 5 minutes.
     *     });
     *     print(description + ": " + entity);
     *     return entity;
     * }
     *
     * var position = Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 2, z: -5 }));
     * var root = createEntity("Root", position, Uuid.NULL);
     * var child = createEntity("Child", Vec3.sum(position, { x: 0, y: -1, z: 0 }), root);
     * var grandChild = createEntity("Grandchild", Vec3.sum(position, { x: 0, y: -2, z: 0 }), child);
     *
     * print("grandChild has root as parent: " + Entities.isChildOfParent(grandChild, root));  // true
     */
/**
     * Get the type &mdash; entity, overlay, or avatar &mdash; of an in-world item.
     * @function Entities.getNestableType
     * @param {Uuid} entityID - The ID of the item to get the type of.
     * @returns {string} The type of the item: <code>"entity"</code> if the item is an entity, <code>"overlay"</code> if the 
     *    the item is an overlay, <code>"avatar"</code> if the item is an avatar; otherwise <code>"unknown"</code> if the item 
     *    cannot be found.
     * @example <caption>Print some nestable types.</caption>
     * var entity = Entities.addEntity({
     *     type: "Sphere",
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 1, z: -2 })),
     *     lifetime: 300  // Delete after 5 minutes.
     * });
     *
     * print(Entities.getNestableType(entity));  // "entity"
     * print(Entities.getNestableType(Uuid.generate()));  // "unknown"
     */
/**
     * Get the ID of the {@link Entities.EntityType|Web} entity that has keyboard focus.
     * @function Entities.getKeyboardFocusEntity
     * @returns {Uuid} The ID of the {@link Entities.EntityType|Web} entity that has focus, if any, otherwise <code>null</code>.
     */
/**
     * Set the {@link Entities.EntityType|Web} entity that has keyboard focus.
     * @function Entities.setKeyboardFocusEntity
     * @param {Uuid} entityID - The ID of the {@link Entities.EntityType|Web} entity to set keyboard focus to. Use 
     *     <code>null</code> or {@link Uuid|Uuid.NULL} to unset keyboard focus from an entity.
     */
/**
     * Emit a {@link Entities.mousePressOnEntity|mousePressOnEntity} event.
     * @function Entities.sendMousePressOnEntity
     * @param {Uuid} entityID - The ID of the entity to emit the event for.
     * @param {PointerEvent} event - The event details.
     */
/**
     * Emit a {@link Entities.mouseMoveOnEntity|mouseMoveOnEntity} event.
     * @function Entities.sendMouseMoveOnEntity
     * @param {Uuid} entityID - The ID of the entity to emit the event for.
     * @param {PointerEvent} event - The event details.
     */
/**
     * Emit a {@link Entities.mouseReleaseOnEntity|mouseReleaseOnEntity} event.
     * @function Entities.sendMouseReleaseOnEntity
     * @param {Uuid} entityID - The ID of the entity to emit the event for.
     * @param {PointerEvent} event - The event details.
     */
/**
     * Emit a {@link Entities.clickDownOnEntity|clickDownOnEntity} event.
     * @function Entities.sendClickDownOnEntity
     * @param {Uuid} entityID - The ID of the entity to emit the event for.
     * @param {PointerEvent} event - The event details.
     */
/**
     * Emit a {@link Entities.holdingClickOnEntity|holdingClickOnEntity} event.
     * @function Entities.sendHoldingClickOnEntity
     * @param {Uuid} entityID - The ID of the entity to emit the event for.
     * @param {PointerEvent} event - The event details.
     */
/**
     * Emit a {@link Entities.clickReleaseOnEntity|clickReleaseOnEntity} event.
     * @function Entities.sendClickReleaseOnEntity
     * @param {Uuid} entityID - The ID of the entity to emit the event for.
     * @param {PointerEvent} event - The event details.
     */
/**
     * Emit a {@link Entities.hoverEnterEntity|hoverEnterEntity} event.
     * @function Entities.sendHoverEnterEntity
     * @param {Uuid} entityID - The ID of the entity to emit the event for.
     * @param {PointerEvent} event - The event details.
     */
/**
     * Emit a {@link Entities.hoverOverEntity|hoverOverEntity} event.
     * @function Entities.sendHoverOverEntity
     * @param {Uuid} entityID - The ID of the entity to emit the event for.
     * @param {PointerEvent} event - The event details.
     */
/**
     * Emit a {@link Entities.hoverLeaveEntity|hoverLeaveEntity} event.
     * @function Entities.sendHoverLeaveEntity
     * @param {Uuid} entityID - The ID of the entity to emit the event for.
     * @param {PointerEvent} event - The event details.
     */
/**
     * Check whether an entity wants hand controller pointer events. For example, a {@link Entities.EntityType|Web} entity does 
     * but a {@link Entities.EntityType|Shape} entity doesn't.
     * @function Entities.wantsHandControllerPointerEvents
     * @param {Uuid} entityID -  The ID of the entity.
     * @returns {boolean} <code>true</code> if the entity can be found and it wants hand controller pointer events, otherwise 
     *     <code>false</code>.
     */
/**
     * Send a script event over a {@link Entities.EntityType|Web} entity's <code>EventBridge</code> to the Web page's scripts.
     * @function Entities.emitScriptEvent
     * @param {Uuid} entityID - The ID of the {@link Entities.EntityType|Web} entity.
     * @param {string} message - The message to send.
     * @todo <em>This function is currently not implemented.</em>
     */
/**
     * Check whether an axis-aligned box and a capsule intersect.
     * @function Entities.AABoxIntersectsCapsule
     * @param {Vec3} brn - The bottom right near (minimum axes values) corner of the AA box.
     * @param {Vec3} dimensions - The dimensions of the AA box.
     * @param {Vec3} start - One end of the capsule.
     * @param {Vec3} end - The other end of the capsule.
     * @param {number} radius - The radiues of the capsule.
     * @returns {boolean} <code>true</code> if the AA box and capsule intersect, otherwise <code>false</code>.
     */
/**
     * Get the meshes in a {@link Entities.EntityType|Model} or {@link Entities.EntityType|PolyVox} entity.
     * @function Entities.getMeshes
     * @param {Uuid} entityID - The ID of the <code>Model</code> or <code>PolyVox</code> entity to get the meshes of.
     * @param {Entities~getMeshesCallback} callback - The function to call upon completion.
     * @deprecated Use the {@link Graphics} API instead.
     */
/**
      * Called when {@link Entities.getMeshes} is complete.
      * @callback Entities~getMeshesCallback
      * @param {MeshProxy[]} meshes - If <code>success<</code> is <code>true</code>, a {@link MeshProxy} per mesh in the 
      *     <code>Model</code> or <code>PolyVox</code> entity; otherwise <code>undefined</code>. 
      * @param {boolean} success - <code>true</code> if the {@link Entities.getMeshes} call was successful, <code>false</code> 
      *     otherwise. The call may be unsuccessful if the requested entity could not be found.
      * @deprecated Use the {@link Graphics} API instead.
      */
/**
     * Get the object to world transform, excluding scale, of an entity.
     * @function Entities.getEntityTransform
     * @param {Uuid} entityID - The ID of the entity.
     * @returns {Mat4} The entity's object to world transform excluding scale (i.e., translation and rotation, with scale of 1) 
     *    if the entity can be found, otherwise a transform with zero translation and rotation and a scale of 1.
     * @example <caption>Position and rotation in an entity's world transform.</caption>
     * var position = Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 1, z: -2 }));
     * var orientation = MyAvatar.orientation;
     * print("Position: " + JSON.stringify(position));
     * print("Orientation: " + JSON.stringify(orientation));
     *
     * var entityID = Entities.addEntity({
     *     type: "Sphere",
     *     position: position,
     *     rotation: orientation,
     *     dimensions: Vec3.HALF,
     *     lifetime: 300  // Delete after 5 minutes.
     * });
     *
     * var transform = Entities.getEntityTransform(entityID);
     * print("Transform: " + JSON.stringify(transform));
     * print("Translation: " + JSON.stringify(Mat4.extractTranslation(transform)));  // Same as position.
     * print("Rotation: " + JSON.stringify(Mat4.extractRotation(transform)));  // Same as orientation.
     * print("Scale: " + JSON.stringify(Mat4.extractScale(transform)));  // { x: 1, y: 1, z: 1 }
     */
/**
     * Get the object to parent transform, excluding scale, of an entity.
     * @function Entities.getEntityLocalTransform
     * @param {Uuid} entityID - The ID of the entity.
     * @returns {Mat4} The entity's object to parent transform excluding scale (i.e., translation and rotation, with scale of 
     *     1) if the entity can be found, otherwise a transform with zero translation and rotation and a scale of 1.
     * @example <caption>Position and rotation in an entity's local transform.</caption>
     * function createEntity(position, rotation, parent) {
     *     var entity = Entities.addEntity({
     *         type: "Box",
     *         position: position,
     *         rotation: rotation,
     *         dimensions: Vec3.HALF,
     *         parentID: parent,
     *         lifetime: 300  // Delete after 5 minutes.
     *     });
     *     return entity;
     * }
     *
     * var position = Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 2, z: -5 }));
     *
     * var parent = createEntity(position, MyAvatar.orientation, Uuid.NULL);
     *
     * var childTranslation = { x: 0, y: -1.5, z: 0 };
     * var childRotation = Quat.fromPitchYawRollDegrees(0, 45, 0);
     * var child = createEntity(Vec3.sum(position, childTranslation), Quat.multiply(childRotation, MyAvatar.orientation), parent);
     *
     * var transform = Entities.getEntityLocalTransform(child);
     * print("Transform: " + JSON.stringify(transform));
     * print("Translation: " + JSON.stringify(Mat4.extractTranslation(transform)));  // childTranslation
     * print("Rotation: " + JSON.stringify(Quat.safeEulerAngles(Mat4.extractRotation(transform))));  // childRotation
     * print("Scale: " + JSON.stringify(Mat4.extractScale(transform)));  // { x: 1, y: 1, z: 1 }     */
/**
    * Get the static certificate for an entity. The static certificate contains static properties of the item which cannot 
    * be altered.
    * @function Entities.getStaticCertificateJSON
    * @param {Uuid} entityID - The ID of the entity to get the static certificate for.
    * @returns {string} The entity's static certificate as a JSON string if the entity can be found, otherwise an empty string.
    */
/**
     * Verify the entity's proof of provenance, i.e., that the entity's <code>certificateID</code> property was produced by 
     * High Fidelity signing the entity's static certificate JSON.
     * @function Entities.verifyStaticCertificateProperties
     * @param {Uuid} entityID - The ID of the entity to verify.
     * @returns {boolean} <code>true</code> if the entity can be found an its <code>certificateID</code> property is present 
     *     and its value matches the entity's static certificate JSON; otherwise <code>false</code>.
     */
/**
     * Triggered on the client that is the physics simulation owner during the collision of two entities. Note: Isn't triggered 
     * for a collision with an avatar.
     * @function Entities.collisionWithEntity
     * @param {Uuid} idA - The ID of one entity in the collision. For an entity script, this is the ID of the entity containing 
     *     the script.
     * @param {Uuid} idB - The ID of the other entity in the collision.
     * @param {Collision} collision - The details of the collision.
     * @returns {Signal}
     * @example <caption>Change the color of an entity when it collides with another entity.</caption>
     * var entityScript = (function () {
     *     function randomInteger(min, max) {
     *         return Math.floor(Math.random() * (max - min + 1)) + min;
     *     }
     *
     *     this.collisionWithEntity = function (myID, otherID, collision) {
     *         Entities.editEntity(myID, {
     *             color: {
     *                 red: randomInteger(128, 255),
     *                 green: randomInteger(128, 255),
     *                 blue: randomInteger(128, 255)
     *             }
     *         });
     *     };
     * });
     *
     * var entityID = Entities.addEntity({
     *     type: "Box",
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -5 })),
     *     dimensions: { x: 0.5, y: 0.5, z: 0.5 },
     *     color: { red: 128, green: 128, blue: 128 },
     *     gravity: { x: 0, y: -9.8, z: 0 },
     *     velocity: { x: 0, y: 0.1, z: 0 },  // Kick off physics.
     *     dynamic: true,
     *     collisionless: false,  // So that collision events are generated.
     *     script: "(" + entityScript + ")",  // Could host the script on a Web server instead.
     *     lifetime: 300  // Delete after 5 minutes.
     * });
     */
/**
     * Triggered when your ability to change the <code>locked</code> property of entities changes.
     * @function Entities.canAdjustLocksChanged
     * @param {boolean} canAdjustLocks - <code>true</code> if the script can change the <code>locked</code> property of an 
     *     entity, otherwise <code>false</code>.
     * @returns {Signal}
     * @example <caption>Report when your ability to change locks changes.</caption>
     * function onCanAdjustLocksChanged(canAdjustLocks) {
     *     print("You can adjust entity locks: " + canAdjustLocks);
     * }
     * Entities.canAdjustLocksChanged.connect(onCanAdjustLocksChanged);
     */
/**
     * Triggered when your ability to rez (create) entities changes.
     * @function Entities.canRezChanged
     * @param {boolean} canRez - <code>true</code> if the script can rez (create) entities, otherwise <code>false</code>.
     * @returns {Signal}
     */
/**
     * Triggered when your ability to rez (create) temporary entities changes. Temporary entities are entities with a finite
     * <code>lifetime</code> property value set.
     * @function Entities.canRezTmpChanged
     * @param {boolean} canRezTmp - <code>true</code> if the script can rez (create) temporary entities, otherwise 
     *     <code>false</code>.
     * @returns {Signal}
     */
/**
     * Triggered when your ability to rez (create) certified entities changes. Certified entities are entities that have PoP
     * certificates.
     * @function Entities.canRezCertifiedChanged
     * @param {boolean} canRezCertified - <code>true</code> if the script can rez (create) certified entities, otherwise 
     *     <code>false</code>.
     * @returns {Signal}
     */
/**
     * Triggered when your ability to rez (create) temporary certified entities changes. Temporary entities are entities with a
     * finite <code>lifetime</code> property value set. Certified entities are entities that have PoP certificates.
     * @function Entities.canRezTmpCertifiedChanged
     * @param {boolean} canRezTmpCertified - <code>true</code> if the script can rez (create) temporary certified entities,
     *     otherwise <code>false</code>.
     * @returns {Signal}
     */
/**
     * Triggered when your ability to make changes to the asset server's assets changes.
     * @function Entities.canWriteAssetsChanged
     * @param {boolean} canWriteAssets - <code>true</code> if the script can change the <code>?</code> property of an entity,
     *     otherwise <code>false</code>.
     * @returns {Signal}
     */
/**
     * Triggered when a mouse button is clicked while the mouse cursor is on an entity, or a controller trigger is fully 
     * pressed while its laser is on an entity.
     * @function Entities.mousePressOnEntity
     * @param {Uuid} entityID - The ID of the entity that was pressed.
     * @param {PointerEvent} event - Details of the event.
     * @returns {Signal}
     * @example <caption>Report when an entity is clicked with the mouse or laser.</caption>
     * function onMousePressOnEntity(entityID, event) {
     *     print("Clicked on entity: " + entityID);
     * }
     *
     * Entities.mousePressOnEntity.connect(onMousePressOnEntity);
     */
/**
     * Triggered when a mouse button is double-clicked while the mouse cursor is on an entity.
     * @function Entities.mousePressOnEntity
     * @param {Uuid} entityID - The ID of the entity that was double-pressed.
     * @param {PointerEvent} event - Details of the event.
     * @returns {Signal}
     */
/**
     * Repeatedly triggered while the mouse cursor or controller laser moves on an entity.
     * @function Entities.mouseMoveOnEntity
     * @param {Uuid} entityID - The ID of the entity that was moved on.
     * @param {PointerEvent} event - Details of the event.
     * @returns {Signal}
     */
/**
     * Triggered when a mouse button is released after clicking on an entity or the controller trigger is partly or fully 
     * released after pressing on an entity, even if the mouse pointer or controller laser has moved off the entity.
     * @function Entities.mouseReleaseOnEntity
     * @param {Uuid} entityID - The ID of the entity that was originally pressed.
     * @param {PointerEvent} event - Details of the event.
     * @returns {Signal}
     */
/**
     * Triggered when a mouse button is clicked while the mouse cursor is not on an entity.
     * @function Entities.mousePressOffEntity
     * @param {PointerEvent} event - Details of the event.
     * @returns {Signal}
     */
/**
     * Triggered when a mouse button is double-clicked while the mouse cursor is not on an entity.
     * @function Entities.mouseDoublePressOffEntity
     * @param {PointerEvent} event - Details of the event.
     * @returns {Signal}
     */
/**
     * Triggered when a mouse button is clicked while the mouse cursor is on an entity. Note: Not triggered by controller.
     * @function Entities.clickDownOnEntity
     * @param {Uuid} entityID - The ID of the entity that was clicked.
     * @param {PointerEvent} event - Details of the event.
     * @returns {Signal}
     */
/**
     * Repeatedly triggered while a mouse button continues to be held after clicking an entity, even if the mouse cursor has 
     * moved off the entity. Note: Not triggered by controller.
     * @function Entities.holdingClickOnEntity
     * @param {Uuid} entityID - The ID of the entity that was originally clicked.
     * @param {PointerEvent} event - Details of the event.
     * @returns {Signal}
     */
/**
     * Triggered when a mouse button is released after clicking on an entity, even if the mouse cursor has moved off the 
     * entity. Note: Not triggered by controller.
     * @function Entities.clickReleaseOnEntity
     * @param {Uuid} entityID - The ID of the entity that was originally clicked.
     * @param {PointerEvent} event - Details of the event.
     * @returns {Signal}
     */
/**
     * Triggered when the mouse cursor or controller laser starts hovering on an entity.
     * @function Entities.hoverEnterEntity
     * @param {Uuid} entityID - The ID of the entity that is being hovered.
     * @param {PointerEvent} event - Details of the event.
     * @returns {Signal}
     */
/**
     * Repeatedly triggered when the mouse cursor or controller laser moves while hovering over an entity.
     * @function Entities.hoverOverEntity
     * @param {Uuid} entityID - The ID of the entity that is being hovered.
     * @param {PointerEvent} event - Details of the event.
     * @returns {Signal}
     */
/**
     * Triggered when the mouse cursor or controller laser stops hovering over an entity.
     * @function Entities.hoverLeaveEntity
     * @param {Uuid} entityID - The ID of the entity that was being hovered.
     * @param {PointerEvent} event - Details of the event.
     * @returns {Signal}
     */
/**
     * Triggered when an avatar enters an entity.
     * @function Entities.enterEntity
     * @param {Uuid} entityID - The ID of the entity that the avatar entered.
     * @returns {Signal}
     * @example <caption>Change the color of an entity when an avatar enters or leaves.</caption>
     * var entityScript = (function () {
     *     this.enterEntity = function (entityID) {
     *         print("Enter entity");
     *         Entities.editEntity(entityID, {
     *             color: { red: 255, green: 64, blue: 64 },
     *         });
     *     };
     *     this.leaveEntity = function (entityID) {
     *         print("Leave entity");
     *         Entities.editEntity(entityID, {
     *             color: { red: 128, green: 128, blue: 128 },
     *         });
     *     };
     * });
     *
     * var entityID = Entities.addEntity({
     *     type: "Sphere",
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -5 })),
     *     dimensions: { x: 3, y: 3, z: 3 },
     *     color: { red: 128, green: 128, blue: 128 },
     *     collisionless: true,  // So that avatar can walk through entity.
     *     script: "(" + entityScript + ")",  // Could host the script on a Web server instead.
     *     lifetime: 300  // Delete after 5 minutes.
     * });
     */
/**
     * Triggered when an avatar leaves an entity.
     * @function Entities.leaveEntity
     * @param {Uuid} entityID - The ID of the entity that the avatar left.
     * @returns {Signal}
     */
/**
     * Triggered when an entity is deleted.
     * @function Entities.deletingEntity
     * @param {Uuid} entityID - The ID of the entity deleted.
     * @returns {Signal}
     * @example <caption>Report when an entity is deleted.</caption>
     * Entities.deletingEntity.connect(function (entityID) {
     *     print("Deleted entity: " + entityID);
     * });
     */
/**
     * Triggered when an entity is added to Interface's local in-memory tree of entities it knows about. This may occur when 
     * entities are loaded upon visiting a domain, when the user rotates their view so that more entities become visible, and 
     * when a domain or client-only entity is added (e.g., by {@Entities.addEntity|addEntity}).
     * @function Entities.addingEntity
     * @param {Uuid} entityID - The ID of the entity added.
     * @returns {Signal}
     * @example <caption>Report when an entity is added.</caption>
     * Entities.addingEntity.connect(function (entityID) {
     *     print("Added entity: " + entityID);
     * });
     */
/**
     * Triggered when you disconnect from a domain, at which time Interface's local in-memory tree of entities it knows about
     * is cleared.
     * @function Entities.clearingEntities
     * @returns {Signal}
     * @example <caption>Report when Interfaces's entity tree is cleared.</caption>
     * Entities.clearingEntities.connect(function () {
     *     print("Entities cleared");
     * });
     */
/**
     * Triggered in when a script in a {@link Entities.EntityType|Web} entity's Web page script sends an event over the 
     * script's <code>EventBridge</code>.
     * @function Entities.webEventReceived
     * @param {Uuid} entityID - The ID of the entity that event was received from.
     * @param {string} message - The message received.
     * @returns {Signal}
     */
/**
     * <p>An entity may be one of the following types:</p>
     * <table>
     *   <thead>
     *     <tr><th>Value</th><th>Description</th><th>Properties</th></tr>
     *   </thead>
     *   <tbody>
     *     <tr><td><code>"Box"</code></td><td>A rectangular prism. This is a synonym of <code>"Shape"</code> for the case 
     *       where the entity's <code>shape</code> property value is <code>"Cube"</code>.<br />
     *       If an entity is created with its <code>type</code> 
     *       set to <code>"Box"</code> it will always be created with a <code>shape</code> property value of 
     *       <code>"Cube"</code>. If an entity of type <code>Shape</code> or <code>Sphere</code> has its <code>shape</code> set 
     *       to <code>"Cube"</code> then its <code>type</code> will be reported as <code>"Box"</code>.
     *       <td>{@link Entities.EntityProperties-Box|EntityProperties-Box}</td></tr>
     *     <tr><td><code>"Light"</code></td><td>A local lighting effect.</td>
     *       <td>{@link Entities.EntityProperties-Light|EntityProperties-Light}</td></tr>
     *     <tr><td><code>"Line"</code></td><td>A sequence of one or more simple straight lines.</td>
     *       <td>{@link Entities.EntityProperties-Line|EntityProperties-Line}</td></tr>
     *     <tr><td><code>"Material"</code></td><td>Modifies the existing materials on Model entities, Shape entities (albedo 
     *       only), {@link Overlays.OverlayType|model overlays}, and avatars.</td>
     *       <td>{@link Entities.EntityProperties-Material|EntityProperties-Material}</td></tr>
     *     <tr><td><code>"Model"</code></td><td>A mesh model from an FBX or OBJ file.</td>
     *       <td>{@link Entities.EntityProperties-Model|EntityProperties-Model}</td></tr>
     *     <tr><td><code>"ParticleEffect"</code></td><td>A particle system that can be used to simulate things such as fire, 
     *       smoke, snow, magic spells, etc.</td>
     *       <td>{@link Entities.EntityProperties-ParticleEffect|EntityProperties-ParticleEffect}</td></tr>
     *     <tr><td><code>"PolyLine"</code></td><td>A sequence of one or more textured straight lines.</td>
     *       <td>{@link Entities.EntityProperties-PolyLine|EntityProperties-PolyLine}</td></tr>
     *     <tr><td><code>"PolyVox"</code></td><td>A set of textured voxels.</td>
     *       <td>{@link Entities.EntityProperties-PolyVox|EntityProperties-PolyVox}</td></tr>
     *     <tr><td><code>"Shape"</code></td><td>A basic entity such as a cube.
     *       See also, the <code>"Box"</code> and <code>"Sphere"</code> entity types.</td>
     *       <td>{@link Entities.EntityProperties-Shape|EntityProperties-Shape}</td></tr>
     *     <tr><td><code>"Sphere"</code></td><td>A sphere. This is a synonym of <code>"Shape"</code> for the case
     *       where the entity's <code>shape</code> property value is <code>"Sphere"</code>.<br />
     *       If an entity is created with its <code>type</code>
     *       set to <code>"Sphere"</code> it will always be created with a <code>shape</code> property value of
     *       <code>"Sphere"</code>. If an entity of type <code>Box</code> or <code>Shape</code> has its <code>shape</code> set
     *       to <code>"Sphere"</code> then its <code>type</code> will be reported as <code>"Sphere"</code>.
     *       <td>{@link Entities.EntityProperties-Sphere|EntityProperties-Sphere}</td></tr>
     *     <tr><td><code>"Text"</code></td><td>A pane of text oriented in space.</td>
     *       <td>{@link Entities.EntityProperties-Text|EntityProperties-Text}</td></tr>
     *     <tr><td><code>"Web"</code></td><td>A browsable Web page.</td>
     *       <td>{@link Entities.EntityProperties-Web|EntityProperties-Web}</td></tr>
     *     <tr><td><code>"Zone"</code></td><td>A volume of lighting effects and avatar permissions.</td>
     *       <td>{@link Entities.EntityProperties-Zone|EntityProperties-Zone}</td></tr>
     *   </tbody>
     * </table>
     * @typedef {string} Entities.EntityType
     */
/**
 * Haze is defined by the following properties.
 * @typedef {object} Entities.Haze
 *
 * @property {number} hazeRange=1000 - The horizontal distance at which visibility is reduced to 95%; i.e., 95% of each pixel's 
 *     color is haze.
 * @property {Color} hazeColor=128,154,179 - The color of the haze when looking away from the key light.
 * @property {boolean} hazeEnableGlare=false - If <code>true</code> then the haze is colored with glare from the key light;
 *     <code>hazeGlareColor</code> and <code>hazeGlareAngle</code> are used.
 * @property {Color} hazeGlareColor=255,299,179 - The color of the haze when looking towards the key light.
 * @property {number} hazeGlareAngle=20 - The angle in degrees across the circle around the key light that the glare color and 
 *     haze color are blended 50/50.
 *
 * @property {boolean} hazeAltitudeEffect=false - If <code>true</code> then haze decreases with altitude as defined by the 
 *     entity's local coordinate system; <code>hazeBaseRef</code> and </code>hazeCeiling</code> are used.
 * @property {number} hazeBaseRef=0 - The y-axis value in the entity's local coordinate system at which the haze density starts 
 *     reducing with altitude.
 * @property {number} hazeCeiling=200 - The y-axis value in the entity's local coordinate system at which the haze density has 
 *     reduced to 5%.
 *
 * @property {number} hazeBackgroundBlend=0 - The proportion of the skybox image to show through the haze: <code>0.0</code> 
 *     displays no skybox image; <code>1.0</code> displays no haze.
 *
 * @property {boolean} hazeAttenuateKeyLight=false - <em>Currently not supported.</em>
 * @property {number} hazeKeyLightRange=1000 - <em>Currently not supported.</em>
 * @property {number} hazeKeyLightAltitude=200 - <em>Currently not supported.</em>
 */
/**
 * A key light is defined by the following properties.
 * @typedef {object} Entities.KeyLight
 * @property {Color} color=255,255,255 - The color of the light.
 * @property {number} intensity=1 - The intensity of the light.
 * @property {Vec3} direction=0,-1,0 - The direction the light is shining.
 * @property {boolean} castShadows=false - If <code>true</code> then shadows are cast. Shadows are cast by avatars, plus 
 *     {@link Entities.EntityType|Model} and {@link Entities.EntityType|Shape} entities that have their 
 *     <code>{@link Entities.EntityProperties|canCastShadows}</code> property set to <code>true</code>.
 */
/**
     * <p>A <code>PolyVoxSurfaceStyle</code> may be one of the following:</p>
     * <table>
     *   <thead>
     *     <tr><th>Value</th><th>Type</th><th>Description</th></tr>
     *   </thead>
     *   <tbody>
     *     <tr><td><code>0</code></td><td>Marching cubes.</td><td>Chamfered edges. Open volume.
     *       Joins neighboring PolyVox entities reasonably well.</td></tr>
     *     <tr><td><code>1</code></td><td>Cubic.</td><td>Square edges. Open volume.
     *       Joins neighboring PolyVox entities cleanly.</td></tr>
     *     <tr><td><code>2</code></td><td>Edged cubic.</td><td>Square edges. Enclosed volume.
     *       Joins neighboring PolyVox entities cleanly.</td></tr>
     *     <tr><td><code>3</code></td><td>Edged marching cubes.</td><td>Chamfered edges. Enclosed volume.
     *       Doesn't join neighboring PolyVox entities.</td></tr>
     *   </tbody>
     * </table>
     * @typedef {number} Entities.PolyVoxSurfaceStyle
     */
/**
     * <p>A <code>Shape</code>, <code>Box</code>, or <code>Sphere</code> {@link Entities.EntityType|EntityType} may display as 
     * one of the following geometrical shapes:</p>
     * <table>
     *   <thead>
     *     <tr><th>Value</th><th>Dimensions</th><th>Notes</th></tr>
     *   </thead>
     *   <tbody>
     *     <tr><td><code>"Circle"</code></td><td>2D</td><td>A circle oriented in 3D.</td></tr>
     *     <tr><td><code>"Cube"</code></td><td>3D</td><td></td></tr>
     *     <tr><td><code>"Cone"</code></td><td>3D</td><td></td></tr>
     *     <tr><td><code>"Cylinder"</code></td><td>3D</td><td></td></tr>
     *     <tr><td><code>"Dodecahedron"</code></td><td>3D</td><td></td></tr>
     *     <tr><td><code>"Hexagon"</code></td><td>3D</td><td>A hexagonal prism.</td></tr>
     *     <tr><td><code>"Icosahedron"</code></td><td>3D</td><td></td></tr>
     *     <tr><td><code>"Octagon"</code></td><td>3D</td><td>An octagonal prism.</td></tr>
     *     <tr><td><code>"Octahedron"</code></td><td>3D</td><td></td></tr>
     *     <tr><td><code>"Quad"</code></td><td>2D</td><td>A square oriented in 3D.</td></tr>
     *     <tr><td><code>"Sphere"</code></td><td>3D</td><td></td></tr>
     *     <tr><td><code>"Tetrahedron"</code></td><td>3D</td><td></td></tr>
     *     <tr><td><code>"Torus"</code></td><td>3D</td><td><em>Not implemented.</em></td></tr>
     *     <tr><td><code>"Triangle"</code></td><td>3D</td><td>A triangular prism.</td></tr>
     *   </tbody>
     * </table>
     * @typedef {string} Entities.Shape
     */
/**
 * A skybox is defined by the following properties.
 * @typedef {object} Entities.Skybox
 * @property {Color} color=0,0,0 - Sets the color of the sky if <code>url</code> is <code>""</code>, otherwise modifies the 
 *     color of the cube map image.
 * @property {string} url="" - A cube map image that is used to render the sky.
 */
/**
 * <p>An RGB or SRGB color value.</p>
 * <table>
 *   <thead>
 *     <tr><th>Index</th><th>Type</th><th>Attributes</th><th>Default</th><th>Value</th></tr>
 *   </thead>
 *   <tbody>
 *     <tr><td><code>0</code></td><td>number</td><td></td><td></td>
 *       <td>Red component value. Number in the range <code>0.0</code> &ndash; <code>1.0</code>.</td></tr>
 *     <tr><td><code>1</code></td><td>number</td><td></td><td></td>
 *       <td>Green component value. Number in the range <code>0.0</code> &ndash; <code>1.0</code>.</td></tr>
 *     <tr><td><code>2</code></td><td>number</td><td></td><td></td>
 *       <td>Blue component value. Number in the range <code>0.0</code> &ndash; <code>1.0</code>.</td></tr>
 *     <tr><td><code>3</code></td><td>boolean</td><td>&lt;optional&gt;</td><td>false</td>
 *       <td>If <code>true</code> then the color is an SRGB color.</td></tr>
 *   </tbody>
 * </table>
 * @typedef {array} RGBS
 */
/**
 * A material or set of materials such as may be used by a {@link Entities.EntityType|Material} entity.
 * @typedef {object} MaterialResource
 * @property {number} materialVersion=1 - The version of the material. <em>Currently not used.</em>
 * @property {Material|Material[]} materials - The details of the material or materials.
 */
/**
 * A material such as may be used by a {@link Entities.EntityType|Material} entity.
 * @typedef {object} Material
 * @property {string} name="" - A name for the material.
 * @property {string} model="hifi_pbr" - <em>Currently not used.</em>
 * @property {Vec3Color|RGBS} emissive - The emissive color, i.e., the color that the material emits. A {@link Vec3Color} value 
 *     is treated as sRGB. A {@link RGBS} value can be either RGB or sRGB.
 * @property {number} opacity=1.0 - The opacity, <code>0.0</code> &ndash; <code>1.0</code>.
 * @property {boolean} unlit=false - If <code>true</code>, the material is not lit.
 * @property {Vec3Color|RGBS} albedo - The albedo color. A {@link Vec3Color} value is treated as sRGB. A {@link RGBS} value can 
 *     be either RGB or sRGB.
 * @property {number} roughness - The roughness, <code>0.0</code> &ndash; <code>1.0</code>.
 * @property {number} metallic - The metallicness, <code>0.0</code> &ndash; <code>1.0</code>.
 * @property {number} scattering - The scattering, <code>0.0</code> &ndash; <code>1.0</code>.
 * @property {string} emissiveMap - URL of emissive texture image.
 * @property {string} albedoMap - URL of albedo texture image.
 * @property {string} opacityMap - URL of opacity texture image. Set value the same as the <code>albedoMap</code> value for 
 *     transparency.
 * @property {string} roughnessMap - URL of roughness texture image. Can use this or <code>glossMap</code>, but not both.
 * @property {string} glossMap - URL of gloss texture image. Can use this or <code>roughnessMap</code>, but not both.
 * @property {string} metallicMap - URL of metallic texture image. Can use this or <code>specularMap</code>, but not both.
 * @property {string} specularMap - URL of specular texture image. Can use this or <code>metallicMap</code>, but not both.
 * @property {string} normalMap - URL of normal texture image. Can use this or <code>bumpMap</code>, but not both.
 * @property {string} bumpMap - URL of bump texture image. Can use this or <code>normalMap</code>, but not both.
 * @property {string} occlusionMap - URL of occlusion texture image.
 * @property {string} scatteringMap - URL of scattering texture image. Only used if <code>normalMap</code> or 
 *     <code>bumpMap</code> is specified.
 * @property {string} lightMap - URL of light map texture image. <em>Currently not used.</em>
 */
/**
     * Set the maximum number of entity packets that the client can send per second.
     * @function Entities.setPacketsPerSecond
     * @param {number} packetsPerSecond - Integer maximum number of entity packets that the client can send per second.
     */
/**
     * Get the maximum number of entity packets that the client can send per second.
     * @function Entities.getPacketsPerSecond
     * @returns {number} Integer maximum number of entity packets that the client can send per second.
     */
/**
     * Check whether servers exist for the client to send entity packets to, i.e., whether you are connected to a domain and 
     * its entity server is working.
     * @function Entities.serversExist
     * @returns {boolean} <code>true</code> if servers exist for the client to send entity packets to, otherwise 
     *     <code>false</code>.
     */
/**
     * Check whether the client has entity packets waiting to be sent.
     * @function Entities.hasPacketsToSend
     * @returns {boolean} <code>true</code> if the client has entity packets waiting to be sent, otherwise <code>false</code>.
     */
/**
     * Get the number of entity packets the client has waiting to be sent.
     * @function Entities.packetsToSendCount
     * @returns {number} Integer number of entity packets the client has waiting to be sent.
     */
/**
     * Get the entity packets per second send rate of the client over its lifetime.
     * @function Entities.getLifetimePPS
     * @returns {number} Entity packets per second send rate of the client over its lifetime.
     */
/**
     * Get the entity bytes per second send rate of the client over its lifetime.
     * @function Entities.getLifetimeBPS
     * @returns {number} Entity bytes per second send rate of the client over its lifetime.
     */
/**
     * Get the entity packets per second queued rate of the client over its lifetime.
     * @function Entities.getLifetimePPSQueued
     * @returns {number} Entity packets per second queued rate of the client over its lifetime.
     */
/**
     * Get the entity bytes per second queued rate of the client over its lifetime.
     * @function Entities.getLifetimeBPSQueued
     * @returns {number} Entity bytes per second queued rate of the client over its lifetime.
     */
/**
     * Get the lifetime of the client from the first entity packet sent until now, in microseconds.
     * @function Entities.getLifetimeInUsecs
     * @returns {number} Lifetime of the client from the first entity packet sent until now, in microseconds.
     */
/**
     * Get the lifetime of the client from the first entity packet sent until now, in seconds.
     * @function Entities.getLifetimeInSeconds
     * @returns {number} Lifetime of the client from the first entity packet sent until now, in seconds.
     */
/**
     * Get the total number of entity packets sent by the client over its lifetime.
     * @function Entities.getLifetimePacketsSent
     * @returns {number} The total number of entity packets sent by the client over its lifetime.
     */
/**
     * Get the total bytes of entity packets sent by the client over its lifetime.
     * @function Entities.getLifetimeBytesSent
     * @returns {number} The total bytes of entity packets sent by the client over its lifetime.
     */
/**
     * Get the total number of entity packets queued by the client over its lifetime.
     * @function Entities.getLifetimePacketsQueued
     * @returns {number} The total number of entity packets queued by the client over its lifetime.
     */
/**
     * Get the total bytes of entity packets queued by the client over its lifetime.
     * @function Entities.getLifetimeBytesQueued
     * @returns {number} The total bytes of entity packets queued by the client over its lifetime.
     */
/**
 * The location API provides facilities related to your current location in the metaverse.
 *
 * @namespace location
 * @property {Uuid} domainID - A UUID uniquely identifying the domain you're visiting. Is {@link Uuid|Uuid.NULL} if you're not
 *     connected to the domain.
 *     <em>Read-only.</em>
 * @property {Uuid} domainId - Synonym for <code>domainId</code>. <em>Read-only.</em> <strong>Deprecated:</strong> This property
 *     is deprecated and will soon be removed.
 * @property {string} hostname - The name of the domain for your current metaverse address (e.g., <code>"AvatarIsland"</code>,
 *     <code>localhost</code>, or an IP address).
 *     <em>Read-only.</em>
 * @property {string} href - Your current metaverse address (e.g., <code>"hifi://avatarisland/15,-10,26/0,0,0,1"</code>)
 *     regardless of whether or not you're connected to the domain.
 *     <em>Read-only.</em>
 * @property {boolean} isConnected - <code>true</code> if you're connected to the domain in your current <code>href</code>
 *     metaverse address, otherwise <code>false</code>.
 *     <em>Read-only.</em>
 * @property {string} pathname - The location and orientation in your current <code>href</code> metaverse address 
 *     (e.g., <code>"/15,-10,26/0,0,0,1"</code>).
 *     <em>Read-only.</em>
 * @property {string} placename - The place name in your current <code>href</code> metaverse address
 *     (e.g., <code>"AvatarIsland"</code>). Is blank if your <code>hostname</code> is an IP address.
 *     <em>Read-only.</em>
 * @property {string} protocol - The protocol of your current <code>href</code> metaverse address (e.g., <code>"hifi"</code>).
 *     <em>Read-only.</em>
 */
/**
     * <p>The reasons for an address lookup via the metaverse API are defined by numeric values:</p>
     * <table>
     *   <thead>
     *     <tr>
     *       <th>Name</th>
     *       <th>Value</th>
     *       <th>Description</th>
     *     </tr>
     *   </thead>
     *   <tbody>
     *     <tr>
     *       <td><strong>UserInput</strong></td>
     *       <td><code>0</code></td>
     *       <td>User-typed input.</td>
     *     </tr>
     *     <tr>
     *       <td><strong>Back</strong></td>
     *       <td><code>1</code></td>
     *       <td>Address from a {@link location.goBack|goBack} call.</td>
     *     </tr>
     *     <tr>
     *       <td><strong>Forward</strong></td>
     *       <td><code>2</code></td>
     *       <td>Address from a {@link location.goForward|goForward} call.</td>
     *     </tr>
     *     <tr>
     *       <td><strong>StartupFromSettings</strong></td>
     *       <td><code>3</code></td>
     *       <td>Initial location at Interface start-up from settings.</td>
     *     </tr>
     *     <tr>
     *       <td><strong>DomainPathResponse</strong></td>
     *       <td><code>4</code></td>
     *       <td>A named path in the domain.</td>
     *     </tr>
     *     <tr>
     *       <td><strong>Internal</strong></td>
     *       <td><code>5</code></td>
     *       <td>An internal attempt to resolve an alternative path.</td>
     *     </tr>
     *     <tr>
     *       <td><strong>AttemptedRefresh</strong></td>
     *       <td><code>6</code></td>
     *       <td>A refresh after connecting to a domain.</td>
     *     </tr>
     *     <tr>
     *       <td><strong>Suggestions</strong></td>
     *       <td><code>7</code></td>
     *       <td>Address from the Goto dialog.</td>
     *     </tr>
     *     <tr>
     *       <td><strong>VisitUserFromPAL</strong></td>
     *       <td><code>8</code></td>
     *       <td>User from the People dialog.</td>
     *     </tr>
     *   </tbody>
     * </table>
     * @typedef location.LookupTrigger
     */
/**
     * Go to a specified metaverse address.
     * @function location.handleLookupString
     * @param {string} address - The address to go to: a <code>"hifi:/"<code> address, an IP address (e.g., 
     * <code>"127.0.0.1"</code> or <code>"localhost"</code>), a domain name, a named path on a domain (starts with 
     * <code>"/"</code>), a position or position and orientation, or a user (starts with <code>"@"</code>).
     * @param {boolean} fromSuggestions=false - Set to <code>true</code> if the address is obtained from the "Goto" dialog.
     *    Helps ensure that user's location history is correctly maintained.
     */
/**
     * Go to a position and orientation resulting from a lookup for a named path in the domain (set in the domain server's 
     * settings).
     * @function location.goToViewpointForPath
     * @param {string} path - The position and orientation corresponding to the named path.
     * @param {string} namedPath - The named path that was looked up on the server.
     * @deprecated This function is deprecated and will be removed.
     */
/**
     * Go back to the previous location in your navigation history, if there is one.
     * @function location.goBack
     */
/**
     * Go forward to the next location in your navigation history, if there is one.
     * @function location.goForward
     */
/**
     * Go to the local Sandbox server that's running on the same PC as Interface.
     * @function location.goToLocalSandbox
     * @param {string} path="" - The position and orientation to go to (e.g., <code>"/0,0,0"</code>).
     * @param {location.LookupTrigger} trigger=StartupFromSettings - The reason for the function call. Helps ensure that user's
     *     location history is correctly maintained.
     */
/**
     * Go to the default "welcome" metaverse address.
     * @function location.goToEntry
     * @param {location.LookupTrigger} trigger=StartupFromSettings - The reason for the function call. Helps ensure that user's
     *     location history is correctly maintained.
     */
/**
     * Go to the specified user's location.
     * @function location.goToUser
     * @param {string} username - The user's username.
     * @param {boolean} matchOrientation=true - If <code>true</code> then go to a location just in front of the user and turn to face
     *     them, otherwise go to the user's exact location and orientation.
     */
/**
     * Refresh the current address, e.g., after connecting to a domain in order to position the user to the desired location.
     * @function location.refreshPreviousLookup
     * @deprecated This function is deprecated and will be removed.
     */
/**
     * Update your current metaverse location in Interface's {@link Settings} file as your last-known address. This can be used
     * to ensure that you start up at that address if you exit Interface without a later address automatically being saved.
     * @function location.storeCurrentAddress
     */
/**
     * Copy your current metaverse address (i.e., <code>location.href</code> property value) to the OS clipboard.
     * @function location.copyAddress
     */
/**
     * Copy your current metaverse location and orientation (i.e., <code>location.pathname</code> property value) to the OS 
     * clipboard.
     * @function location.copyPath
     */
/**
     * Retrieve and remember the place name for the given domain ID if the place name is not already known.
     * @function location.lookupShareableNameForDomainID
     * @param {Uuid} domainID - The UUID of the domain.
     * @deprecated This function is deprecated and will be removed.
     */
/**
     * Triggered when looking up the details of a metaverse user or location to go to has completed (successfully or
     * unsuccessfully).
     * @function location.lookupResultsFinished
     * @returns {Signal}
     */
/**
     * Triggered when looking up the details of a metaverse user or location to go to has completed and the domain or user is 
     * offline.
     * @function location.lookupResultIsOffline
     * @returns {Signal}
     */
/**
     * Triggered when looking up the details of a metaverse user or location to go to has completed and the domain or user could
     * not be found.
     * @function location.lookupResultIsNotFound
     * @returns {Signal}
     */
/**
     * Triggered when a request is made to go to an IP address.
     * @function location.possibleDomainChangeRequired
     * @param {Url} domainURL - URL for domain
     * @param {Uuid} domainID - The UUID of the domain to go to.
     * @returns {Signal}
     */
/**
     * Triggered when a request is made to go to a named domain or user.
     * @function location.possibleDomainChangeRequiredViaICEForID
     * @param {string} iceServerHostName - IP address of the ICE server.
     * @param {Uuid} domainID - The UUID of the domain to go to.
     * @returns {Signal}
     */
/**
     * Triggered when an attempt is made to send your avatar to a specified position on the current domain. For example, when
     * you change domains or enter a position to go to in the "Goto" dialog.
     * @function location.locationChangeRequired
     * @param {Vec3} position - The position to go to.
     * @param {boolean} hasOrientationChange - If <code>true</code> then a new <code>orientation</code> has been requested.
     * @param {Quat} orientation - The orientation to change to. Is {@link Quat(0)|Quat.IDENTITY} if 
     *     <code>hasOrientationChange</code> is <code>false</code>.
     * @param {boolean} shouldFaceLocation - If <code>true</code> then the request is to go to a position near that specified 
     *     and orient your avatar to face it. For example when you visit someone from the "People" dialog.
     * @returns {Signal}
     * @example <caption>Report location change requests.</caption>
     * function onLocationChangeRequired(newPosition, hasOrientationChange, newOrientation, shouldFaceLocation) {
     *     print("Location change required:");
     *     print("- New position = " + JSON.stringify(newPosition));
     *     print("- Has orientation change = " + hasOrientationChange);
     *     print("- New orientation = " + JSON.stringify(newOrientation));
     *     print("- Should face location = " + shouldFaceLocation);
     * }
     *
     * location.locationChangeRequired.connect(onLocationChangeRequired);
     */
/**
     * Triggered when an attempt is made to send your avatar to a new named path on the domain (set in the domain server's
     * settings). For example, when you enter a "/" followed by the path's name in the "GOTO" dialog.
     * @function location.pathChangeRequired
     * @param {string} path - The name of the path to go to.
     * @returns {Signal}
     * @example <caption>Report path change requests.</caption>
     * function onPathChangeRequired(newPath) {
     *     print("onPathChangeRequired: newPath = " + newPath);
     * }
     *
     * location.pathChangeRequired.connect(onPathChangeRequired);
     */
/**
     * Triggered when you navigate to a new domain.
     * @function location.hostChanged
     * @param {string} hostname - The new domain's host name.
     * @returns {Signal}
     * @example <caption>Report when you navigate to a new domain.</caption>
     * function onHostChanged(host) {
     *     print("Host changed to: " + host);
     * }
     *
     * location.hostChanged.connect(onHostChanged);
     */
/**
     * Triggered when there's a change in whether or not there's a previous location that can be navigated to using
     * {@link location.goBack|goBack}. (Reflects changes in the state of the "Goto" dialog's back arrow.)
     * @function location.goBackPossible
     * @param {boolean} isPossible - <code>true</code> if there's a previous location to navigate to, otherwise 
     *     <code>false</code>.
     * @returns {Signal}
     * @example <caption>Report when ability to navigate back changes.</caption>
     * function onGoBackPossible(isPossible) {
     *     print("Go back possible: " + isPossible);
     * }
     *
     * location.goBackPossible.connect(onGoBackPossible);
     */
/**
     * Triggered when there's a change in whether or not there's a forward location that can be navigated to using
     * {@link location.goForward|goForward}. (Reflects changes in the state of the "Goto" dialog's forward arrow.)
     * @function location.goForwardPossible
     * @param {boolean} isPossible - <code>true</code> if there's a forward location to navigate to, otherwise
     *     <code>false</code>.
     * @returns {Signal}
     * @example <caption>Report when ability to navigate forward changes.</caption>
     * function onGoForwardPossible(isPossible) {
     *     print("Go forward possible: " + isPossible);
     * }
     *
     * location.goForwardPossible.connect(onGoForwardPossible);
     */
/**
     * <p>The reasons that you may be refused connection to a domain are defined by numeric values:</p>
     * <table>
     *   <thead>
     *     <tr>
     *       <th>Reason</th>
     *       <th>Value</th>
     *       <th>Description</th>
     *     </tr>
     *   </thead>
     *   <tbody>
     *     <tr>
     *       <td><strong>Unknown</strong></td>
     *       <td><code>0</code></td>
     *       <td>Some unknown reason.</td>
     *     </tr>
     *     <tr>
     *       <td><strong>ProtocolMismatch</strong></td>
     *       <td><code>1</code></td>
     *       <td>The communications protocols of the domain and your Interface are not the same.</td>
     *     </tr>
     *     <tr>
     *       <td><strong>LoginError</strong></td>
     *       <td><code>2</code></td>
     *       <td>You could not be logged into the domain.</td>
     *     </tr>
     *     <tr>
     *       <td><strong>NotAuthorized</strong></td>
     *       <td><code>3</code></td>
     *       <td>You are not authorized to connect to the domain.</td>
     *     </tr>
     *     <tr>
     *       <td><strong>TooManyUsers</strong></td>
     *       <td><code>4</code></td>
     *       <td>The domain already has its maximum number of users.</td>
     *     </tr>
     *   </tbody>
     * </table>
     * @typedef Window.ConnectionRefusedReason
     */
/**
 * <p>The Messages API enables text and data to be sent between scripts over named "channels". A channel can have an arbitrary 
 * name to help separate messaging between different sets of scripts.</p>
 *
 * <p><strong>Note:</strong> If you want to call a function in another script, you should use one of the following rather than 
 * sending a message:</p>
 * <ul>
 *   <li>{@link Entities.callEntityClientMethod}</li>
 *   <li>{@link Entities.callEntityMethod}</li>
 *   <li>{@link Entities.callEntityServerMethod}</li>
 *   <li>{@link Script.callEntityScriptMethod}</li>
 * </ul>
 *
 * @namespace Messages
 */
/**
     * Send a text message on a channel.
     * @function Messages.sendMessage
     * @param {string} channel - The channel to send the message on.
     * @param {string} message - The message to send.
     * @param {boolean} [localOnly=false] - If <code>false</code> then the message is sent to all Interface, client entity, 
     *     server entity, and assignment client scripts in the domain.<br />
     *     If <code>true</code> then: if sent from an Interface or client entity script it is received by all Interface and 
     *     client entity scripts; if sent from a server entity script it is received by all entity server scripts; and if sent 
     *     from an assignment client script it is received only by that same assignment client script.
     * @example <caption>Send and receive a message.</caption>
     * // Receiving script.
     * var channelName = "com.highfidelity.example.messages-example";
     *
     * function onMessageReceived(channel, message, sender, localOnly) {
     *     print("Message received:");
     *     print("- channel: " + channel);
     *     print("- message: " + message);
     *     print("- sender: " + sender);
     *     print("- localOnly: " + localOnly);
     * }
     *
     * Messages.subscribe(channelName);
     * Messages.messageReceived.connect(onMessageReceived);
     *
     * Script.scriptEnding.connect(function () {
     *     Messages.messageReceived.disconnect(onMessageReceived);
     *     Messages.unsubscribe(channelName);
     * });
     *
     *
     * // Sending script.
     * var channelName = "com.highfidelity.example.messages-example";
     * var message = "Hello";
     * Messages.sendMessage(channelName, message);
     */
/**
     * Send a text message locally on a channel.
     * This is the same as calling {@link Messages.sendMessage|sendMessage} with <code>localOnly</code> set to 
     * <code>true</code>.
     * @function Messages.sendLocalMessage
     * @param {string} channel - The channel to send the message on.
     * @param {string} message - The message to send.
     */
/**
     * Send a data message on a channel.
     * @function Messages.sendData
     * @param {string} channel - The channel to send the data on.
     * @param {object} data - The data to send. The data is handled as a byte stream, for example as may be provided via a 
     *     JavaScript <code>Int8Array</code> object.
     * @param {boolean} [localOnly=false] - If <code>false</code> then the message is sent to all Interface, client entity,
     *     server entity, and assignment client scripts in the domain.<br />
     *     If <code>true</code> then: if sent from an Interface or client entity script it is received by all Interface and
     *     client entity scripts; if sent from a server entity script it is received by all entity server scripts; and if sent
     *     from an assignment client script it is received only by that same assignment client script.
     * @example <caption>Send and receive data.</caption>
     * // Receiving script.
     * var channelName = "com.highfidelity.example.messages-example";
     *
     * function onDataReceived(channel, data, sender, localOnly) {
     *     var int8data = new Int8Array(data);
     *     var dataAsString = "";
     *     for (var i = 0; i < int8data.length; i++) {
     *         if (i > 0) {
     *             dataAsString += ", ";
     *         }
     *         dataAsString += int8data[i];
     *     }
     *     print("Data received:");
     *     print("- channel: " + channel);
     *     print("- data: " + dataAsString);
     *     print("- sender: " + sender);
     *     print("- localOnly: " + localOnly);
     * }
     *
     * Messages.subscribe(channelName);
     * Messages.dataReceived.connect(onDataReceived);
     *
     * Script.scriptEnding.connect(function () {
     *     Messages.dataReceived.disconnect(onDataReceived);
     *     Messages.unsubscribe(channelName);
     * });
     *
     *
     * // Sending script.
     * var channelName = "com.highfidelity.example.messages-example";
     * var int8data = new Int8Array([1, 1, 2, 3, 5, 8, 13]);
     * Messages.sendData(channelName, int8data.buffer);
     */
/**
     * Subscribe the scripting environment &mdash; Interface, the entity script server, or assignment client instance &mdash; 
     * to receive messages on a specific channel. Note that, for example, if there are two Interface scripts that subscribe to 
     * different channels, both scripts will receive messages on both channels.
     * @function Messages.subscribe
     * @param {string} channel - The channel to subscribe to.
     */
/**
     * Unsubscribe the scripting environment from receiving messages on a specific channel.
     * @function Messages.unsubscribe
     * @param {string} channel - The channel to unsubscribe from.
     */
/**
     * Triggered when the a text message is received.
     * @function Messages.messageReceived
     * @param {string} channel - The channel that the message was sent on. You can use this to filter out messages not relevant 
     *     to your script.
     * @param {string} message - The message received.
     * @param {Uuid} senderID - The UUID of the sender: the user's session UUID if sent by an Interface or client entity 
     *     script, the UUID of the entity script server if sent by a server entity script, or the UUID of the assignment client 
     *     instance if sent by an assignment client script.
     * @param {boolean} localOnly - <code>true</code> if the message was sent with <code>localOnly = true</code>.
     * @returns {Signal}
     */
/**
     * Triggered when a data message is received.
     * @function Messages.dataReceived
     * @param {string} channel - The channel that the message was sent on. You can use this to filter out messages not relevant
     *     to your script.
     * @param {object} data - The data received. The data is handled as a byte stream, for example as may be used by a 
     *     JavaScript <code>Int8Array</code> object.
     * @param {Uuid} senderID - The UUID of the sender: the user's session UUID if sent by an Interface or client entity
     *     script, the UUID of the entity script server if sent by a server entity script, or the UUID of the assignment client
     *     script, the UUID of the entity script server if sent by a server entity script, or the UUID of the assignment client
     *     instance if sent by an assignment client script.
     * @param {boolean} localOnly - <code>true</code> if the message was sent with <code>localOnly = true</code>.
     * @returns {Signal}
     */
/**
     * @constructor Resource
     * @property url {string} url of this resource
     * @property state {Resource.State} current loading state
     */
/**
     * @name Resource.State
     * @static
     * @property QUEUED {int} The resource is queued up, waiting to be loaded.
     * @property LOADING {int} The resource is downloading
     * @property LOADED {int} The resource has finished downloaded by is not complete
     * @property FINISHED {int} The resource has completly finished loading and is ready.
     * @property FAILED {int} Downloading the resource has failed.
     */
/**
     * Release this resource
     * @function Resource#release
     */
/**
     * Signaled when download progress for this resource has changed
     * @function Resource#progressChanged
     * @param bytesReceived {int} bytes downloaded so far
     * @param bytesTotal {int} total number of bytes in the resource
     * @returns {Signal}
     */
/**
     * Signaled when resource loading state has changed
     * @function Resource#stateChanged
     * @param bytesReceived {Resource.State} new state
     * @returns {Signal}
     */
/**
     * @namespace ResourceCache
     * @property numTotal {number} total number of total resources
     * @property numCached {number} total number of cached resource
     * @property sizeTotal {number} size in bytes of all resources
     * @property sizeCached {number} size in bytes of all cached resources
     */
/**
     * Returns the total number of resources
     * @function ResourceCache.getNumTotalResources
     * @return {number}
     */
/**
     * Returns the total size in bytes of all resources
     * @function ResourceCache.getSizeTotalResources
     * @return {number}
     */
/**
     * Returns the total number of cached resources
     * @function ResourceCache.getNumCachedResources
     * @return {number}
     */
/**
     * Returns the total size in bytes of cached resources
     * @function ResourceCache.getSizeCachedResources
     * @return {number}
     */
/**
     * Returns list of all resource urls
     * @function ResourceCache.getResourceList
     * @return {string[]}
     */
/**
     * Asynchronously loads a resource from the spedified URL and returns it.
     * @param url {string} url of resource to load
     * @param fallback {string} fallback URL if load of the desired url fails
     * @function ResourceCache.getResource
     * @return {Resource}
     */
/**
     * Prefetches a resource.
     * @param url {string} url of resource to load
     * @function ResourceCache.prefetch
     * @return {Resource}
     */
/**
 * The <code>"offset"</code> {@link Entities.ActionType|ActionType} moves an entity so that it is a set distance away from a 
 * target point.
 * It has arguments in addition to the common {@link Entities.ActionArguments|ActionArguments}.
 *
 * @typedef {object} Entities.ActionArguments-Offset
 * @property {Vec3} pointToOffsetFrom=0,0,0 - The target point to offset the entity from.
 * @property {number} linearDistance=0 - The distance away from the target point to position the entity.
 * @property {number} linearTimeScale=34e+38 - Controls how long it takes for the entity's position to catch up with the
 *     target offset. The value is the time for the action to catch up to 1/e = 0.368 of the target value, where the action 
 *     is applied using an exponential decay.
 */
/**
 * The <code>"tractor"</code> {@link Entities.ActionType|ActionType} moves and rotates an entity to a target position and 
 * orientation, optionally relative to another entity.
 * It has arguments in addition to the common {@link Entities.ActionArguments|ActionArguments}.
 *
 * @typedef {object} Entities.ActionArguments-Tractor
 * @property {Vec3} targetPosition=0,0,0 - The target position.
 * @property {Quat} targetRotation=0,0,0,1 - The target rotation.
 * @property {Uuid} otherID=null - If an entity ID, the <code>targetPosition</code> and <code>targetRotation</code> are 
 *     relative to this entity's position and rotation.
 * @property {number} linearTimeScale=3.4e+38 - Controls how long it takes for the entity's position to catch up with the
 *     target position. The value is the time for the action to catch up to 1/e = 0.368 of the target value, where the action 
 *     is applied using an exponential decay.
 * @property {number} angularTimeScale=3.4e+38 - Controls how long it takes for the entity's orientation to catch up with the
 *     target orientation. The value is the time for the action to catch up to 1/e = 0.368 of the target value, where the 
 *     action is applied using an exponential decay.
 */
/**
 * The <code>"travel-oriented"</code> {@link Entities.ActionType|ActionType} orients an entity to align with its direction of 
 * travel.
 * It has arguments in addition to the common {@link Entities.ActionArguments|ActionArguments}.
 *
 * @typedef {object} Entities.ActionArguments-TravelOriented
 * @property {Vec3} forward=0,0,0 - The axis of the entity to align with the entity's direction of travel.
 * @property {number} angularTimeScale=0.1 - Controls how long it takes for the entity's orientation to catch up with the 
 *     direction of travel. The value is the time for the action to catch up to 1/e = 0.368 of the target value, where the 
 *     action is applied using an exponential decay.
 */
/**
 * The <code>"ball-socket"</code> {@link Entities.ActionType|ActionType} connects two entities with a ball and socket joint. 
 * It has arguments in addition to the common {@link Entities.ActionArguments|ActionArguments}.
 *
 * @typedef {object} Entities.ActionArguments-BallSocket
 * @property {Vec3} pivot=0,0,0 - The local offset of the joint relative to the entity's position.
 * @property {Uuid} otherEntityID=null - The ID of the other entity that is connected to the joint.
 * @property {Vec3} otherPivot=0,0,0 - The local offset of the joint relative to the other entity's position.
 */
/**
 * The <code>"cone-twist"</code> {@link Entities.ActionType|ActionType} connects two entities with a joint that can move 
 * through a cone and can twist.
 * It has arguments in addition to the common {@link Entities.ActionArguments|ActionArguments}.
 *
 * @typedef {object} Entities.ActionArguments-ConeTwist
 * @property {Vec3} pivot=0,0,0 - The local offset of the joint relative to the entity's position.
 * @property {Vec3} axis=1,0,0 - The axis of the entity that moves through the cone. Must be a non-zero vector.
 * @property {Uuid} otherEntityID=null - The ID of the other entity that is connected to the joint.
 * @property {Vec3} otherPivot=0,0,0 - The local offset of the joint relative to the other entity's position.
 * @property {Vec3} otherAxis=1,0,0 - The axis of the other entity that moves through the cone. Must be a non-zero vector.
 * @property {number} swingSpan1=6.238 - The angle through which the joint can move in one axis of the cone, in radians.
 * @property {number} swingSpan2=6.238 - The angle through which the joint can move in the other axis of the cone, in radians.
 * @property {number} twistSpan=6.238 - The angle through with the joint can twist, in radians.
 */
/**
 * The <code>"hinge"</code> {@link Entities.ActionType|ActionType} lets an entity pivot about an axis or connects two entities
 * with a hinge joint.
 * It has arguments in addition to the common {@link Entities.ActionArguments|ActionArguments}.
 *
 * @typedef {object} Entities.ActionArguments-Hinge
 * @property {Vec3} pivot=0,0,0 - The local offset of the joint relative to the entity's position.
 * @property {Vec3} axis=1,0,0 - The axis of the entity that it pivots about. Must be a non-zero vector.
 * @property {Uuid} otherEntityID=null - The ID of the other entity that is connected to the joint, if any. If none is 
 *     specified then the first entity simply pivots about its specified <code>axis</code>.
 * @property {Vec3} otherPivot=0,0,0 - The local offset of the joint relative to the other entity's position.
 * @property {Vec3} otherAxis=1,0,0 - The axis of the other entity that it pivots about. Must be a non-zero vector.
 * @property {number} low=-6.283 - The most negative angle that the hinge can take, in radians.
 * @property {number} high=6.283 - The most positive angle that the hinge can take, in radians.
 * @property {number} angle=0 - The current angle of the hinge. <em>Read-only.</em>
 */
/**
 * The <code>"slider"</code> {@link Entities.ActionType|ActionType} lets an entity slide and rotate along an axis, or connects 
 * two entities that slide and rotate along a shared axis.
 * It has arguments in addition to the common {@link Entities.ActionArguments|ActionArguments}.
 *
 * @typedef {object} Entities.ActionArguments-Slider
 * @property {Vec3} point=0,0,0 - The local position of a point in the entity that slides along the axis.
 * @property {Vec3} axis=1,0,0 - The axis of the entity that slides along the joint. Must be a non-zero vector.
 * @property {Uuid} otherEntityID=null - The ID of the other entity that is connected to the joint, if any. If non is 
 *     specified then the first entity simply slides and rotates about its specified <code>axis</code>.
 * @property {Vec3} otherPoint=0,0,0 - The local position of a point in the other entity that slides along the axis.
 * @property {Vec3} axis=1,0,0 - The axis of the other entity that slides along the joint. Must be a non-zero vector.
 * @property {number} linearLow=1.17e-38 - The most negative linear offset from the entity's initial point that the entity can 
 *     have along the slider.
 * @property {number} linearHigh=3.40e+38 - The most positive linear offset from the entity's initial point that the entity can 
 *     have along the slider. 
 * @property {number} angularLow=-6.283 - The most negative angle that the entity can rotate about the axis if the action 
 *     involves only one entity, otherwise the most negative angle the rotation can be between the two entities. In radians.
 * @property {number} angularHigh=6.283 - The most positive angle that the entity can rotate about the axis if the action 
 *     involves only one entity, otherwise the most positive angle the rotation can be between the two entities. In radians.
 * @property {number} linearPosition=0 - The current linear offset the entity is from its initial point if the action involves 
 *     only one entity, otherwise the linear offset between the two entities' action points. <em>Read-only.</em>
 * @property {number} angularPosition=0 - The current angular offset of the entity from its initial rotation if the action 
 *     involves only one entity, otherwise the angular offset between the two entities. <em>Read-only.</em>
 */
/**
* Different entity action types have different arguments: some common to all actions (listed below) and some specific to each 
* {@link Entities.ActionType|ActionType} (linked to below). The arguments are accessed as an object of property names and 
* values.
*
* @typedef {object} Entities.ActionArguments
* @property {Entities.ActionType} type - The type of action.
* @property {string} tag="" - A string that a script can use for its own purposes.
* @property {number} ttl=0 - How long the action should exist, in seconds, before it is automatically deleted. A value of 
*     <code>0</code> means that the action should not be deleted.
* @property {boolean} isMine=true - Is <code>true</code> if you created the action during your current Interface session, 
*     <code>false</code> otherwise. <em>Read-only.</em>
* @property {boolean} ::no-motion-state - Is present when the entity hasn't been registered with the physics engine yet (e.g., 
*     if the action hasn't been properly configured), otherwise <code>undefined</code>. <em>Read-only.</em>
* @property {boolean} ::active - Is <code>true</code> when the action is modifying the entity's motion, <code>false</code> 
*     otherwise. Is present once the entity has been registered with the physics engine, otherwise <code>undefined</code>. 
*     <em>Read-only.</em>
* @property {Entities.PhysicsMotionType} ::motion-type - How the entity moves with the action. Is present once the entity has 
*     been registered with the physics engine, otherwise <code>undefined</code>. <em>Read-only.</em>
*
* @see The different action types have additional arguments as follows:
* @see {@link Entities.ActionArguments-FarGrab|ActionArguments-FarGrab}
* @see {@link Entities.ActionArguments-Hold|ActionArguments-Hold}
* @see {@link Entities.ActionArguments-Offset|ActionArguments-Offset}
* @see {@link Entities.ActionArguments-Tractor|ActionArguments-Tractor}
* @see {@link Entities.ActionArguments-TravelOriented|ActionArguments-TravelOriented}
* @see {@link Entities.ActionArguments-Hinge|ActionArguments-Hinge}
* @see {@link Entities.ActionArguments-Slider|ActionArguments-Slider}
* @see {@link Entities.ActionArguments-ConeTwist|ActionArguments-ConeTwist}
* @see {@link Entities.ActionArguments-BallSocket|ActionArguments-BallSocket}
*/
/**
 * <p>An entity's physics motion type may be one of the following:</p>
 * <table>
 *   <thead>
 *     <tr><th>Value</th><th>Description</th></tr>
 *   </thead>
 *   <tbody>
 *     <tr><td><code>"static"</code></td><td>There is no motion because the entity is locked  &mdash; its <code>locked</code> 
 *         property is set to <code>true</code>.</td></tr>
 *     <tr><td><code>"kinematic"</code></td><td>Motion is applied without physical laws (e.g., damping) because the entity is 
 *         not locked and has its <code>dynamic</code> property set to <code>false</code>.</td></tr>
 *     <tr><td><code>"dynamic"</code></td><td>Motion is applied according to physical laws (e.g., damping) because the entity 
 *         is not locked and has its <code>dynamic</code> property set to <code>true</code>.</td></tr>
 *   </tbody>
 * </table>
 * @typedef {string} Entities.PhysicsMotionType
 */
/**
     * @namespace
     * @augments Picks
     *
     * Enum for different types of Picks and Pointers.
     *
     * @typedef {enum} Picks.PickType
     * @property {number} Ray Ray Picks intersect a ray with the nearest object in front of them, along a given direction.
     * @property {number} Stylus Stylus Picks provide "tapping" functionality on/into flat surfaces.
     */
/**
 * @typedef {string} Assets.GetOptions.ResponseType
 * <p>Available <code>responseType</code> values for use with @{link Assets.getAsset} and @{link Assets.loadFromCache} configuration option. </p>
 * <table>
 *   <thead>
 *     <tr><th>responseType</th><th>typeof response value</th></tr>
 *   </thead>
 *   <tbody>
 *     <tr><td><code>"text"</code></td><td>contents returned as utf-8 decoded <code>String</code> value</td></tr>
 *     <tr><td><code>"arraybuffer"</code></td><td>contents as a binary <code>ArrayBuffer</code> object</td></tr>
 *     <tr><td><code>"json"</code></td><td>contents as a parsed <code>JSON</code> object</td></tr>
 *   </tbody>
 * </table>
 */
/**
 * @namespace Assets
 */
/**
     * Upload content to the connected domain's asset server.
     * @function Assets.uploadData
     * @static
     * @param data {string} content to upload
     * @param callback {Assets~uploadDataCallback} called when upload is complete
     */
/**
     * Called when uploadData is complete
     * @callback Assets~uploadDataCallback
     * @param {string} url
     * @param {string} hash
     */
/**
     * Download data from the connected domain's asset server.
     * @function Assets.downloadData
     * @static
     * @param url {string} url of asset to download, must be atp scheme url.
     * @param callback {Assets~downloadDataCallback}
     */
/**
     * Called when downloadData is complete
     * @callback Assets~downloadDataCallback
     * @param data {string} content that was downloaded
     */
/**
     * Sets up a path to hash mapping within the connected domain's asset server
     * @function Assets.setMapping
     * @static
     * @param path {string}
     * @param hash {string}
     * @param callback {Assets~setMappingCallback}
     */
/**
     * Called when setMapping is complete
     * @callback Assets~setMappingCallback
     * @param {string} error
     */
/**
     * Look up a path to hash mapping within the connected domain's asset server
     * @function Assets.getMapping
     * @static
     * @param path {string}
     * @param callback {Assets~getMappingCallback}
     */
/**
     * Called when getMapping is complete.
     * @callback Assets~getMappingCallback
     * @param error {string} error description if the path could not be resolved; otherwise a null value.
     * @param assetID {string} hash value if found, else an empty string
     */
/**
     * Request Asset data from the ATP Server
     * @function Assets.getAsset
     * @param {URL|Assets.GetOptions} options An atp: style URL, hash, or relative mapped path; or an {@link Assets.GetOptions} object with request parameters
     * @param {Assets~getAssetCallback} scope[callback] A scope callback function to receive (error, results) values
     */
/**
    * A set of properties that can be passed to {@link Assets.getAsset}.
    * @typedef {Object} Assets.GetOptions
    * @property {URL} [url] an "atp:" style URL, hash, or relative mapped path to fetch
    * @property {string} [responseType=text] the desired reponse type (text | arraybuffer | json)
    * @property {boolean} [decompress=false] whether to attempt gunzip decompression on the fetched data
    *    See: {@link Assets.putAsset} and its .compress=true option
    */
/**
     * Called when Assets.getAsset is complete.
     * @callback Assets~getAssetCallback
     * @param {string} error - contains error message or null value if no error occured fetching the asset
     * @param {Asset~getAssetResult} result - result object containing, on success containing asset metadata and contents
     */
/**
    * Result value returned by {@link Assets.getAsset}.
    * @typedef {Object} Assets~getAssetResult
    * @property {url} [url] the resolved "atp:" style URL for the fetched asset
    * @property {string} [hash] the resolved hash for the fetched asset
    * @property {string|ArrayBuffer|Object} [response] response data (possibly converted per .responseType value)
    * @property {string} [responseType] response type (text | arraybuffer | json)
    * @property {string} [contentType] detected asset mime-type (autodetected)
    * @property {number} [byteLength] response data size in bytes
    * @property {number} [decompressed] flag indicating whether data was decompressed
    */
/**
     * Upload Asset data to the ATP Server
     * @function Assets.putAsset
     * @param {Assets.PutOptions} options A PutOptions object with upload parameters
     * @param {Assets~putAssetCallback} scope[callback] A scoped callback function invoked with (error, results)
     */
/**
    * A set of properties that can be passed to {@link Assets.putAsset}.
    * @typedef {Object} Assets.PutOptions
    * @property {ArrayBuffer|string} [data] byte buffer or string value representing the new asset's content
    * @property {string} [path=null] ATP path mapping to automatically create (upon successful upload to hash)
    * @property {boolean} [compress=false] whether to gzip compress data before uploading
    */
/**
     * Called when Assets.putAsset is complete.
     * @callback Assets~puttAssetCallback
     * @param {string} error - contains error message (or null value if no error occured while uploading/mapping the new asset)
     * @param {Asset~putAssetResult} result - result object containing error or result status of asset upload
     */
/**
    * Result value returned by {@link Assets.putAsset}.
    * @typedef {Object} Assets~putAssetResult
    * @property {url} [url] the resolved "atp:" style URL for the uploaded asset (based on .path if specified, otherwise on the resulting ATP hash)
    * @property {string} [path] the uploaded asset's resulting ATP path (or undefined if no path mapping was assigned)
    * @property {string} [hash] the uploaded asset's resulting ATP hash
    * @property {boolean} [compressed] flag indicating whether the data was compressed before upload
    * @property {number} [byteLength] flag indicating final byte size of the data uploaded to the ATP server
    */
/**
 * A set of properties that can be passed to {@link Menu.addMenuItem} to create a new menu item.
 *
 * If none of <code>position</code>, <code>beforeItem</code>, <code>afterItem</code>, or <code>grouping</code> are specified, 
 * the menu item will be placed at the end of the menu.
 *
 * @typedef {object} Menu.MenuItemProperties
 * @property {string} menuName Name of the menu. Nested menus can be described using the ">" symbol.
 * @property {string} menuItemName Name of the menu item.
 * @property {boolean} [isCheckable=false] Whether or not the menu item is checkable.
 * @property {boolean} [isChecked=false] Whether or not the menu item is checked.
 * @property {boolean} [isSeparator=false] Whether or not the menu item is a separator.
 * @property {string} [shortcutKey] A shortcut key that triggers the menu item.
 * @property {KeyEvent} [shortcutKeyEvent] A {@link KeyEvent} that specifies a key that triggers the menu item.
 * @property {number} [position] The position to place the new menu item. An integer number with <code>0</code> being the first
 *     menu item.
 * @property {string} [beforeItem] The name of the menu item to place this menu item before.
 * @property {string} [afterItem] The name of the menu item to place this menu item after.
 * @property {string} [grouping] The name of grouping to add this menu item to.
 */
/**
 * A quaternion value. See also the {@link Quat(0)|Quat} object.
 * @typedef {object} Quat
 * @property {number} x - Imaginary component i.
 * @property {number} y - Imaginary component j.
 * @property {number} z - Imaginary component k.
 * @property {number} w - Real component.
 */
/**
 * The Quat API provides facilities for generating and manipulating quaternions.
 * Quaternions should be used in preference to Euler angles wherever possible because quaternions don't suffer from the problem
 * of gimbal lock.
 * @namespace Quat
 * @variation 0
 * @property IDENTITY {Quat} The identity rotation, i.e., no rotation. Its value is <code>{ x: 0, y: 0, z: 0, w: 1 }</code>.
 * @example <caption>Print the <code>IDENTITY</code> value.</caption>
 * print(JSON.stringify(Quat.IDENTITY)); // { x: 0, y: 0, z: 0, w: 1 }
 * print(JSON.stringify(Quat.safeEulerAngles(Quat.IDENTITY))); // { x: 0, y: 0, z: 0 }
 */
/**
     * Multiply two quaternions.
     * @function Quat(0).multiply
     * @param {Quat} q1 - The first quaternion.
     * @param {Quat} q2 - The second quaternion.
     * @returns {Quat} <code>q1</code> multiplied with <code>q2</code>.
     * @example <caption>Calculate the orientation of your avatar's right hand in world coordinates.</caption>
     * var handController = Controller.Standard.RightHand;
     * var handPose = Controller.getPoseValue(handController);
     * if (handPose.valid) {
     *     var handOrientation = Quat.multiply(MyAvatar.orientation, handPose.rotation);
     * }
     */
/**
     * Normalizes a quaternion.
     * @function Quat(0).normalize
     * @param {Quat} q - The quaternion to normalize.
     * @returns {Quat} <code>q</code> normalized to have unit length.
     * @example <caption>Normalize a repeated delta rotation so that maths rounding errors don't accumulate.</caption>
     * var deltaRotation = Quat.fromPitchYawRollDegrees(0, 0.1, 0);
     * var currentRotation = Quat.ZERO;
     * while (Quat.safeEulerAngles(currentRotation).y < 180) {
     *     currentRotation = Quat.multiply(deltaRotation, currentRotation);
     *     currentRotation = Quat.normalize(currentRotation);
     *     // Use currentRotatation for something.
     * }
     */
/**
    * Calculate the conjugate of a quaternion. For a unit quaternion, its conjugate is the same as its 
    *     {@link Quat(0).inverse|Quat.inverse}.
    * @function Quat(0).conjugate
    * @param {Quat} q - The quaternion to conjugate.
    * @returns {Quat} The conjugate of <code>q</code>.
    * @example <caption>A unit quaternion multiplied by its conjugate is a zero rotation.</caption>
    * var quaternion = Quat.fromPitchYawRollDegrees(10, 20, 30);
    * Quat.print("quaternion", quaternion, true); // dvec3(10.000000, 20.000004, 30.000004)
    * var conjugate = Quat.conjugate(quaternion);
    * Quat.print("conjugate", conjugate, true); // dvec3(1.116056, -22.242186, -28.451778)
    * var identity = Quat.multiply(conjugate, quaternion);
    * Quat.print("identity", identity, true); // dvec3(0.000000, 0.000000, 0.000000)
    */
/**
     * Calculate a camera orientation given eye position, point of interest, and "up" direction. The camera's negative z-axis is
     * the forward direction. The result has zero roll about its forward direction with respect to the given "up" direction.
     * @function Quat(0).lookAt
     * @param {Vec3} eye - The eye position.
     * @param {Vec3} target - The point to look at.
     * @param {Vec3} up - The "up" direction.
     * @returns {Quat} A quaternion that orients the negative z-axis to point along the eye-to-target vector and the x-axis to
     * be the cross product of the eye-to-target and up vectors.
     * @example <caption>Rotate your view in independent mode to look at the world origin upside down.</caption>
     * Camera.mode = "independent";
     * Camera.orientation = Quat.lookAt(Camera.position, Vec3.ZERO, Vec3.UNIT_NEG_Y);
     */
/**
    * Calculate a camera orientation given eye position and point of interest. The camera's negative z-axis is the forward 
    * direction. The result has zero roll about its forward direction.
    * @function Quat(0).lookAtSimple
    * @param {Vec3} eye - The eye position.
    * @param {Vec3} target - The point to look at.
    * @returns {Quat} A quaternion that orients the negative z-axis to point along the eye-to-target vector and the x-axis to be
    *     the cross product of the eye-to-target and an "up" vector. The "up" vector is the y-axis unless the eye-to-target
    *     vector is nearly aligned with it (i.e., looking near vertically up or down), in which case the x-axis is used as the
    *     "up" vector.
    * @example <caption>Rotate your view in independent mode to look at the world origin.</caption>
    * Camera.mode = "independent";
    * Camera.orientation = Quat.lookAtSimple(Camera.position, Vec3.ZERO);
    */
/**
    * Calculate the shortest rotation from a first vector onto a second.
    * @function Quat(0).rotationBetween
    * @param {Vec3} v1 - The first vector.
    * @param {Vec3} v2 - The second vector.
    * @returns {Quat} The rotation from <code>v1</code> onto <code>v2</code>.
    * @example <caption>Apply a change in velocity to an entity and rotate it to face the direction it's travelling.</caption>
    * var newVelocity = Vec3.sum(entityVelocity, deltaVelocity);
    * var properties = { velocity: newVelocity };
    * if (Vec3.length(newVelocity) > 0.001) {
    *     properties.rotation = Quat.rotationBetween(entityVelocity, newVelocity);
    * }
    * Entities.editEntity(entityID, properties);
    * entityVelocity = newVelocity;
    */
/**
     * Generate a quaternion from a {@link Vec3} of Euler angles in degrees.
     * @function Quat(0).fromVec3Degrees
     * @param {Vec3} vector - A vector of three Euler angles in degrees, the angles being the rotations about the x, y, and z
     *     axes.
     * @returns {Quat} A quaternion created from the Euler angles in <code>vector</code>.
     * @example <caption>Zero out pitch and roll from an orientation.</caption>
     * var eulerAngles = Quat.safeEulerAngles(orientation);
     * eulerAngles.x = 0;
     * eulerAngles.z = 0;
     * var newOrientation = Quat.fromVec3Degrees(eulerAngles);
     */
/**
     * Generate a quaternion from a {@link Vec3} of Euler angles in radians.
     * @function Quat(0).fromVec3Radians
     * @param {Vec3} vector - A vector of three Euler angles in radians, the angles being the rotations about the x, y, and z
     *     axes.
     * @returns {Quat} A quaternion created using the Euler angles in <code>vector</code>.
     * @example <caption>Create a rotation of 180 degrees about the y axis.</caption>
     * var rotation = Quat.fromVec3Radians({ x: 0, y: Math.PI, z: 0 });
     */
/**
    * Generate a quaternion from pitch, yaw, and roll values in degrees.
    * @function Quat(0).fromPitchYawRollDegrees
    * @param {number} pitch - The pitch angle in degrees.
    * @param {number} yaw - The yaw angle in degrees.
    * @param {number} roll - The roll angle in degrees.
    * @returns {Quat} A quaternion created using the <code>pitch</code>, <code>yaw</code>, and <code>roll</code> Euler angles.
    * @example <caption>Create a rotation of 180 degrees about the y axis.</caption>
    * var rotation = Quat.fromPitchYawRollDgrees(0, 180, 0 );
    */
/**
    * Generate a quaternion from pitch, yaw, and roll values in radians.
    * @function Quat(0).fromPitchYawRollRadians
    * @param {number} pitch - The pitch angle in radians.
    * @param {number} yaw - The yaw angle in radians.
    * @param {number} roll - The roll angle in radians.
    * @returns {Quat} A quaternion created from the <code>pitch</code>, <code>yaw</code>, and <code>roll</code> Euler angles.
    * @example <caption>Create a rotation of 180 degrees about the y axis.</caption>
    * var rotation = Quat.fromPitchYawRollRadians(0, Math.PI, 0);
    */
/**
     * Calculate the inverse of a quaternion. For a unit quaternion, its inverse is the same as its
     *     {@link Quat(0).conjugate|Quat.conjugate}.
     * @function Quat(0).inverse
     * @param {Quat} q - The quaternion.
     * @returns {Quat} The inverse of <code>q</code>.
     * @example <caption>A quaternion multiplied by its inverse is a zero rotation.</caption>
     * var quaternion = Quat.fromPitchYawRollDegrees(10, 20, 30);
     * Quat.print("quaternion", quaternion, true); // dvec3(10.000000, 20.000004, 30.000004)
     * var inverse = Quat.invserse(quaternion);
     * Quat.print("inverse", inverse, true); // dvec3(1.116056, -22.242186, -28.451778)
     * var identity = Quat.multiply(inverse, quaternion);
     * Quat.print("identity", identity, true); // dvec3(0.000000, 0.000000, 0.000000)
     */
/**
     * Get the "front" direction that the camera would face if its orientation was set to the quaternion value.
     * This is a synonym for {@link Quat(0).getForward|Quat.getForward}.
     * The High Fidelity camera has axes x = right, y = up, -z = forward.
     * @function Quat(0).getFront
     * @param {Quat} orientation - A quaternion representing an orientation.
     * @returns {Vec3} The negative z-axis rotated by <code>orientation</code>.
     */
/**
     * Get the "forward" direction that the camera would face if its orientation was set to the quaternion value.
     * This is a synonym for {@link Quat(0).getFront|Quat.getFront}.
     * The High Fidelity camera has axes x = right, y = up, -z = forward.
     * @function Quat(0).getForward
     * @param {Quat} orientation - A quaternion representing an orientation.
     * @returns {Vec3} The negative z-axis rotated by <code>orientation</code>.
     * @example <caption>Demonstrate that the "forward" vector is for the negative z-axis.</caption>
     * var forward = Quat.getForward(Quat.IDENTITY);
     * print(JSON.stringify(forward)); // {"x":0,"y":0,"z":-1}
     */
/**
     * Get the "right" direction that the camera would have if its orientation was set to the quaternion value.
     * The High Fidelity camera has axes x = right, y = up, -z = forward.
     * @function Quat(0).getRight
     * @param {Quat} orientation - A quaternion representing an orientation.
     * @returns {Vec3} The x-axis rotated by <code>orientation</code>.
     */
/**
     * Get the "up" direction that the camera would have if its orientation was set to the quaternion value.
     * The High Fidelity camera has axes x = right, y = up, -z = forward.
     * @function Quat(0).getUp
     * @param {Quat} orientation - A quaternion representing an orientation.
     * @returns {Vec3} The y-axis rotated by <code>orientation</code>.
     */
/**
     * Calculate the Euler angles for the quaternion, in degrees. (The "safe" in the name signifies that the angle results will
     * not be garbage even when the rotation is particularly difficult to decompose with pitches around +/-90 degrees.)
     * @function Quat(0).safeEulerAngles
     * @param {Quat} orientation - A quaternion representing an orientation.
     * @returns {Vec3} A {@link Vec3} of Euler angles for the <code>orientation</code>, in degrees, the angles being the 
     * rotations about the x, y, and z axes.
     * @example <caption>Report the camera yaw.</caption>
     * var eulerAngles = Quat.safeEulerAngles(Camera.orientation);
     * print("Camera yaw: " + eulerAngles.y);
     */
/**
     * Generate a quaternion given an angle to rotate through and an axis to rotate about.
     * @function Quat(0).angleAxis
     * @param {number} angle - The angle to rotate through, in degrees.
     * @param {Vec3} axis - The unit axis to rotate about.
     * @returns {Quat} A quaternion that is a rotation through <code>angle</code> degrees about the <code>axis</code>. 
     * <strong>WARNING:</strong> This value is in degrees whereas the value returned by {@link Quat(0).angle|Quat.angle} is
     * in radians.
     * @example <caption>Calculate a rotation of 90 degrees about the direction your camera is looking.</caption>
     * var rotation = Quat.angleAxis(90, Quat.getForward(Camera.orientation));
     */
/**
     * Get the rotation axis for a quaternion.
     * @function Quat(0).axis
     * @param {Quat} q - The quaternion.
     * @returns {Vec3} The normalized rotation axis for <code>q</code>.
     * @example <caption>Get the rotation axis of a quaternion.</caption>
     * var forward = Quat.getForward(Camera.orientation);
     * var rotation = Quat.angleAxis(90, forward);
     * var axis = Quat.axis(rotation);
     * print("Forward: " + JSON.stringify(forward));
     * print("Axis: " + JSON.stringify(axis)); // Same value as forward.
     */
/**
     * Get the rotation angle for a quaternion.
     * @function Quat(0).angle
     * @param {Quat} q - The quaternion.
     * @returns {number} The rotation angle for <code>q</code>, in radians. <strong>WARNING:</strong> This value is in radians 
     * whereas the value used by {@link Quat(0).angleAxis|Quat.angleAxis} is in degrees.
     * @example <caption>Get the rotation angle of a quaternion.</caption>
     * var forward = Quat.getForward(Camera.orientation);
     * var rotation = Quat.angleAxis(90, forward);
     * var angle = Quat.angle(rotation);
     * print("Angle: " + angle * 180 / Math.PI);  // 90 degrees.
     */
/**
     * Compute a spherical linear interpolation between two rotations, safely handling two rotations that are very similar.
     * See also, {@link Quat(0).slerp|Quat.slerp}.
     * @function Quat(0).mix
     * @param {Quat} q1 - The beginning rotation.
     * @param {Quat} q2 - The ending rotation.
     * @param {number} alpha - The mixture coefficient between <code>0.0</code> and <code>1.0</code>. Specifies the proportion
     *     of <code>q2</code>'s value to return in favor of <code>q1</code>'s value. A value of <code>0.0</code> returns 
     *     <code>q1</code>'s value; <code>1.0</code> returns <code>q2s</code>'s value.
     * @returns {Quat} A spherical linear interpolation between rotations <code>q1</code> and <code>q2</code>.
     * @example <caption>Animate between one rotation and another.</caption>
     * var dt = amountOfTimeThatHasPassed;
     * var mixFactor = amountOfTimeThatHasPassed / TIME_TO_COMPLETE;
     * if (mixFactor > 1) {
     *     mixFactor = 1;
     * }
     * var newRotation = Quat.mix(startRotation, endRotation, mixFactor);
     */
/**
     * Compute a spherical linear interpolation between two rotations, for rotations that are not very similar.
     * See also, {@link Quat(0).mix|Quat.mix}.
     * @function Quat(0).slerp
     * @param {Quat} q1 - The beginning rotation.
     * @param {Quat} q2 - The ending rotation.
     * @param {number} alpha - The mixture coefficient between <code>0.0</code> and <code>1.0</code>. Specifies the proportion
     *     of <code>q2</code>'s value to return in favor of <code>q1</code>'s value. A value of <code>0.0</code> returns
     *     <code>q1</code>'s value; <code>1.0</code> returns <code>q2s</code>'s value.
     * @returns {Quat} A spherical linear interpolation between rotations <code>q1</code> and <code>q2</code>.
     */
/**
     * Compute a spherical quadrangle interpolation between two rotations along a path oriented toward two other rotations.
     * Equivalent to: <code>Quat.slerp(Quat.slerp(q1, q2, alpha), Quat.slerp(s1, s2, alpha), 2 * alpha * (1.0 - alpha))</code>.
     * @function Quat(0).squad
     * @param {Quat} q1 - Initial rotation.
     * @param {Quat} q2 - Final rotation.
     * @param {Quat} s1 - First control point.
     * @param {Quat} s2 - Second control point.
     * @param {number} alpha - The mixture coefficient between <code>0.0</code> and <code>1.0</code>. A value of 
     *     <code>0.0</code> returns <code>q1</code>'s value; <code>1.0</code> returns <code>q2s</code>'s value.
     * @returns {Quat} A spherical quadrangle interpolation between rotations <code>q1</code> and <code>q2</code> using control
     *     points <code>s1</code> and <code>s2</code>.
     */
/**
     * Calculate the dot product of two quaternions. The closer the quaternions are to each other the more non-zero the value is
     * (either positive or negative). Identical unit rotations have a dot product of +/- 1.
     * @function Quat(0).dot
     * @param {Quat} q1 - The first quaternion.
     * @param {Quat} q2 - The second quaternion.
     * @returns {number} The dot product of <code>q1</code> and <code>q2</code>.
     * @example <caption>Testing unit quaternions for equality.</caption>
     * var q1 = Quat.fromPitchYawRollDegrees(0, 0, 0);
     * var q2 = Quat.fromPitchYawRollDegrees(0, 0, 0);
     * print(Quat.equal(q1, q2)); // true
     * var q3 = Quat.fromPitchYawRollDegrees(0, 0, 359.95);
     * print(Quat.equal(q1, q3)); // false
     *
     * var dot = Quat.dot(q1, q3);
     * print(dot); // -0.9999999403953552
     * var equal = Math.abs(1 - Math.abs(dot)) < 0.000001;
     * print(equal); // true
     */
/**
     * Print to the program log a text label followed by a quaternion's pitch, yaw, and roll Euler angles.
     * @function Quat(0).print
     * @param {string} label - The label to print.
     * @param {Quat} q - The quaternion to print.
     * @param {boolean} [asDegrees=false] - If <code>true</code> the angle values are printed in degrees, otherwise they are
     *     printed in radians.
     * @example <caption>Two ways of printing a label plus a quaternion's Euler angles.</caption>
     * var quaternion = Quat.fromPitchYawRollDegrees(0, 45, 0);
     *
     * // Quaternion: dvec3(0.000000, 45.000004, 0.000000)
     * Quat.print("Quaternion:", quaternion,  true);
     *
     * // Quaternion: {"x":0,"y":45.000003814697266,"z":0}
     * print("Quaternion: " + JSON.stringify(Quat.safeEulerAngles(quaternion)));
     */
/**
     * Test whether two quaternions are equal. <strong>Note:</strong> The quaternions must be exactly equal in order for 
     * <code>true</code> to be returned; it is often better to use {@link Quat(0).dot|Quat.dot} and test for closeness to +/-1.
     * @function Quat(0).equal
     * @param {Quat} q1 - The first quaternion.
     * @param {Quat} q2 - The second quaternion.
     * @returns {boolean} <code>true</code> if the quaternions are equal, otherwise <code>false</code>.
     * @example <caption>Testing unit quaternions for equality.</caption>
     * var q1 = Quat.fromPitchYawRollDegrees(0, 0, 0);
     * var q2 = Quat.fromPitchYawRollDegrees(0, 0, 0);
     * print(Quat.equal(q1, q2)); // true
     * var q3 = Quat.fromPitchYawRollDegrees(0, 0, 359.95);
     * print(Quat.equal(q1, q3)); // false
     *
     * var dot = Quat.dot(q1, q3);
     * print(dot); // -0.9999999403953552
     * var equal = Math.abs(1 - Math.abs(dot)) < 0.000001;
     * print(equal); // true
     */
/**
     * Cancels out the roll and pitch component of a quaternion so that its completely horizontal with a yaw pointing in the 
     * given quaternion's direction.
     * @function Quat(0).cancelOutRollAndPitch
     * @param {Quat} orientation - A quaternion representing an orientation.
     * @returns {Quat}  <code>orientation</code> with its roll and pitch canceled out.
     * @example <caption>Two ways of calculating a camera orientation in the x-z plane with a yaw pointing in the direction of
     *     a given quaternion.</caption>
     * var quaternion = Quat.fromPitchYawRollDegrees(10, 20, 30);
     *
     * var noRollOrPitch = Quat.cancelOutRollAndPitch(quaternion);
     * Quat.print("", noRollOrPitch, true); // dvec3(0.000000, 22.245995, 0.000000)
     *
     * var front = Quat.getFront(quaternion);
     * var lookAt = Quat.lookAtSimple(Vec3.ZERO, { x: front.x, y: 0, z: front.z });
     * Quat.print("", lookAt, true); // dvec3(0.000000, 22.245996, 0.000000)
     *
     */
/**
    * Cancels out the roll component of a quaternion so that its horizontal axis is level.
    * @function Quat(0).cancelOutRoll
    * @param {Quat} orientation - A quaternion representing an orientation.
    * @returns {Quat} <code>orientation</code> with its roll canceled out.
    * @example <caption>Two ways of calculating a camera orientation that points in the direction of a given quaternion but
    *     keeps the camera's horizontal axis level.</caption>
    * var quaternion = Quat.fromPitchYawRollDegrees(10, 20, 30);
    *
    * var noRoll = Quat.cancelOutRoll(quaternion);
    * Quat.print("", noRoll, true); // dvec3(-1.033004, 22.245996, -0.000000)
    *
    * var front = Quat.getFront(quaternion);
    * var lookAt = Quat.lookAtSimple(Vec3.ZERO, front);
    * Quat.print("", lookAt, true); // dvec3(-1.033004, 22.245996, -0.000000)
    */
/**
 * A UUID (Universally Unique IDentifier) is used to uniquely identify entities, overlays, avatars, and the like. It is
 * represented in JavaScript as a string in the format, <code>{nnnnnnnn-nnnn-nnnn-nnnn-nnnnnnnnnnnn}</code>, where the "n"s are
 * hexadecimal digits.
 *
 * @namespace Uuid
 * @property NULL {Uuid} The null UUID, <code>{00000000-0000-0000-0000-000000000000}</code>.
 */
/**
     * Generates a UUID from a string representation of the UUID.
     * @function Uuid.fromString
     * @param {string} string - A string representation of a UUID. The curly braces are optional.
     * @returns {Uuid} A UUID if the given <code>string</code> is valid, <code>null</code> otherwise.
     * @example <caption>Valid and invalid parameters.</caption>
     * var uuid = Uuid.fromString("{527c27ea-6d7b-4b47-9ae2-b3051d50d2cd}");
     * print(uuid); // {527c27ea-6d7b-4b47-9ae2-b3051d50d2cd}
     *
     * uuid = Uuid.fromString("527c27ea-6d7b-4b47-9ae2-b3051d50d2cd");
     * print(uuid); // {527c27ea-6d7b-4b47-9ae2-b3051d50d2cd}
     *
     * uuid = Uuid.fromString("527c27ea");
     * print(uuid); // null
     */
/**
     * Generates a string representation of a UUID. However, because UUIDs are represented in JavaScript as strings, this is in
     * effect a no-op.
     * @function Uuid.toString
     * @param {Uuid} id - The UUID to generate a string from.
     * @returns {string} - A string representation of the UUID.
     */
/**
     * Generate a new UUID.
     * @function Uuid.generate
     * @returns {Uuid} A new UUID.
     * @example <caption>Generate a new UUID and reports its JavaScript type.</caption>
     * var uuid = Uuid.generate();
     * print(uuid);        // {nnnnnnnn-nnnn-nnnn-nnnn-nnnnnnnnnnnn}
     * print(typeof uuid); // string
     */
/**
     * Test whether two given UUIDs are equal.
     * @function Uuid.isEqual
     * @param {Uuid} idA - The first UUID to compare.
     * @param {Uuid} idB - The second UUID to compare.
     * @returns {boolean} <code>true</code> if the two UUIDs are equal, otherwise <code>false</code>.
     * @example <caption>Demonstrate <code>true</code> and <code>false</code> cases.</caption>
     * var uuidA = Uuid.generate();
     * var uuidB = Uuid.generate();
     * print(Uuid.isEqual(uuidA, uuidB)); // false
     * uuidB = uuidA;
     * print(Uuid.isEqual(uuidA, uuidB)); // true
     */
/**
     * Test whether a given UUID is null.
     * @function Uuid.isNull
     * @param {Uuid} id - The UUID to test.
     * @returns {boolean} <code>true</code> if the UUID equals Uuid.NULL or is <code>null</code>, otherwise <code>false</code>.
     * @example <caption>Demonstrate <code>true</code> and <code>false</code> cases.</caption>
     * var uuid; // undefined
     * print(Uuid.isNull(uuid)); // false
     * uuid = Uuid.generate();
     * print(Uuid.isNull(uuid)); // false
     * uuid = Uuid.NULL;
     * print(Uuid.isNull(uuid)); // true
     * uuid = null;
     * print(Uuid.isNull(uuid)); // true
     */
/**
     * Print to the program log a text label followed by the UUID value.
     * @function Uuid.print
     * @param {string} label - The label to print.
     * @param {Uuid} id - The UUID to print.
     * @example <caption>Two ways of printing a label plus UUID.</caption>
     * var uuid = Uuid.generate();
     * Uuid.print("Generated UUID:", uuid); // Generated UUID: {nnnnnnnn-nnnn-nnnn-nnnn-nnnnnnnnnnnn}
     * print("Generated UUID: " + uuid);    // Generated UUID: {nnnnnnnn-nnnn-nnnn-nnnn-nnnnnnnnnnnn}
     */
/**
* @namespace Users
*/
/**
    * Ignore another user.
    * @function Users.ignore
    * @param {nodeID} nodeID The node or session ID of the user you want to ignore.
    * @param {bool} enable True for ignored; false for un-ignored.
    */
/**
    * Gets a bool containing whether you have ignored the given Avatar UUID.
    * @function Users.getIgnoreStatus
    * @param {nodeID} nodeID The node or session ID of the user whose ignore status you want.
    */
/**
    * Mute another user for you and you only.
    * @function Users.personalMute
    * @param {nodeID} nodeID The node or session ID of the user you want to mute.
    * @param {bool} enable True for enabled; false for disabled.
    */
/**
    * Requests a bool containing whether you have personally muted the given Avatar UUID.
    * @function Users.requestPersonalMuteStatus
    * @param {nodeID} nodeID The node or session ID of the user whose personal mute status you want.
    */
/**
    * Sets an avatar's gain for you and you only.
    * Units are Decibels (dB)
    * @function Users.setAvatarGain
    * @param {nodeID} nodeID The node or session ID of the user whose gain you want to modify, or null to set the master gain.
    * @param {float} gain The gain of the avatar you'd like to set. Units are dB.
    */
/**
    * Gets an avatar's gain for you and you only.
    * @function Users.getAvatarGain
    * @param {nodeID} nodeID The node or session ID of the user whose gain you want to get, or null to get the master gain.
    * @return {float} gain (in dB)
    */
/**
    * Kick another user.
    * @function Users.kick
    * @param {nodeID} nodeID The node or session ID of the user you want to kick.
    */
/**
    * Mute another user for everyone.
    * @function Users.mute
    * @param {nodeID} nodeID The node or session ID of the user you want to mute.
    */
/**
    * Returns a string containing the username associated with the given Avatar UUID
    * @function Users.getUsernameFromID
    * @param {nodeID} nodeID The node or session ID of the user whose username you want.
    */
/**
    * Returns `true` if the DomainServer will allow this Node/Avatar to make kick
    * @function Users.getCanKick
    * @return {bool} `true` if the client can kick other users, `false` if not.
    */
/**
    * Toggle the state of the ignore in radius feature
    * @function Users.toggleIgnoreRadius
    */
/**
    * Enables the ignore radius feature.
    * @function Users.enableIgnoreRadius
    */
/**
    * Disables the ignore radius feature.
    * @function Users.disableIgnoreRadius
    */
/**
    * Returns `true` if the ignore in radius feature is enabled
    * @function Users.getIgnoreRadiusEnabled
    * @return {bool} `true` if the ignore in radius feature is enabled, `false` if not.
    */
/**
    * Notifies scripts that another user has entered the ignore radius
    * @function Users.enteredIgnoreRadius
    */
/**
    * Notifies scripts of the username and machine fingerprint associated with a UUID.
    * Username and machineFingerprint will be their default constructor output if the requesting user isn't an admin.
    * @function Users.usernameFromIDReply
    */
/**
     * Notifies scripts that a user has disconnected from the domain
     * @function Users.avatarDisconnected
     * @param {nodeID} NodeID The session ID of the avatar that has disconnected
     */
/**
 * A 2-dimensional vector.
 *
 * @typedef {object} Vec2
 * @property {number} x - X-coordinate of the vector.
 * @property {number} y - Y-coordinate of the vector.
 */
/**
 * A 3-dimensional vector.
 *
 * @typedef {object} Vec3
 * @property {number} x - X-coordinate of the vector.
 * @property {number} y - Y-coordinate of the vector.
 * @property {number} z - Z-coordinate of the vector.
 */
/**
 * A 4-dimensional vector.
 *
 * @typedef {object} Vec4
 * @property {number} x - X-coordinate of the vector.
 * @property {number} y - Y-coordinate of the vector.
 * @property {number} z - Z-coordinate of the vector.
 * @property {number} w - W-coordinate of the vector.
 */
/**
 * A color vector.
 *
 * @typedef {object} Vec3Color
 * @property {number} x - Red component value. Integer in the range <code>0</code> - <code>255</code>.
 * @property {number} y - Green component value. Integer in the range <code>0</code> - <code>255</code>.
 * @property {number} z - Blue component value. Integer in the range <code>0</code> - <code>255</code>.
 */
/**
 * Helper functions to render ephemeral debug markers and lines.
 * DebugDraw markers and lines are only visible locally, they are not visible by other users.
 * @namespace DebugDraw
 */
/**
     * Draws a line in world space, but it will only be visible for a single frame.
     * @function DebugDraw.drawRay
     * @param {Vec3} start - start position of line in world space.
     * @param {Vec3} end - end position of line in world space.
     * @param {Vec4} color - color of line, each component should be in the zero to one range.  x = red, y = blue, z = green, w = alpha.
     */
/**
     * Adds a debug marker to the world. This marker will be drawn every frame until it is removed with DebugDraw.removeMarker.
     * This can be called repeatedly to change the position of the marker.
     * @function DebugDraw.addMarker
     * @param {string} key - name to uniquely identify this marker, later used for DebugDraw.removeMarker.
     * @param {Quat} rotation - start position of line in world space.
     * @param {Vec3} position - position of the marker in world space.
     * @param {Vec4} color - color of the marker.
     */
/**
     * Removes debug marker from the world.  Once a marker is removed, it will no longer be visible.
     * @function DebugDraw.removeMarker
     * @param {string} key - name of marker to remove.
     */
/**
     * Adds a debug marker to the world, this marker will be drawn every frame until it is removed with DebugDraw.removeMyAvatarMarker.
     * This can be called repeatedly to change the position of the marker.
     * @function DebugDraw.addMyAvatarMarker
     * @param {string} key - name to uniquely identify this marker, later used for DebugDraw.removeMyAvatarMarker.
     * @param {Quat} rotation - start position of line in avatar space.
     * @param {Vec3} position - position of the marker in avatar space.
     * @param {Vec4} color - color of the marker.
     */
/**
     * Removes debug marker from the world.  Once a marker is removed, it will no longer be visible
     * @function DebugDraw.removeMyAvatarMarker
     * @param {string} key - name of marker to remove.
     */
/**
 * The Paths API provides absolute paths to the scripts and resources directories.
 *
 * @namespace Paths
 * @deprecated The Paths API is deprecated. Use {@link Script.resolvePath} and {@link Script.resourcesPath} instead.
 * @readonly
 * @property {string} defaultScripts - The path to the scripts directory. <em>Read-only.</em>
 * @property {string} resources - The path to the resources directory. <em>Read-only.</em>
 */
/**
 * <p>An entity may collide with the following types of items:</p>
 * <table>
 *   <thead>
 *     <tr><th>Value</th><th>Description</th>
 *   </thead>
 *   <tbody>
 *     <tr><td><code>1</code></td><td>Static entities &mdash; non-dynamic entities with no velocity.</td></tr>
 *     <tr><td><code>2</code></td><td>Dynamic entities &mdash; entities that have their <code>dynamic</code> property set to 
 *         <code>true</code>.</td></tr>
 *     <tr><td><code>4</code></td><td>Kinematic entities &mdash; non-dynamic entities with velocity.</td></tr>
 *     <tr><td><code>8</code></td><td>My avatar.</td></tr>
 *     <tr><td><code>16</code></td><td>Other avatars.</td></tr>
 *   </tbody>
 * </table>
 * <p>The values for the collision types that are enabled are added together to give the CollisionMask value. For example, a 
 * value of <code>31</code> means that an entity will collide with all item types.</p>
 * @typedef {number} Entities.CollisionMask
 */
/**
 * A PointerEvent defines a 2D or 3D mouse or similar pointer event.
 * @typedef {object} PointerEvent
 * @property {string} type - The type of event: <code>"Press"</code>, <code>"DoublePress"</code>, <code>"Release"</code>, or
 *     <code>"Move"</code>.
 * @property {number} id - Integer number used to identify the pointer: <code>0</code> = hardware mouse, <code>1</code> = left
 *     controller, <code>2</code> = right controller.
 * @property {Vec2} pos2D - The 2D position of the event on the intersected overlay or entity XY plane, where applicable.
 * @property {Vec3} pos3D - The 3D position of the event on the intersected overlay or entity, where applicable.
 * @property {Vec3} normal - The surface normal at the intersection point.
 * @property {Vec3} direction - The direction of the intersection ray.
 * @property {string} button - The name of the button pressed: <code>None</code>, <code>Primary</code>, <code>Secondary</code>,
 *    or <code>Tertiary</code>.
 * @property {boolean} isPrimaryButton - <code>true</code> if the button pressed was the primary button, otherwise 
 *     <code>undefined</code>;
 * @property {boolean} isLeftButton - <code>true</code> if the button pressed was the primary button, otherwise
 *     <code>undefined</code>;
 * @property {boolean} isSecondaryButton - <code>true</code> if the button pressed was the secondary button, otherwise
 *     <code>undefined</code>;
 * @property {boolean} isRightButton - <code>true</code> if the button pressed was the secondary button, otherwise
 *     <code>undefined</code>;
 * @property {boolean} isTertiaryButton - <code>true</code> if the button pressed was the tertiary button, otherwise
 *     <code>undefined</code>;
 * @property {boolean} isMiddleButton - <code>true</code> if the button pressed was the tertiary button, otherwise
 *     <code>undefined</code>;
 * @property {boolean} isPrimaryHeld - <code>true</code> if the primary button is currently being pressed, otherwise
 *     <code>false</code>
 * @property {boolean} isSecondaryHeld - <code>true</code> if the secondary button is currently being pressed, otherwise
 *     <code>false</code>
 * @property {boolean} isTertiaryHeld - <code>true</code> if the tertiary button is currently being pressed, otherwise
 *     <code>false</code>
 * @property {KeyboardModifiers} keyboardModifiers - Integer value with bits set according to which keyboard modifier keys were
 *     pressed when the event was generated.
 */
/**
 * <p>A KeyboardModifiers value is used to specify which modifier keys on the keyboard are pressed. The value is the sum 
 * (bitwise OR) of the relevant combination of values from the following table:</p>
 * <table>
 *   <thead>
 *     <tr><th>Key</th><th>Hexadecimal value</th><th>Decimal value</th><th>Description</th></tr>
 *   </thead>
 *   <tbody>
 *     <tr><td>Shift</td><td><code>0x02000000</code></td><td><code>33554432</code></td>
 *         <td>A Shift key on the keyboard is pressed.</td></tr>
 *     <tr><td>Control</td><td><code>0x04000000</code></td><td><code>67108864</code></td>
 *         <td>A Control key on the keyboard is pressed.</td></tr>
 *     <tr><td>Alt</td><td><code>0x08000000</code></td><td><code>134217728</code></td>
 *         <td>An Alt key on the keyboard is pressed.</td></tr>
 *     <tr><td>Meta</td><td><code>0x10000000</code></td><td><code>268435456</code></td>
 *         <td>A Meta or Windows key on the keyboard is pressed.</td></tr>
 *     <tr><td>Keypad</td><td><code>0x20000000</code></td><td><code>536870912</code></td>
 *         <td>A keypad button is pressed.</td></tr>
 *     <tr><td>Group</td><td><code>0x40000000</code></td><td><code>1073741824</code></td>
 *         <td>X11 operating system only: An AltGr / Mode_switch key on the keyboard is pressed.</td></tr>
 *   </tbody>
 * </table>
 * @typedef {number} KeyboardModifiers
 */
/**
 * Defines a rectangular portion of an image or screen.
 * @typedef {object} Rect
 * @property {number} x - Integer left, x-coordinate value.
 * @property {number} y - Integer top, y-coordinate value.
 * @property {number} width - Integer width of the rectangle.
 * @property {number} height - Integer height of the rectangle.
 */
/**
 * An RGB color value.
 * @typedef {object} Color
 * @property {number} red - Red component value. Integer in the range <code>0</code> - <code>255</code>.
 * @property {number} green - Green component value. Integer in the range <code>0</code> - <code>255</code>.
 * @property {number} blue - Blue component value. Integer in the range <code>0</code> - <code>255</code>.
 */
/**
 * An axis-aligned cube, defined as the bottom right near (minimum axes values) corner of the cube plus the dimension of its 
 * sides.
 * @typedef {object} AACube
 * @property {number} x - X coordinate of the brn corner of the cube.
 * @property {number} y - Y coordinate of the brn corner of the cube.
 * @property {number} z - Z coordinate of the brn corner of the cube.
 * @property {number} scale - The dimensions of each side of the cube.
 */
/**
 * @typedef {object} Collision
 * @property {ContactEventType} type - The contact type of the collision event.
 * @property {Uuid} idA - The ID of one of the entities in the collision.
 * @property {Uuid} idB - The ID of the other of the entities in the collision.
 * @property {Vec3} penetration - The amount of penetration between the two entities.
 * @property {Vec3} contactPoint - The point of contact.
 * @property {Vec3} velocityChange - The change in relative velocity of the two entities, in m/s.
 */
/**
 * A 2D size value.
 * @typedef {object} Size
 * @property {number} height - The height value.
 * @property {number} width - The width value.
 */
/**
 * A PickRay defines a vector with a starting point. It is used, for example, when finding entities or overlays that lie under a
 * mouse click or intersect a laser beam.
 *
 * @typedef {object} PickRay
 * @property {Vec3} origin - The starting position of the PickRay.
 * @property {Vec3} direction - The direction that the PickRay travels.
 */
/**
 * A StylusTip defines the tip of a stylus.
 *
 * @typedef {object} StylusTip
 * @property {number} side - The hand the tip is attached to: <code>0</code> for left, <code>1</code> for right.
 * @property {Vec3} position - The position of the stylus tip.
 * @property {Quat} orientation - The orientation of the stylus tip.
 * @property {Vec3} velocity - The velocity of the stylus tip.
 */
/**
 * <p>The type of a collision contact event.
 * <table>
 *   <thead>
 *     <tr><th>Value</th><th>Description</th></tr>
 *   </thead>
 *   <tbody>
 *     <tr><td><code>0</code></td><td>Start of the collision.</td></tr>
 *     <tr><td><code>1</code></td><td>Continuation of the collision.</td></tr>
 *     <tr><td><code>2</code></td><td>End of the collision.</td></tr>
 *   </tbody>
 * </table>
 * @typedef {number} ContactEventType
 */
/**
 * A handle for a mesh in an entity, such as returned by {@link Entities.getMeshes}.
 * @class MeshProxy
 * @deprecated Use the {@link Graphics} API instead.
 */
/**
     * Get the number of vertices in the mesh.
     * @function MeshProxy#getNumVertices
     * @returns {number} Integer number of vertices in the mesh.
     * @deprecated Use the {@link Graphics} API instead.
     */
/**
     * Get the position of a vertex in the mesh.
     * @function MeshProxy#getPos
     * @param {number} index - Integer index of the mesh vertex.
     * @returns {Vec3} Local position of the vertex relative to the mesh.
     * @deprecated Use the {@link Graphics} API instead.
     */
/**
 * <p>A ShapeType defines the shape used for collisions or zones.</p>
 * <table>
 *   <thead>
 *     <tr><th>Value</th><th>Description</th></tr>
 *   </thead>
 *   <tbody>
 *     <tr><td><code>"none"</code></td><td>No shape.</td></tr>
 *     <tr><td><code>"box"</code></td><td>A cube.</td></tr>
 *     <tr><td><code>"sphere"</code></td><td>A sphere.</td></tr>
 *     <tr><td><code>"capsule-x"</code></td><td>A capsule (cylinder with spherical ends) oriented on the x-axis.</td></tr>
 *     <tr><td><code>"capsule-y"</code></td><td>A capsule (cylinder with spherical ends) oriented on the y-axis.</td></tr>
 *     <tr><td><code>"capsule-z"</code></td><td>A capsule (cylinder with spherical ends) oriented on the z-axis.</td></tr>
 *     <tr><td><code>"cylinder-x"</code></td><td>A cylinder oriented on the x-axis.</td></tr>
 *     <tr><td><code>"cylinder-y"</code></td><td>A cylinder oriented on the y-axis.</td></tr>
 *     <tr><td><code>"cylinder-z"</code></td><td>A cylinder oriented on the z-axis.</td></tr>
 *     <tr><td><code>"hull"</code></td><td><em>Not used.</em></td></tr>
 *     <tr><td><code>"compound"</code></td><td>A compound convex hull specified in an OBJ file.</td></tr>
 *     <tr><td><code>"simple-hull"</code></td><td>A convex hull automatically generated from the model.</td></tr>
 *     <tr><td><code>"simple-compound"</code></td><td>A compound convex hull automatically generated from the model, using 
 *         sub-meshes.</td></tr>
 *     <tr><td><code>"static-mesh"</code></td><td>The exact shape of the model.</td></tr>
 *     <tr><td><code>"plane"</code></td><td>A plane.</td></tr>
 *   </tbody>
 * </table>
 * @typedef {string} ShapeType
 */
/**
 * <p>Camera modes affect the position of the camera and the controls for camera movement. The camera can be in one of the
 * following modes:</p>
 * <table>
 *   <thead>
 *     <tr>
 *       <th>Mode</th>
 *       <th>String</th>
 *       <th>Description</th>
 *     </tr>
 *   </thead>
 *   <tbody>
 *     <tr>
 *       <td><strong>First&nbsp;Person</strong></td>
 *       <td><code>"first&nbsp;person"</code></td>
 *       <td>The camera is positioned such that you have the same view as your avatar. The camera moves and rotates with your
 *       avatar.</td>
 *     </tr>
 *     <tr>
 *       <td><strong>Third&nbsp;Person</strong></td>
 *       <td><code>"third&nbsp;person"</code></td>
 *       <td>The camera is positioned such that you have a view from just behind your avatar. The camera moves and rotates with
 *       your avatar.</td>
 *     </tr>
 *     <tr>
 *       <td><strong>Mirror</strong></td>
 *       <td><code>"mirror"</code></td>
 *       <td>The camera is positioned such that you are looking directly at your avatar. The camera moves and rotates with your 
 *       avatar.</td>
 *     </tr>
 *     <tr>
 *       <td><strong>Independent</strong></td>
 *       <td><code>"independent"</code></td>
 *       <td>The camera's position and orientation don't change with your avatar movement. Instead, they can be set via 
 *       scripting.</td>
 *     </tr>
 *     <tr>
 *       <td><strong>Entity</strong></td>
 *       <td><code>"entity"</code></td>
 *       <td>The camera's position and orientation are set to be the same as a specified entity's, and move with the entity as
 *       it moves.
 *     </tr>
 *   </tbody>
 * </table>
 * @typedef {string} Camera.Mode
 */
/**
 * A ViewFrustum has a "keyhole" shape: a regular frustum for stuff that is visible plus a central sphere for stuff that is
 * nearby (for physics simulation).
 *
 * @typedef {object} ViewFrustum
 * @property {Vec3} position - The location of the frustum's apex.
 * @property {Quat} orientation - The direction that the frustum is looking at.
 * @property {number} centerRadius - Center radius of the keyhole in meters.
 * @property {number} fieldOfView - Horizontal field of view in degrees.
 * @property {number} aspectRatio - Aspect ratio of the frustum.
 * @property {Mat4} projection - The projection matrix for the view defined by the frustum.
 */
/**
     * The Camera API provides access to the "camera" that defines your view in desktop and HMD display modes.
     *
     * @namespace Camera
     * @property position {Vec3} The position of the camera. You can set this value only when the camera is in independent mode.
     * @property orientation {Quat} The orientation of the camera. You can set this value only when the camera is in independent
     *     mode.
     * @property mode {Camera.Mode} The camera mode.
     * @property frustum {ViewFrustum} The camera frustum.
     * @property cameraEntity {Uuid} The ID of the entity that is used for the camera position and orientation when the camera
     *     is in entity mode.
     */
/**
     * Get the current camera mode. You can also get the mode using the <code>Camera.mode</code> property.
     * @function Camera.getModeString
     * @returns {Camera.Mode} The current camera mode.
     */
/**
    * Set the camera mode. You can also set the mode using the <code>Camera.mode</code> property.
    * @function Camera.setModeString
    * @param {Camera.Mode} mode - The mode to set the camera to.
    */
/**
    * Get the current camera position. You can also get the position using the <code>Camera.position</code> property.
    * @function Camera.getPosition
    * @returns {Vec3} The current camera position.
    */
/**
    * Set the camera position. You can also set the position using the <code>Camera.position</code> property. Only works if the
    *     camera is in independent mode.
    * @function Camera.setPosition
    * @param {Vec3} position - The position to set the camera at.
    */
/**
    * Get the current camera orientation. You can also get the orientation using the <code>Camera.orientation</code> property.
    * @function Camera.getOrientation
    * @returns {Quat} The current camera orientation.
    */
/**
    * Set the camera orientation. You can also set the orientation using the <code>Camera.orientation</code> property. Only
    *     works if the camera is in independent mode.
    * @function Camera.setOrientation
    * @param {Quat} orientation - The orientation to set the camera to.
    */
/**
     * Compute a {@link PickRay} based on the current camera configuration and the specified <code>x, y</code> position on the 
     *     screen. The {@link PickRay} can be used in functions such as {@link Entities.findRayIntersection} and 
     *     {@link Overlays.findRayIntersection}.
     * @function Camera.computePickRay
     * @param {number} x - X-coordinate on screen.
     * @param {number} y - Y-coordinate on screen.
     * @returns {PickRay} The computed {@link PickRay}.
     * @example <caption>Use a PickRay to detect mouse clicks on entities.</caption>
     * function onMousePressEvent(event) {
     *     var pickRay = Camera.computePickRay(event.x, event.y);
     *     var intersection = Entities.findRayIntersection(pickRay);
     *     if (intersection.intersects) {
     *         print ("You clicked on entity " + intersection.entityID);
     *     }
     * }
     *
     * Controller.mousePressEvent.connect(onMousePressEvent);
     */
/**
     * Rotate the camera to look at the specified <code>position</code>. Only works if the camera is in independent mode.
     * @function Camera.lookAt
     * @param {Vec3} position - Position to look at.
     * @example <caption>Rotate your camera to look at entities as you click on them with your mouse.</caption>
     * function onMousePressEvent(event) {
     *     var pickRay = Camera.computePickRay(event.x, event.y);
     *     var intersection = Entities.findRayIntersection(pickRay);
     *     if (intersection.intersects) {
     *         // Switch to independent mode.
     *         Camera.mode = "independent";
     *         // Look at the entity that was clicked.
     *         var properties = Entities.getEntityProperties(intersection.entityID, "position");
     *         Camera.lookAt(properties.position);
     *     }
     * }
     *
     * Controller.mousePressEvent.connect(onMousePressEvent);
     */
/**
     * Set the camera to continue looking at the specified <code>position</code> even while the camera moves. Only works if the 
     * camera is in independent mode.
     * @function Camera.keepLookingAt
     * @param {Vec3} position - Position to keep looking at.
     */
/**
     * Stops the camera from continually looking at the position that was set with <code>Camera.keepLookingAt</code>.
     * @function Camera.stopLookingAt
     */
/**
     * Triggered when the camera mode changes.
     * @function Camera.modeUpdated
     * @param {Camera.Mode} newMode - The new camera mode.
     * @returns {Signal}
     * @example <caption>Report camera mode changes.</caption>
     * function onCameraModeUpdated(newMode) {
     *     print("The camera mode has changed to " + newMode);
     * }
     *
     * Camera.modeUpdated.connect(onCameraModeUpdated);
     */
