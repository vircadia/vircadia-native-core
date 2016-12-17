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

#include <QDebug>

#include <SettingHandle.h>
#include <SettingManager.h>
#include <DependencyManager.h>

#ifdef Q_OS_WIN
#include <comdef.h>
#include <Wbemidl.h>
#endif //Q_OS_WIN

#ifdef Q_OS_MAC
#include <IOKit/IOBSD.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/storage/IOMedia.h>
#endif //Q_OS_MAC

static const QString FALLBACK_FINGERPRINT_KEY = "fallbackFingerprint";
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
    qDebug() << "Mac serial number: " << uuidString;
#endif //Q_OS_MAC

#ifdef Q_OS_WIN
    HRESULT hres;
    IWbemLocator *pLoc = NULL;

    // initialize com.  Interface already does, but other
    // users of this lib don't necessarily do so.
    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres)) {
        qDebug() << "Failed to initialize COM library!";
        return uuidString;
    }
    
    // initialize WbemLocator
    hres = CoCreateInstance(
            CLSID_WbemLocator,             
            0, 
            CLSCTX_INPROC_SERVER, 
            IID_IWbemLocator, (LPVOID *) &pLoc);

    if (FAILED(hres)) {
        qDebug() << "Failed to initialize WbemLocator";
        return uuidString;
    }
    
    // Connect to WMI through the IWbemLocator::ConnectServer method
    IWbemServices *pSvc = NULL;

    // Connect to the root\cimv2 namespace with
    // the current user and obtain pointer pSvc
    // to make IWbemServices calls.
    hres = pLoc->ConnectServer(
         _bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
         NULL,                    // User name. NULL = current user
         NULL,                    // User password. NULL = current
         0,                       // Locale. NULL indicates current
         NULL,                    // Security flags.
         0,                       // Authority (for example, Kerberos)
         0,                       // Context object 
         &pSvc                    // pointer to IWbemServices proxy
         );

    if (FAILED(hres)) {
        pLoc->Release();
        qDebug() << "Failed to connect to WMI";
        return uuidString;
    }
    
    // Set security levels on the proxy
    hres = CoSetProxyBlanket(
       pSvc,                        // Indicates the proxy to set
       RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
       RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
       NULL,                        // Server principal name 
       RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx 
       RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
       NULL,                        // client identity
       EOAC_NONE                    // proxy capabilities 
    );

    if (FAILED(hres)) {
        pSvc->Release();
        pLoc->Release();     
        qDebug() << "Failed to set security on proxy blanket";
        return uuidString;
    }
    
    // Use the IWbemServices pointer to grab the Win32_BIOS stuff
    IEnumWbemClassObject* pEnumerator = NULL;
    hres = pSvc->ExecQuery(
        bstr_t("WQL"), 
        bstr_t("SELECT * FROM Win32_ComputerSystemProduct"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 
        NULL,
        &pEnumerator);

    if (FAILED(hres)) {
        pSvc->Release();
        pLoc->Release();
        qDebug() << "query to get Win32_ComputerSystemProduct info";
        return uuidString;
    }
    
    // Get the SerialNumber from the Win32_BIOS data 
    IWbemClassObject *pclsObj;
    ULONG uReturn = 0;

    SHORT  sRetStatus = -100;

    while (pEnumerator) {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

        if(0 == uReturn){
            break;
        }

        VARIANT vtProp;

        // Get the value of the Name property
        hr = pclsObj->Get(L"UUID", 0, &vtProp, 0, 0);
        if (!FAILED(hres)) {
            switch (vtProp.vt) {
                case VT_BSTR:
                    uuidString = QString::fromWCharArray(vtProp.bstrVal);
                    break;
            }
        }
        VariantClear(&vtProp);

        pclsObj->Release();
    }
    pEnumerator->Release();

    // Cleanup
    pSvc->Release();
    pLoc->Release();
    
    qDebug() << "Windows BIOS UUID: " << uuidString;
#endif //Q_OS_WIN

    return uuidString;

}

QUuid FingerprintUtils::getMachineFingerprint() {

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
        qDebug() << "read fallback maching fingerprint: " << uuid.toString();
        if (uuid == QUuid()) {
            // no fallback yet, set one
            uuid = QUuid::createUuid();
            settings.setValue(FALLBACK_FINGERPRINT_KEY, uuid.toString());
            qDebug() << "no fallback machine fingerprint, setting it to: " << uuid.toString();
        }
    }
    return uuid;
}

