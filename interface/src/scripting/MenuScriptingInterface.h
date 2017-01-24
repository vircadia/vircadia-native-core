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
 * The `Menu` provides access to the menu that is shown at the top of the window
 * shown on a user's desktop and the right click menu that is accessible
 * in both Desktop and HMD mode.
 *
 * <h3>Groupings</h3>
 * A `grouping` is a way to group a set of menus and/or menu items together
 * so that they can all be set visible or invisible as a group. There are
 * 2 available groups: "Advanced" and "Developer"
 * These groupings can be toggled in the "Settings" menu.
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
     * @param {string} menuName Name that will be shown in the menu.
     * @param {string} grouping Name of the grouping to add this menu to.
     */
    void addMenu(const QString& menuName, const QString& grouping = QString());

    /**jsdoc
     * Remove a top-level menu.
     * @function Menu.removeMenu
     * @param {string} menuName Name of the menu to remove.
     */
    void removeMenu(const QString& menuName);

    /**jsdoc
     * Check whether a top-level menu exists.
     * @function Menu.menuExists
     * @param {string} menuName Name of the menu to check for existence.
     * @return {bool} `true` if the menu exists, otherwise `false`.
     */
    bool menuExists(const QString& menuName);

    /**jsdoc
     * Add a separator with an unclickable label below it.
     * The line will be placed at the bottom of the menu.
     * @function Menu.addSeparator
     * @param {string} menuName Name of the menu to add a separator to.
     * @param {string} separatorName Name of the separator that will be shown (but unclickable) below the separator line.
     */
    void addSeparator(const QString& menuName, const QString& separatorName);

    /**jsdoc
     * Remove a separator and its label from a menu.
     * @function Menu.removeSeparator
     * @param {string} menuName Name of the menu to remove a separator from.
     * @param {string} separatorName Name of the separator to remove.
     */
    void removeSeparator(const QString& menuName, const QString& separatorName);

    /**jsdoc
     * Add a new menu item to a menu.
     * @function Menu.addMenuItem
     * @param {Menu.MenuItemProperties} properties
     */
    void addMenuItem(const MenuItemProperties& properties);

    /**jsdoc
     * Add a new menu item to a menu.
     * @function Menu.addMenuItem
     * @param {string} menuName Name of the menu to add a menu item to.
     * @param {string} menuItem Name of the menu item. This is what will be displayed in the menu.
     * @param {string} shortcutKey A shortcut key that can be used to trigger the menu item.
     */
    void addMenuItem(const QString& menuName, const QString& menuitem, const QString& shortcutKey);

    /**jsdoc
     * Add a new menu item to a menu.
     * @function Menu.addMenuItem
     * @param {string} menuName Name of the menu to add a menu item to.
     * @param {string} menuItem Name of the menu item. This is what will be displayed in the menu.
     */
    void addMenuItem(const QString& menuName, const QString& menuitem);

    /**jsdoc
     * Remove a menu item from a menu.
     * @function Menu.removeMenuItem
     * @param {string} menuName Name of the menu to remove a menu item from.
     * @param {string} menuItem Name of the menu item to remove.
     */
    void removeMenuItem(const QString& menuName, const QString& menuitem);

    /**jsdoc
     * Check if a menu item exists.
     * @function Menu.menuItemExists
     * @param {string} menuName Name of the menu that the menu item is in.
     * @param {string} menuItem Name of the menu item to check for existence of.
     * @return {bool} `true` if the menu item exists, otherwise `false`.
     */
    bool menuItemExists(const QString& menuName, const QString& menuitem);

    /**
     * Not working, will not document until fixed
     */
    void addActionGroup(const QString& groupName, const QStringList& actionList,
                        const QString& selected = QString());
    void removeActionGroup(const QString& groupName);

    /**jsdoc
     * Check whether a checkable menu item is checked.
     * @function Menu.isOptionChecked
     * @param {string} menuOption The name of the menu item.
     * @return `true` if the option is checked, otherwise false.
     */
    bool isOptionChecked(const QString& menuOption);

    /**jsdoc
     * Set a checkable menu item as checked or unchecked.
     * @function Menu.setIsOptionChecked
     * @param {string} menuOption The name of the menu item to modify.
     * @param {bool} isChecked If `true`, the menu item will be checked, otherwise it will not be checked.
     */
    void setIsOptionChecked(const QString& menuOption, bool isChecked);

    /**jsdoc
     * Toggle the status of a checkable menu item. If it is checked, it will be unchecked.
     * If it is unchecked, it will be checked.
     * @function Menu.setIsOptionChecked
     * @param {string} menuOption The name of the menu item to toggle.
     */
    void triggerOption(const QString& menuOption);

    /**jsdoc
     * Check whether a menu is enabled. If a menu is disabled it will be greyed out
     * and unselectable.
     * Menus are enabled by default.
     * @function Menu.isMenuEnabled
     * @param {string} menuName The name of the menu to check.
     * @return {bool} `true` if the menu is enabled, otherwise false.
     */
    bool isMenuEnabled(const QString& menuName);

    /**jsdoc
     * Set a menu to be enabled or disabled.
     * @function Menu.setMenuEnabled
     * @param {string} menuName The name of the menu to modify.
     * @param {bool} isEnabled Whether the menu will be enabled or not.
     */
    void setMenuEnabled(const QString& menuName, bool isEnabled);

    void closeInfoView(const QString& path);
    bool isInfoViewVisible(const QString& path);

signals:
    /**jsdoc
     * This is a signal that is emitted when a menu item is clicked.
     * @function Menu.menuItemEvent
     * @param {string} menuItem Name of the menu item that was triggered.
     */
    void menuItemEvent(const QString& menuItem);
};

#endif // hifi_MenuScriptingInterface_h
