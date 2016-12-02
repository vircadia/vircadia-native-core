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

QString FingerprintUtils::getMachineFingerprint() {
    QString retval;
#ifdef Q_OS_WIN
    HRESULT hres;
    IWbemLocator *pLoc = NULL;

    // initialize com
    hres = CoCreateInstance(
            CLSID_WbemLocator,             
            0, 
            CLSCTX_INPROC_SERVER, 
            IID_IWbemLocator, (LPVOID *) &pLoc);

    if (FAILED(hres)) {
        qDebug() << "Failed to initialize WbemLocator";
        return retval;
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
        return retval;
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
        return retval;
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
        return retval;
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
                    retval = QString::fromWCharArray(vtProp.bstrVal);
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
    
    qDebug() << "Windows BIOS UUID: " << retval;
#endif //Q_OS_WIN

#ifdef Q_OS_MAC

    io_registry_entry_t ioRegistryRoot = IORegistryEntryFromPath(kIOMasterPortDefault, "IOService:/");
    CFStringRef uuidCf = (CFStringRef) IORegistryEntryCreateCFProperty(ioRegistryRoot, CFSTR(kIOPlatformUUIDKey), kCFAllocatorDefault, 0);
    IOObjectRelease(ioRegistryRoot);
    retval = QString::fromCFString(uuidCf);
    CFRelease(uuidCf); 
    qDebug() << "Mac serial number: " << retval;
#endif //Q_OS_MAC

    return retval;
}

