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
#include <QFile>
#include <QCryptographicHash>
#include <QDirIterator>

#ifdef Q_OS_WIN
#include <Windows.h>
#include <winreg.h>
#endif //Q_OS_WIN

#ifdef Q_OS_MAC
#include <IOKit/IOBSD.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/storage/IOMedia.h>
#endif //Q_OS_MAC

// Number of iterations to apply to the hash, for stretching.
// The number is arbitrary and has the only purpose of slowing down brute-force
// attempts. The number here should be low enough not to cause any trouble for
// low-end hardware.
//
// Changing this results in different hardware IDs being computed.
static const int HASH_ITERATIONS = 65535;

// Salt string for the hardware ID, makes our IDs unique to our project.
// Changing this results in different hardware IDs being computed.
static const QByteArray HASH_SALT{"Vircadia"};

static const QString FALLBACK_FINGERPRINT_KEY = "fallbackFingerprint";

QUuid FingerprintUtils::_machineFingerprint { QUuid() };

QString FingerprintUtils::getMachineFingerprintString() {
    QCryptographicHash hash(QCryptographicHash::Keccak_256);

#ifdef Q_OS_LINUX
    // As per the documentation:
    // https://man7.org/linux/man-pages/man5/machine-id.5.html
    //
    // we use the machine id as a base, but add an application-specific key to it.
    // If machine-id isn't available, we try hardware networking devices instead.

    QFile machineIdFile("/etc/machine-id");
    if (!machineIdFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // No machine ID, probably no systemd.
        qCWarning(networking) << "Failed to open /etc/machine-id";

        QDir netDevicesDir("/sys/class/net");
        QFileInfoList netDevicesInfo = netDevicesDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

        if (netDevicesInfo.empty()) {
            // Let getMachineFingerprint handle this contingency
            qCWarning(networking) << "Failed to find any hardware networking devices";
            return QUuid().toString();
        }

        for(auto& fileInfo : netDevicesInfo) {

            if (fileInfo.isSymLink() && fileInfo.symLinkTarget().contains("virtual")) {
                // symlink points to something like:
                // ../../devices/virtual/net/lo
                // these are not real devices and have random IDs, so we
                // don't care about them.
                continue;
            }

            QFile addressFile(fileInfo.filePath() + "/address");
            if (addressFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                qCDebug(networking) << "Adding contents of " << addressFile.fileName();
                hash.addData(addressFile.readAll());
            } else {
                qCWarning(networking) << "Failed to read " << addressFile.fileName();
            }
        }
    } else {
        QByteArray data = machineIdFile.readAll();
        hash.addData(data);
    }

#endif //Q_OS_LINUX

#ifdef Q_OS_MAC
    io_registry_entry_t ioRegistryRoot = IORegistryEntryFromPath(kIOMasterPortDefault, "IOService:/");
    CFStringRef uuidCf = (CFStringRef) IORegistryEntryCreateCFProperty(ioRegistryRoot, CFSTR(kIOPlatformUUIDKey), kCFAllocatorDefault, 0);
    IOObjectRelease(ioRegistryRoot);
    hash.addData(QString::fromCFString(uuidCf).toUtf8());
    CFRelease(uuidCf); 
#endif //Q_OS_MAC

#ifdef Q_OS_WIN
    HKEY cryptoKey;
    bool success = false;

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
                    hash.addData(QString::fromStdString(machineGUID).toUtf8());
                    success = true;
                }
            }
        }

        RegCloseKey(cryptoKey);
    }

    if (!success) {
        // Let getMachineFingerprint handle this contingency
        return QUuid().toString();
    }
#endif //Q_OS_WIN

    // Makes this hash unique to us
    hash.addData(HASH_SALT);

    // Stretching
    for (int i = 0; i < HASH_ITERATIONS; i++) {
        hash.addData(hash.result());
    }

    QByteArray result = hash.result();
    result.resize(128 / 8); // GUIDs are 128 bit numbers

    // Set UUID version to 4, ensuring it's a valid UUID
    result[6] = (result[6] & 0x0F) | 0x40;

    // Of course, Qt couldn't be nice and just parse something like:
    // 1b1b9d6d45c2473bac13dc3011ff58d6
    //
    // So we have to turn it into:
    // {1b1b9d6d-45c2-473b-ac13-dc3011ff58d6}

    QString uuidString = result.toHex();
    uuidString.insert(20, '-');
    uuidString.insert(16, '-');
    uuidString.insert(12, '-');
    uuidString.insert(8, '-');
    uuidString.prepend("{");
    uuidString.append("}");

    qCDebug(networking) << "Final machine fingerprint:" << uuidString;

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

