//
//  QmlCommerce.cpp
//  interface/src/commerce
//
//  Created by Howard Stearns on 8/4/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "QmlCommerce.h"
#include "CommerceLogging.h"
#include "Application.h"
#include "DependencyManager.h"
#include "Ledger.h"
#include "Wallet.h"
#include <AccountManager.h>
#include <Application.h>
#include <UserActivityLogger.h>
#include <ScriptEngines.h>
#include <ui/TabletScriptingInterface.h>
#include "scripting/HMDScriptingInterface.h"

QmlCommerce::QmlCommerce() {
    auto ledger = DependencyManager::get<Ledger>();
    auto wallet = DependencyManager::get<Wallet>();
    connect(ledger.data(), &Ledger::buyResult, this, &QmlCommerce::buyResult);
    connect(ledger.data(), &Ledger::balanceResult, this, &QmlCommerce::balanceResult);
    connect(ledger.data(), &Ledger::inventoryResult, this, &QmlCommerce::inventoryResult);
    connect(wallet.data(), &Wallet::securityImageResult, this, &QmlCommerce::securityImageResult);
    connect(ledger.data(), &Ledger::historyResult, this, &QmlCommerce::historyResult);
    connect(wallet.data(), &Wallet::keyFilePathIfExistsResult, this, &QmlCommerce::keyFilePathIfExistsResult);
    connect(ledger.data(), &Ledger::accountResult, this, &QmlCommerce::accountResult);
    connect(wallet.data(), &Wallet::walletStatusResult, this, &QmlCommerce::walletStatusResult);
    connect(ledger.data(), &Ledger::certificateInfoResult, this, &QmlCommerce::certificateInfoResult);
    connect(ledger.data(), &Ledger::alreadyOwnedResult, this, &QmlCommerce::alreadyOwnedResult);
    connect(ledger.data(), &Ledger::updateCertificateStatus, this, &QmlCommerce::updateCertificateStatus);
    connect(ledger.data(), &Ledger::transferHfcToNodeResult, this, &QmlCommerce::transferHfcToNodeResult);
    connect(ledger.data(), &Ledger::transferHfcToUsernameResult, this, &QmlCommerce::transferHfcToUsernameResult);
    connect(ledger.data(), &Ledger::transferHfcToUsernameResult, this, &QmlCommerce::transferHfcToUsernameResult);
    
    auto accountManager = DependencyManager::get<AccountManager>();
    connect(accountManager.data(), &AccountManager::usernameChanged, this, [&]() {
        setPassphrase("");
    });

    _appsPath = PathUtils::getAppDataPath() + "Apps";
}

void QmlCommerce::getWalletStatus() {
    auto wallet = DependencyManager::get<Wallet>();
    wallet->getWalletStatus();
}

void QmlCommerce::getLoginStatus() {
    emit loginStatusResult(DependencyManager::get<AccountManager>()->isLoggedIn());
}

void QmlCommerce::getKeyFilePathIfExists() {
    auto wallet = DependencyManager::get<Wallet>();
    emit keyFilePathIfExistsResult(wallet->getKeyFilePath());
}

void QmlCommerce::getWalletAuthenticatedStatus() {
    auto wallet = DependencyManager::get<Wallet>();
    emit walletAuthenticatedStatusResult(wallet->walletIsAuthenticatedWithPassphrase());
}

void QmlCommerce::getSecurityImage() {
    auto wallet = DependencyManager::get<Wallet>();
    wallet->getSecurityImage();
}

void QmlCommerce::chooseSecurityImage(const QString& imageFile) {
    auto wallet = DependencyManager::get<Wallet>();
    wallet->chooseSecurityImage(imageFile);
}

void QmlCommerce::buy(const QString& assetId, int cost, const bool controlledFailure) {
    auto ledger = DependencyManager::get<Ledger>();
    auto wallet = DependencyManager::get<Wallet>();
    QStringList keys = wallet->listPublicKeys();
    if (keys.count() == 0) {
        QJsonObject result{ { "status", "fail" }, { "message", "Uninitialized Wallet." } };
        return emit buyResult(result);
    }
    QString key = keys[0];
    // For now, we receive at the same key that pays for it.
    ledger->buy(key, cost, assetId, key, controlledFailure);
}

void QmlCommerce::balance() {
    auto ledger = DependencyManager::get<Ledger>();
    auto wallet = DependencyManager::get<Wallet>();
    QStringList cachedPublicKeys = wallet->listPublicKeys();
    if (!cachedPublicKeys.isEmpty()) {
        ledger->balance(cachedPublicKeys);
    }
}

void QmlCommerce::inventory() {
    auto ledger = DependencyManager::get<Ledger>();
    auto wallet = DependencyManager::get<Wallet>();
    QStringList cachedPublicKeys = wallet->listPublicKeys();
    if (!cachedPublicKeys.isEmpty()) {
        ledger->inventory(cachedPublicKeys);
    }
}

void QmlCommerce::history(const int& pageNumber) {
    auto ledger = DependencyManager::get<Ledger>();
    auto wallet = DependencyManager::get<Wallet>();
    QStringList cachedPublicKeys = wallet->listPublicKeys();
    if (!cachedPublicKeys.isEmpty()) {
        ledger->history(cachedPublicKeys, pageNumber);
    }
}

void QmlCommerce::changePassphrase(const QString& oldPassphrase, const QString& newPassphrase) {
    auto wallet = DependencyManager::get<Wallet>();
    if (wallet->getPassphrase()->isEmpty()) {
        emit changePassphraseStatusResult(wallet->setPassphrase(newPassphrase));
    } else if (wallet->getPassphrase() == oldPassphrase && !newPassphrase.isEmpty()) {
        emit changePassphraseStatusResult(wallet->changePassphrase(newPassphrase));
    } else {
        emit changePassphraseStatusResult(false);
    }
}

void QmlCommerce::setPassphrase(const QString& passphrase) {
    auto wallet = DependencyManager::get<Wallet>();
    wallet->setPassphrase(passphrase);
    getWalletAuthenticatedStatus();
}

void QmlCommerce::generateKeyPair() {
    auto wallet = DependencyManager::get<Wallet>();
    wallet->generateKeyPair();
    getWalletAuthenticatedStatus();
}

void QmlCommerce::account() {
    auto ledger = DependencyManager::get<Ledger>();
    ledger->account();
}

void QmlCommerce::certificateInfo(const QString& certificateId) {
    auto ledger = DependencyManager::get<Ledger>();
    ledger->certificateInfo(certificateId);
}

void QmlCommerce::transferHfcToNode(const QString& nodeID, const int& amount, const QString& optionalMessage) {
    auto ledger = DependencyManager::get<Ledger>();
    auto wallet = DependencyManager::get<Wallet>();
    QStringList keys = wallet->listPublicKeys();
    if (keys.count() == 0) {
        QJsonObject result{ { "status", "fail" },{ "message", "Uninitialized Wallet." } };
        return emit buyResult(result);
    }
    QString key = keys[0];
    ledger->transferHfcToNode(key, nodeID, amount, optionalMessage);
}

void QmlCommerce::transferHfcToUsername(const QString& username, const int& amount, const QString& optionalMessage) {
    auto ledger = DependencyManager::get<Ledger>();
    auto wallet = DependencyManager::get<Wallet>();
    QStringList keys = wallet->listPublicKeys();
    if (keys.count() == 0) {
        QJsonObject result{ { "status", "fail" },{ "message", "Uninitialized Wallet." } };
        return emit buyResult(result);
    }
    QString key = keys[0];
    ledger->transferHfcToUsername(key, username, amount, optionalMessage);
}

void QmlCommerce::replaceContentSet(const QString& itemHref) {
    qApp->replaceDomainContent(itemHref);
    QJsonObject messageProperties = {
        { "status", "SuccessfulRequestToReplaceContent" },
        { "content_set_url", itemHref }
    };
    UserActivityLogger::getInstance().logAction("replace_domain_content", messageProperties);

    emit contentSetChanged(itemHref);
}

void QmlCommerce::alreadyOwned(const QString& marketplaceId) {
    auto ledger = DependencyManager::get<Ledger>();
    ledger->alreadyOwned(marketplaceId);
}

bool QmlCommerce::isAppInstalled(const QString& itemHref) {
    QUrl appHref(itemHref);

    // First check if .app.json exists
    QFileInfo appFile(_appsPath + "/" + appHref.fileName());
    if (!(appFile.exists() && appFile.isFile())) {
        return false;
    }

    // Then check to see if script is running
    auto runningScripts = DependencyManager::get<ScriptEngines>()->getRunningScripts();
    foreach(const QString& runningScript, runningScripts) {
        QUrl runningScriptURL = QUrl(runningScript);
        qCDebug(commerce) << "ZRF FIXME" << runningScriptURL;
        if (runningScriptURL == appHref) {
            return true;
        }
    }

    return false;
}

bool QmlCommerce::installApp(const QString& itemHref) {
    if (!QDir(_appsPath).exists()) {
        if (!QDir().mkdir(_appsPath)) {
            qCDebug(commerce) << "Couldn't make _appsPath directory.";
            return false;
        }
    }

    QUrl appHref(itemHref);

    auto request =
        std::unique_ptr<ResourceRequest>(DependencyManager::get<ResourceManager>()->createResourceRequest(this, appHref));

    if (!request) {
        qCDebug(commerce) << "Couldn't create resource request for app.";
        return false;
    }

    QEventLoop loop;
    connect(request.get(), &ResourceRequest::finished, &loop, &QEventLoop::quit);
    request->send();
    loop.exec();

    if (request->getResult() != ResourceRequest::Success) {
        qCDebug(commerce) << "Failed to get .app.json file from remote.";
        return false;
    }

    // Copy the .app.json to the apps directory inside %AppData%/High Fidelity/Interface
    auto requestData = request->getData();
    QFile appFile(_appsPath + "/" + appHref.fileName());
    if (!appFile.open(QIODevice::WriteOnly)) {
        qCDebug(commerce) << "Couldn't open local .app.json file for creation.";
        return false;
    }
    if (appFile.write(requestData) == -1) {
        qCDebug(commerce) << "Couldn't write to local .app.json file.";
        return false;
    }
    // Close the file
    appFile.close();

    // Read from the returned datastream to know what .js to add to Running Scripts
    QJsonDocument appFileJsonDocument = QJsonDocument::fromJson(requestData);
    QJsonObject appFileJsonObject = appFileJsonDocument.object();
    QString scriptUrl = appFileJsonObject["scriptURL"].toString();

    if ((DependencyManager::get<ScriptEngines>()->loadScript(scriptUrl.trimmed())).isNull()) {
        qCDebug(commerce) << "Couldn't load script.";
        return false;
    }

    emit appInstalled(itemHref);
    return true;
}

bool QmlCommerce::uninstallApp(const QString& itemHref) {
    QUrl appHref(itemHref);

    // Read from the file to know what .js script to stop
    QFile appFile(_appsPath + "/" + appHref.fileName());
    if (!appFile.open(QIODevice::ReadOnly)) {
        qCDebug(commerce) << "Couldn't open local .app.json file for deletion.";
        return false;
    }
    QJsonDocument appFileJsonDocument = QJsonDocument::fromJson(appFile.readAll());
    QJsonObject appFileJsonObject = appFileJsonDocument.object();
    QString scriptUrl = appFileJsonObject["scriptURL"].toString();

    if (!DependencyManager::get<ScriptEngines>()->stopScript(scriptUrl.trimmed(), false)) {
        qCDebug(commerce) << "Couldn't stop script.";
        return false;
    }

    // Delete the .app.json from the filesystem
    // remove() closes the file first.
    if (!appFile.remove()) {
        qCDebug(commerce) << "Couldn't delete local .app.json file.";
        return false;
    }

    emit appUninstalled(itemHref);
    return true;
}

bool QmlCommerce::openApp(const QString& itemHref) {
    QUrl appHref(itemHref);

    // Read from the file to know what .html or .qml document to open
    QFile appFile(_appsPath + "/" + appHref.fileName());
    if (!appFile.open(QIODevice::ReadOnly)) {
        qCDebug(commerce) << "Couldn't open local .app.json file.";
        return false;
    }
    QJsonDocument appFileJsonDocument = QJsonDocument::fromJson(appFile.readAll());
    QJsonObject appFileJsonObject = appFileJsonDocument.object();
    QString homeUrl = appFileJsonObject["homeURL"].toString();

    auto tabletScriptingInterface = DependencyManager::get<TabletScriptingInterface>();
    auto tablet = dynamic_cast<TabletProxy*>(tabletScriptingInterface->getTablet("com.highfidelity.interface.tablet.system"));
    if (homeUrl.contains(".qml", Qt::CaseInsensitive)) {
        tablet->loadQMLSource(homeUrl);
    } else if (homeUrl.contains(".html", Qt::CaseInsensitive)) {
        tablet->gotoWebScreen(homeUrl);
    } else {
        qCDebug(commerce) << "Attempted to open unknown type of homeURL!";
        return false;
    }

    DependencyManager::get<HMDScriptingInterface>()->openTablet();

    return true;
}
