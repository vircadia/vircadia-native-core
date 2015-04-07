//
//  ScriptsScriptingInterface.cpp
//  interface/src/scripting
//
//  Created by Thijs Wenker on 3/31/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Application.h"

#include "ScriptsScriptingInterface.h"

ScriptsScriptingInterface* ScriptsScriptingInterface::getInstance() {
    static ScriptsScriptingInterface sharedInstance;
    return &sharedInstance;
}

QVariantList ScriptsScriptingInterface::getRunning() {
    const int WINDOWS_DRIVE_LETTER_SIZE = 1;
    QVariantList result;
    QStringList runningScripts = Application::getInstance()->getRunningScripts();
    for (int i = 0; i < runningScripts.size(); i++) {
        QUrl runningScriptURL = QUrl(runningScripts.at(i));
        if (runningScriptURL.scheme().size() <= WINDOWS_DRIVE_LETTER_SIZE) {
            runningScriptURL = QUrl::fromLocalFile(runningScriptURL.toDisplayString(QUrl::FormattingOptions(QUrl::FullyEncoded)));
        }
        QVariantMap resultNode;
        resultNode.insert("name", runningScriptURL.fileName());
        resultNode.insert("url", runningScriptURL.toDisplayString(QUrl::FormattingOptions(QUrl::FullyEncoded)));
        resultNode.insert("local", runningScriptURL.isLocalFile());
        result.append(resultNode);
    }
    return result;
}

QVariantList ScriptsScriptingInterface::getPublic() {
    return getPublicChildNodes(NULL);
}

QVariantList ScriptsScriptingInterface::getPublicChildNodes(TreeNodeFolder* parent) {
    QVariantList result;
    QList<TreeNodeBase*> treeNodes = Application::getInstance()->getRunningScriptsWidget()->getScriptsModel()
        ->getFolderNodes(parent);
    for (int i = 0; i < treeNodes.size(); i++) {
        TreeNodeBase* node = treeNodes.at(i);
        if (node->getType() == TREE_NODE_TYPE_FOLDER) {
            TreeNodeFolder* folder = static_cast<TreeNodeFolder*>(node);
            QVariantMap resultNode;
            resultNode.insert("name", node->getName());
            resultNode.insert("type", "folder");
            resultNode.insert("children", getPublicChildNodes(folder));
            result.append(resultNode);
            continue;
        }
        TreeNodeScript* script = static_cast<TreeNodeScript*>(node);
        if (script->getOrigin() == ScriptOrigin::SCRIPT_ORIGIN_LOCAL) {
            continue;
        }
        QVariantMap resultNode;
        resultNode.insert("name", node->getName());
        resultNode.insert("type", "script");
        resultNode.insert("url", script->getFullPath());
        result.append(resultNode);
    }
    return result;
}

QVariantList ScriptsScriptingInterface::getLocal() {
    QVariantList result;
    QList<TreeNodeBase*> treeNodes = Application::getInstance()->getRunningScriptsWidget()->getScriptsModel()
        ->getFolderNodes(NULL);
    for (int i = 0; i < treeNodes.size(); i++) {
        TreeNodeBase* node = treeNodes.at(i);
        if (node->getType() != TREE_NODE_TYPE_SCRIPT) {
            continue;
        }
        TreeNodeScript* script = static_cast<TreeNodeScript*>(node);
        if (script->getOrigin() != ScriptOrigin::SCRIPT_ORIGIN_LOCAL) {
            continue;
        }
        QVariantMap resultNode;
        resultNode.insert("name", node->getName());
        resultNode.insert("path", script->getFullPath());
        result.append(resultNode);
    }
    return result;
}
