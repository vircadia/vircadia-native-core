//
//  MenuScriptingInterface.h
//  interface/src/scripting
//
//  Created by Brad Hefta-Gaub on 2/25/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_MenuScriptingInterface_h
#define hifi_MenuScriptingInterface_h

#include <QObject>
#include <QString>

class MenuItemProperties;

/**jsdoc
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
 * CURRENTLY NOT WORKING:
 *
 * <h3>Action groups</h3>
 * When 1+ menu items are checkable and in the same action group, only 1 can be
 * selected at any one time. If another item in the action group is selected, the
 * previous will be deselected. This feature provides the ability to create
 * "radio-button"-like menus.
 */

class MenuScriptingInterface : public QObject {
    Q_OBJECT
    MenuScriptingInterface() { };
public:
    static MenuScriptingInterface* getInstance();

private slots:
    friend class Menu;
    void menuItemTriggered();

public slots:
    /**jsdoc
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
    void addMenu(const QString& menuName, const QString& grouping = QString());

    /**jsdoc
     * Remove a top-level menu.
     * @function Menu.removeMenu
     * @param {string} menuName - Name of the menu to remove.
     * @example <caption>Remove a menu and nested submenu.</caption>
     * Menu.removeMenu("Test Menu > Test Sub Menu");
     * Menu.removeMenu("Test Menu");
     */
    void removeMenu(const QString& menuName);

    /**jsdoc
     * Check whether a top-level menu exists.
     * @function Menu.menuExists
     * @param {string} menuName - Name of the menu to check for existence.
     * @returns {boolean} <code>true</code> if the menu exists, otherwise <code>false</code>.
     * @example <caption>Check if the "Developer" menu exists.</caption>
     * if (Menu.menuExists("Developer")) {
     *     print("Developer menu exists.");
     * }
     */
    bool menuExists(const QString& menuName);

    /**jsdoc
     * Add a separator with an unclickable label below it. The separator will be placed at the bottom of the menu.
     * If you want to add a separator at a specific point in the menu, use {@link Menu.addMenuItem} with
     * {@link Menu.MenuItemProperties} instead.
     * @function Menu.addSeparator
     * @param {string} menuName - Name of the menu to add a separator to.
     * @param {string} separatorName - Name of the separator that will be displayed as the label below the separator line.
     * @example <caption>Add a separator.</caption>
     * Menu.addSeparator("Developer","Test Separator");
     */
    void addSeparator(const QString& menuName, const QString& separatorName);

    /**jsdoc
     * Remove a separator from a menu.
     * @function Menu.removeSeparator
     * @param {string} menuName - Name of the menu to remove the separator from.
     * @param {string} separatorName - Name of the separator to remove.
     * @example <caption>Remove a separator.</caption>
     * Menu.removeSeparator("Developer","Test Separator");
     */
    void removeSeparator(const QString& menuName, const QString& separatorName);

    /**jsdoc
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
    void addMenuItem(const MenuItemProperties& properties);

    /**jsdoc
     * Add a new menu item to a menu. The new item is added at the end of the menu.
     * @function Menu.addMenuItem
     * @param {string} menuName - Name of the menu to add a menu item to.
     * @param {string} menuItem - Name of the menu item. This is what will be displayed in the menu.
     * @param {string} [shortcutKey] A shortcut key that can be used to trigger the menu item.
     * @example <caption>Add a menu item to the end of the "Developer" menu.</caption>
     * Menu.addMenuItem("Developer", "Test", "Ctrl+Shift+T");
     */
    void addMenuItem(const QString& menuName, const QString& menuitem, const QString& shortcutKey);
    void addMenuItem(const QString& menuName, const QString& menuitem);

    /**jsdoc
     * Remove a menu item from a menu.
     * @function Menu.removeMenuItem
     * @param {string} menuName - Name of the menu to remove a menu item from.
     * @param {string} menuItem - Name of the menu item to remove.
     * Menu.removeMenuItem("Developer", "Test");
     */
    void removeMenuItem(const QString& menuName, const QString& menuitem);

    /**jsdoc
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
    bool menuItemExists(const QString& menuName, const QString& menuitem);

    /**
     * TODO: Not working; don't document until fixed.
     */
    void addActionGroup(const QString& groupName, const QStringList& actionList,
                        const QString& selected = QString());
    void removeActionGroup(const QString& groupName);

    /**jsdoc
     * Check whether a checkable menu item is checked.
     * @function Menu.isOptionChecked
     * @param {string} menuOption - The name of the menu item.
     * @returns {boolean} <code>true</code> if the option is checked, otherwise <code>false</code>.
     * @example <caption>Report whether the Settings > Advanced menu item is turned on.</caption>
     * print(Menu.isOptionChecked("Advanced Menus")); // true or false
     */
    bool isOptionChecked(const QString& menuOption);

    /**jsdoc
     * Set a checkable menu item as checked or unchecked.
     * @function Menu.setIsOptionChecked
     * @param {string} menuOption - The name of the menu item to modify.
     * @param {boolean} isChecked - If <code>true</code>, the menu item will be checked, otherwise it will not be checked.
     * @example <caption>Turn on Settings > Advanced Menus.</caption>
     * Menu.setIsOptionChecked("Advanced Menus", true);
     * print(Menu.isOptionChecked("Advanced Menus")); // true
     */
    void setIsOptionChecked(const QString& menuOption, bool isChecked);

    /**jsdoc
     * Trigger the menu item as if the user clicked on it.
     * @function Menu.triggerOption
     * @param {string} menuOption - The name of the menu item to trigger.
     * @example <caption>Open the help window.</caption>
     * Menu.triggerOption('Help...');
     */
    void triggerOption(const QString& menuOption);

    /**jsdoc
     * Check whether a menu or menu item is enabled. If disabled, the item is grayed out and unusable.
     * Menus are enabled by default.
     * @function Menu.isMenuEnabled
     * @param {string} menuName The name of the menu or menu item to check.
     * @returns {boolean} <code>true</code> if the menu is enabled, otherwise <code>false</code>.
     * @example <caption>Report with the Settings > Advanced Menus menu item is enabled.</caption>
     * print(Menu.isMenuEnabled("Settings > Advanced Menus")); // true or false
     */
    bool isMenuEnabled(const QString& menuName);

    /**jsdoc
     * Set a menu or menu item to be enabled or disabled. If disabled, the item is grayed out and unusable.
     * @function Menu.setMenuEnabled
     * @param {string} menuName - The name of the menu or menu item to modify.
     * @param {boolean} isEnabled - If <code>true</code>, the menu will be enabled, otherwise it will be disabled.
     * @example <caption>Disable the Settings > Advanced Menus menu item.</caption>
     * Menu.setMenuEnabled("Settings > Advanced Menus", false);
     * print(Menu.isMenuEnabled("Settings > Advanced Menus")); // false
     */
    void setMenuEnabled(const QString& menuName, bool isEnabled);

    /**
    * TODO: Not used or useful; will not document until used.
    */
    void closeInfoView(const QString& path);
    bool isInfoViewVisible(const QString& path);

signals:
    /**jsdoc
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
    void menuItemEvent(const QString& menuItem);
};

#endif // hifi_MenuScriptingInterface_h
