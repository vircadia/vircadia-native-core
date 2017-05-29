//
//  FingerprintUtils.h
//  libraries/networking/src
//
//  Created by David Kelly on 2016-12-02.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "FingerprintUtils.h"
#include "NetworkLogging.h"

#include <QDebug>

#include <SettingHandle.h>
#include <SettingManager.h>
#include <DependencyManager.h>

#ifdef Q_OS_WIN
#include <Windows.h>
#include <winreg.h>
#endif //Q_OS_WIN

#ifdef Q_OS_MAC
#include <IOKit/IOBSD.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/storage/IOMedia.h>
#endif //Q_OS_MAC

static const QString FALLBACK_FINGERPRINT_KEY = "fallbackFingerprint";

QUuid FingerprintUtils::_machineFingerprint { QUuid() };

QString FingerprintUtils::getMachineFingerprintString() {
    QString uuidString;
#ifdef Q_OS_LINUX
    // sadly need to be root to get smbios guid from linux, so 
    // for now lets do nothing.
#endif //Q_OS_LINUX

#ifdef Q_OS_MAC
    io_registry_entry_t ioRegistryRoot = IORegistryEntryFromPath(kIOMasterPortDefault, "IOService:/");
    CFStringRef uuidCf = (CFStringRef) IORegistryEntryCreateCFProperty(ioRegistryRoot, CFSTR(kIOPlatformUUIDKey), kCFAllocatorDefault, 0);
    IOObjectRelease(ioRegistryRoot);
    uuidString = QString::fromCFString(uuidCf);
    CFRelease(uuidCf); 
    qCDebug(networking) << "Mac serial number: " << uuidString;
#endif //Q_OS_MAC

#ifdef Q_OS_WIN
    HKEY cryptoKey;
    
    // try and open the key that contains the machine GUID
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Cryptography", 0, KEY_READ, &cryptoKey) == ERROR_SUCCESS) {
        DWORD type;
        DWORD guidSize;

        const char* MACHINE_GUID_KEY = "MachineGuid";

        // try and retrieve the size of the GUID value
        if (RegQueryValueEx(cryptoKey, MACHINE_GUID_KEY, NULL, &type, NULL, &guidSize) == ERROR_SUCCESS) {
            // make sure that the value is a string
            if (type == REG_SZ) {
                // retrieve the machine GUID and return that as our UUID string
                std::string machineGUID(guidSize / sizeof(char), '\0');

                if (RegQueryValueEx(cryptoKey, MACHINE_GUID_KEY, NULL, NULL,
                                    reinterpret_cast<LPBYTE>(&machineGUID[0]), &guidSize) == ERROR_SUCCESS) {
                    uuidString = QString::fromStdString(machineGUID);
                }
            }
        }

        RegCloseKey(cryptoKey);
    }

#endif //Q_OS_WIN

    return uuidString;

}

QUuid FingerprintUtils::getMachineFingerprint() {

    if (_machineFingerprint.isNull()) {
        QString uuidString = getMachineFingerprintString();

        // now, turn into uuid.  A malformed string will
        // return QUuid() ("{00000...}"), which handles
        // any errors in getting the string
        QUuid uuid(uuidString);

        if (uuid == QUuid()) {
            // if you cannot read a fallback key cuz we aren't saving them, just generate one for
            // this session and move on
            if (DependencyManager::get<Setting::Manager>().isNull()) {
                return QUuid::createUuid();
            }
            // read fallback key (if any)
            Settings settings;
            uuid = QUuid(settings.value(FALLBACK_FINGERPRINT_KEY).toString());
            qCDebug(networking) << "read fallback maching fingerprint: " << uuid.toString();

            if (uuid == QUuid()) {
                // no fallback yet, set one
                uuid = QUuid::createUuid();
                settings.setValue(FALLBACK_FINGERPRINT_KEY, uuid.toString());
                qCDebug(networking) << "no fallback machine fingerprint, setting it to: " << uuid.toString();
            }
        }

        _machineFingerprint = uuid;
    }

    return _machineFingerprint;
}

