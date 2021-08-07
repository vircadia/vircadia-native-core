//
//  ScriptsModelFilter.h
//  interface/src
//
//  Created by Thijs Wenker on 01/11/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/// @addtogroup ScriptEngine
/// @{

#ifndef hifi_ScriptsModelFilter_h
#define hifi_ScriptsModelFilter_h

#include "ScriptsModel.h"
#include <QSortFilterProxyModel>

/*@jsdoc
 * Sorted and filtered information on the scripts that are in the default scripts directory of the Interface installation. This 
 * is provided as a property of {@link ScriptDiscoveryService}.
 *
 * <p>The information provided reflects the subdirectory structure. Properties, methods, and signals are per QT's
 * <a href="https://doc.qt.io/qt-5/qsortfilterproxymodel.html">QSortFilterProxyModel</a> class, with the following details:</p>
 * <ul>
 *   <li>The rows are sorted per directory and file names.</li>
 *   <li>A single column of data: <code>columnCount(index)</code> returns <code>1</code>. </li>
 *   <li>Data is provided for the following roles:
 *     <table>
 *       <thead>
 *         <tr><th>Role</th><th>Value</th><th>Description</th></tr>
 *       </thead>
 *       <tbody>
 *         <tr><td>Display</td><td><code>0</code></td><td>The directory or script file name.</td></tr>
 *         <tr><td>Path</td><td><code>256</code></td><td>The path and filename of the data item if it is a script,
 *         <code>undefined</code> if it is a directory.</td></tr>
 *       </tbody>
 *     </table>
 *   </li>
 *   <li>Use <code>null</code> for the root directory's index.</li>
 * </ul>
 *
 * @class ScriptsModelFilter
 * @hideconstructor
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-avatar
 *
 * @example <caption>List all scripts that include "edit" in their name.</caption>
 * var DISPLAY_ROLE = 0;
 * var PATH_ROLE = 256;
 * 
 * function printDirectory(parentIndex, directoryLevel, indent) {
 *     var numRows = ScriptDiscoveryService.scriptsModelFilter.rowCount(parentIndex);
 *     for (var i = 0; i < numRows; i++) {
 *         var rowIndex = ScriptDiscoveryService.scriptsModelFilter.index(i, 0, parentIndex);
 * 
 *         var name = ScriptDiscoveryService.scriptsModelFilter.data(rowIndex, DISPLAY_ROLE);
 *         var hasChildren = ScriptDiscoveryService.scriptsModelFilter.hasChildren(rowIndex);
 *         var path = hasChildren ? "" : ScriptDiscoveryService.scriptsModelFilter.data(rowIndex, PATH_ROLE);
 * 
 *         print(indent + "- " + name + (hasChildren ? "" : " - " + path));
 * 
 *         if (hasChildren) {
 *             printDirectory(rowIndex, directoryLevel + 1, indent + "    ");
 *         }
 *     }
 * }
 * 
 * ScriptDiscoveryService.scriptsModelFilter.filterRegExp = new RegExp("^.*edit.*$", "i");  // Set the filter.
 * print("Edit scripts:");
 * printDirectory(null, 0, "");  // null index for the root directory.
 */
/// Provides script file information available from the <code><a href="https://apidocs.vircadia.dev/ScriptDiscoveryService.html">ScriptDiscoveryService</a></code> scripting interface
class ScriptsModelFilter : public QSortFilterProxyModel {
    Q_OBJECT
public:
    ScriptsModelFilter(QObject *parent = NULL);
protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;
};

#endif // hifi_ScriptsModelFilter_h

/// @}
