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

QmlCommerce::QmlCommerce() :
    _appsPath(PathUtils::getAppDataPath() + "Apps/")
{
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
    connect(ledger.data(), &Ledger::transferAssetToNodeResult, this, &QmlCommerce::transferAssetToNodeResult);
    connect(ledger.data(), &Ledger::transferAssetToUsernameResult, this, &QmlCommerce::transferAssetToUsernameResult);
    connect(ledger.data(), &Ledger::authorizeAssetTransferResult, this, &QmlCommerce::authorizeAssetTransferResult);
    connect(ledger.data(), &Ledger::availableUpdatesResult, this, &QmlCommerce::availableUpdatesResult);
    connect(ledger.data(), &Ledger::updateItemResult, this, &QmlCommerce::updateItemResult);

    auto accountManager = DependencyManager::get<AccountManager>();
    connect(accountManager.data(), &AccountManager::usernameChanged, this, [&]() { setPassphrase(""); });
}


void QmlCommerce::openSystemApp(const QString& appName) {
    static const QMap<QString, QString> systemApps {
        {"GOTO",        "hifi/tablet/TabletAddressDialog.qml"},
        {"PEOPLE",      "hifi/Pal.qml"},
        {"WALLET",      "hifi/commerce/wallet/Wallet.qml"},
        {"MARKET",      "hifi/commerce/marketplace/Marketplace.qml"}
    };

    static const QMap<QString, QString> systemInject{
    };


    auto tablet = dynamic_cast<TabletProxy*>(
        DependencyManager::get<TabletScriptingInterface>()->getTablet("com.highfidelity.interface.tablet.system"));

    QMap<QString, QString>::const_iterator appPathIter = systemApps.find(appName);
    if (appPathIter != systemApps.end()) {
        if (appPathIter->contains(".qml", Qt::CaseInsensitive)) {
            tablet->loadQMLSource(*appPathIter);
        }
        else if (appPathIter->contains(".html", Qt::CaseInsensitive)) {
            QMap<QString, QString>::const_iterator injectIter = systemInject.find(appName);
            if (appPathIter == systemInject.end()) {
                tablet->gotoWebScreen(MetaverseAPI::getCurrentMetaverseServerURL().toString() + *appPathIter);
            }
            else {
                QString inject = "file:///" + qApp->applicationDirPath() + *injectIter;
                tablet->gotoWebScreen(MetaverseAPI::getCurrentMetaverseServerURL().toString() + *appPathIter, inject);
            }
        }
        else {
            qCDebug(commerce) << "Attempted to open unknown type of URL!";
            return;
        }
    }
    else {
        qCDebug(commerce) << "Attempted to open unknown APP!";
        return;
    }

    DependencyManager::get<HMDScriptingInterface>()->openTablet();
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

bool QmlCommerce::copyKeyFileFrom(const QString& pathname) {
    auto wallet = DependencyManager::get<Wallet>();
    return wallet->copyKeyFileFrom(pathname);
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

void QmlCommerce::inventory(const QString& editionFilter,
                            const QString& typeFilter,
                            const QString& titleFilter,
                            const int& page,
                            const int& perPage) {
    auto ledger = DependencyManager::get<Ledger>();
    auto wallet = DependencyManager::get<Wallet>();
    QStringList cachedPublicKeys = wallet->listPublicKeys();
    if (!cachedPublicKeys.isEmpty()) {
        ledger->inventory(editionFilter, typeFilter, titleFilter, page, perPage);
    }
}

void QmlCommerce::history(const int& pageNumber, const int& itemsPerPage) {
    auto ledger = DependencyManager::get<Ledger>();
    auto wallet = DependencyManager::get<Wallet>();
    QStringList cachedPublicKeys = wallet->listPublicKeys();
    if (!cachedPublicKeys.isEmpty()) {
        ledger->history(cachedPublicKeys, pageNumber, itemsPerPage);
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

void QmlCommerce::setSoftReset() {
    auto wallet = DependencyManager::get<Wallet>();
    wallet->setSoftReset();
}

void QmlCommerce::clearWallet() {
    auto wallet = DependencyManager::get<Wallet>();
    wallet->clear();
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

void QmlCommerce::transferAssetToNode(const QString& nodeID,
                                      const QString& certificateID,
                                      const int& amount,
                                      const QString& optionalMessage) {
    auto ledger = DependencyManager::get<Ledger>();
    auto wallet = DependencyManager::get<Wallet>();
    QStringList keys = wallet->listPublicKeys();
    if (keys.count() == 0) {
        QJsonObject result{ { "status", "fail" }, { "message", "Uninitialized Wallet." } };
        return emit transferAssetToNodeResult(result);
    }
    QString key = keys[0];
    ledger->transferAssetToNode(key, nodeID, certificateID, amount, optionalMessage);
}

void QmlCommerce::transferAssetToUsername(const QString& username,
                                          const QString& certificateID,
                                          const int& amount,
                                          const QString& optionalMessage) {
    auto ledger = DependencyManager::get<Ledger>();
    auto wallet = DependencyManager::get<Wallet>();
    QStringList keys = wallet->listPublicKeys();
    if (keys.count() == 0) {
        QJsonObject result{ { "status", "fail" }, { "message", "Uninitialized Wallet." } };
        return emit transferAssetToUsernameResult(result);
    }
    QString key = keys[0];
    ledger->transferAssetToUsername(key, username, certificateID, amount, optionalMessage);
}

void QmlCommerce::authorizeAssetTransfer(const QString& couponID,
    const QString& certificateID,
    const int& amount,
    const QString& optionalMessage) {
    auto ledger = DependencyManager::get<Ledger>();
    auto wallet = DependencyManager::get<Wallet>();
    QStringList keys = wallet->listPublicKeys();
    if (keys.count() == 0) {
        QJsonObject result{ { "status", "fail" }, { "message", "Uninitialized Wallet." } };
        return emit authorizeAssetTransferResult(result);
    }
    QString key = keys[0];
    ledger->authorizeAssetTransfer(key, couponID, certificateID, amount, optionalMessage);
}

void QmlCommerce::replaceContentSet(const QString& itemHref, const QString& certificateID, const QString& itemName) {
    if (!certificateID.isEmpty()) {
        auto ledger = DependencyManager::get<Ledger>();
        ledger->updateLocation(
            certificateID,
            DependencyManager::get<AddressManager>()->getPlaceName(),
            true);
    }
    qApp->replaceDomainContent(itemHref, itemName);
    QJsonObject messageProperties = {
        { "status", "SuccessfulRequestToReplaceContent" },
        { "content_set_url", itemHref } };
    UserActivityLogger::getInstance().logAction("replace_domain_content", messageProperties);
    emit contentSetChanged(itemHref);
}

void QmlCommerce::alreadyOwned(const QString& marketplaceId) {
    auto ledger = DependencyManager::get<Ledger>();
    ledger->alreadyOwned(marketplaceId);
}

QString QmlCommerce::getInstalledApps(const QString& justInstalledAppID) {
    QString installedAppsFromMarketplace;
    QStringList runningScripts = DependencyManager::get<ScriptEngines>()->getRunningScripts();

    QDir directory(_appsPath);
    QStringList apps = directory.entryList(QStringList("*.app.json"));
    foreach (QString appFileName, apps) {
        // If we were supplied a "justInstalledAppID" argument, that means we're entering this function
        // to get the new list of installed apps immediately after installing an app.
        // In that case, the app we installed may not yet have its associated script running -
        // that task is asynchronous and takes a nonzero amount of time. This is especially true
        // for apps that are not in Interface's script cache.
        // Thus, we protect against deleting the .app.json from the user's disk (below)
        // by skipping that check for the app we just installed.
        if ((justInstalledAppID != "") && ((justInstalledAppID + ".app.json") == appFileName)) {
            installedAppsFromMarketplace += appFileName + ",";
            continue;
        }

        QFile appFile(_appsPath + appFileName);
        if (appFile.open(QIODevice::ReadOnly)) {
            QJsonDocument appFileJsonDocument = QJsonDocument::fromJson(appFile.readAll());

            appFile.close();

            QJsonObject appFileJsonObject = appFileJsonDocument.object();
            QString scriptURL = appFileJsonObject["scriptURL"].toString();

            // If the script .app.json is on the user's local disk but the associated script isn't running
            // for some reason (i.e. the user stopped it from Running Scripts),
            // delete the .app.json from the user's local disk.
            if (!runningScripts.contains(scriptURL)) {
                if (!appFile.remove()) {
                    qCWarning(commerce) << "Couldn't delete local .app.json file (app's script isn't running). App filename is:"
                                        << appFileName;
                }
            } else {
                installedAppsFromMarketplace += appFileName;
                installedAppsFromMarketplace += ",";
            }
        } else {
            qCDebug(commerce) << "Couldn't open local .app.json file for reading.";
        }
    }

    return installedAppsFromMarketplace;
}

bool QmlCommerce::installApp(const QString& itemHref, const bool& alsoOpenImmediately) {
    if (!QDir(_appsPath).exists()) {
        if (!QDir().mkdir(_appsPath)) {
            qCDebug(commerce) << "Couldn't make _appsPath directory.";
            return false;
        }
    }

    QUrl appHref(itemHref);

    auto request =
        DependencyManager::get<ResourceManager>()->createResourceRequest(this, appHref, true, -1, "QmlCommerce::installApp");

    if (!request) {
        qCDebug(commerce) << "Couldn't create resource request for app.";
        return false;
    }

    connect(request, &ResourceRequest::finished, this, [=]() {
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

        // Don't try to re-load (install) a script if it's already running
        QStringList runningScripts = DependencyManager::get<ScriptEngines>()->getRunningScripts();
        if (!runningScripts.contains(scriptUrl)) {
            if ((DependencyManager::get<ScriptEngines>()->loadScript(scriptUrl.trimmed())).isNull()) {
                qCDebug(commerce) << "Couldn't load script.";
                return false;
            }

            QFileInfo appFileInfo(appFile);
            emit appInstalled(appFileInfo.baseName());
        }

        if (alsoOpenImmediately) {
            QmlCommerce::openApp(itemHref);
        }

        return true;
    });
    request->send();
    return true;
}

bool QmlCommerce::uninstallApp(const QString& itemHref) {
    QUrl appHref(itemHref);

    // Read from the file to know what .js script to stop
    QFile appFile(_appsPath + "/" + appHref.fileName());
    if (!appFile.open(QIODevice::ReadOnly)) {
        qCDebug(commerce)
            << "Couldn't open local .app.json file for deletion. Cannot continue with app uninstallation. App filename is:"
            << appHref.fileName();
        return false;
    }
    QJsonDocument appFileJsonDocument = QJsonDocument::fromJson(appFile.readAll());
    QJsonObject appFileJsonObject = appFileJsonDocument.object();
    QString scriptUrl = appFileJsonObject["scriptURL"].toString();

    if (!DependencyManager::get<ScriptEngines>()->stopScript(scriptUrl.trimmed(), false)) {
        qCWarning(commerce) << "Couldn't stop script during app uninstall. Continuing anyway.";
    }

    // Delete the .app.json from the filesystem
    // remove() closes the file first.
    if (!appFile.remove()) {
        qCWarning(commerce) << "Couldn't delete local .app.json file during app uninstall. Continuing anyway. App filename is:"
                            << appHref.fileName();
    }

    QFileInfo appFileInfo(appFile);
    emit appUninstalled(appFileInfo.baseName());
    return true;
}

bool QmlCommerce::openApp(const QString& itemHref) {
    QUrl appHref(itemHref);

    // Read from the file to know what .html or .qml document to open
    QFile appFile(_appsPath + "/" + appHref.fileName());
    if (!appFile.open(QIODevice::ReadOnly)) {
        qCDebug(commerce) << "Couldn't open local .app.json file:" << appFile;
        return false;
    }
    QJsonDocument appFileJsonDocument = QJsonDocument::fromJson(appFile.readAll());
    QJsonObject appFileJsonObject = appFileJsonDocument.object();
    QString homeUrl = appFileJsonObject["homeURL"].toString();

    auto tablet = dynamic_cast<TabletProxy*>(
        DependencyManager::get<TabletScriptingInterface>()->getTablet("com.highfidelity.interface.tablet.system"));
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

void QmlCommerce::getAvailableUpdates(const QString& itemId, const int& pageNumber, const int& itemsPerPage) {
    auto ledger = DependencyManager::get<Ledger>();
    ledger->getAvailableUpdates(itemId, pageNumber, itemsPerPage);
}

void QmlCommerce::updateItem(const QString& certificateId) {
    auto ledger = DependencyManager::get<Ledger>();
    auto wallet = DependencyManager::get<Wallet>();
    QStringList keys = wallet->listPublicKeys();
    if (keys.count() == 0) {
        QJsonObject result{ { "status", "fail" }, { "message", "Uninitialized Wallet." } };
        return emit updateItemResult(result);
    }
    QString key = keys[0];
    ledger->updateItem(key, certificateId);
}
