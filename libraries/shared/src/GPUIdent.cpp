//
//  GPUIdent.cpp
//  libraries/shared/src
//
//  Created by Howard Stearns on 4/16/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

#include <QtCore/QtGlobal>

#ifdef Q_OS_WIN
#include <atlbase.h>
#include <Wbemidl.h>

#elif defined(Q_OS_MAC)
#include <OpenGL/OpenGL.h>
#endif

#include "SharedLogging.h"
#include "GPUIdent.h"

GPUIdent GPUIdent::_instance {};

GPUIdent* GPUIdent::ensureQuery(const QString& vendor, const QString& renderer) {
    // Expects vendor and render to be supplied on first call. Results are cached and the arguments can then be dropped.
    // Too bad OpenGL doesn't seem to have a way to get the specific device id.
    if (_isQueried) {
        return this;
    }
    _isQueried = true;  // Don't try again, even if not _isValid;
#if (defined Q_OS_MAC)
    GLuint cglDisplayMask = -1; // Iterate over all of them.
    CGLRendererInfoObj rendererInfo;
    GLint rendererInfoCount;
    CGLError err = CGLQueryRendererInfo(cglDisplayMask, &rendererInfo, &rendererInfoCount);
    GLint j, numRenderers = 0, deviceVRAM, bestVRAM = 0;
    err = CGLQueryRendererInfo(cglDisplayMask, &rendererInfo, &numRenderers);
    if (0 == err) {
        // Iterate over all of them and use the figure for the one with the most VRAM,
        // on the assumption that this is the one that will actually be used.
        CGLDescribeRenderer(rendererInfo, 0, kCGLRPRendererCount, &numRenderers);
        for (j = 0; j < numRenderers; j++) {
            CGLDescribeRenderer(rendererInfo, j, kCGLRPVideoMemoryMegabytes, &deviceVRAM);
            if (deviceVRAM > bestVRAM) {
                bestVRAM = deviceVRAM;
                _isValid = true;
            }
        }
    }
    _dedicatedMemoryMB = bestVRAM;
    CGLDestroyRendererInfo(rendererInfo);

#elif defined(Q_OS_WIN)
    // COM must be initialized already using CoInitialize. E.g., by the audio subsystem.
    CComPtr<IWbemLocator> spLoc = NULL;
    HRESULT hr = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_SERVER, IID_IWbemLocator, (LPVOID *)&spLoc);
    if (hr != S_OK || spLoc == NULL) {
        qCDebug(shared) << "Unable to connect to WMI";
        return this;
    }

    CComBSTR bstrNamespace(_T("\\\\.\\root\\CIMV2"));
    CComPtr<IWbemServices> spServices;

    // Connect to CIM
    hr = spLoc->ConnectServer(bstrNamespace, NULL, NULL, 0, NULL, 0, 0, &spServices);
    if (hr != WBEM_S_NO_ERROR) {
        qCDebug(shared) << "Unable to connect to CIM";
        return this;
    }

    // Switch the security level to IMPERSONATE so that provider will grant access to system-level objects.
    hr = CoSetProxyBlanket(spServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_DEFAULT);
    if (hr != S_OK) {
        qCDebug(shared) << "Unable to authorize access to system objects.";
        return this;
    }

    // Get the vid controller
    CComPtr<IEnumWbemClassObject> spEnumInst = NULL;
    hr = spServices->CreateInstanceEnum(CComBSTR("Win32_VideoController"), WBEM_FLAG_SHALLOW, NULL, &spEnumInst);
    if (hr != WBEM_S_NO_ERROR || spEnumInst == NULL) {
        qCDebug(shared) << "Unable to reach video controller.";
        return this;
    }

    // I'd love to find a better way to learn which graphics adapter is the one we're using.
    // Alas, no combination of vendor, renderer, and adapter name seems to be a substring of the others.
    // Here we get a list of words that we'll match adapter names against. Most matches wins.
    // Alas, this won't work when someone has multiple variants of the same card installed.
    QRegExp wordMatcher{ "\\W" };
    QStringList words;
    words << vendor.toUpper().split(wordMatcher) << renderer.toUpper().split(wordMatcher);
    words.removeAll("");
    words.removeDuplicates();
    int bestCount = 0;

    ULONG uNumOfInstances = 0;
    CComPtr<IWbemClassObject> spInstance = NULL;
    hr = spEnumInst->Next(WBEM_INFINITE, 1, &spInstance, &uNumOfInstances);
    while (hr == S_OK && spInstance && uNumOfInstances) {
        // Get properties from the object
        CComVariant var;

        hr = spInstance->Get(CComBSTR(_T("Name")), 0, &var, 0, 0);
        if (hr != S_OK) {
            qCDebug(shared) << "Unable to get video name";
            continue;
        }
        char sString[256];
        WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, sString, sizeof(sString), NULL, NULL);
        QStringList adapterWords = QString(sString).toUpper().split(wordMatcher);
        adapterWords.removeAll("");
        adapterWords.removeDuplicates();
        int count = 0;
        for (const QString& adapterWord : adapterWords) {
            if (words.contains(adapterWord)) {
                count++;
            }
        }
        if (count > bestCount) {
            bestCount = count;
            _name = QString(sString).trimmed();

            hr = spInstance->Get(CComBSTR(_T("DriverVersion")), 0, &var, 0, 0);
            if (hr == S_OK) {
                char sString[256];
                WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, sString, sizeof(sString), NULL, NULL);
                _driver = sString;
            }
            else {
                qCDebug(shared) << "Unable to get video driver";
            }

            hr = spInstance->Get(CComBSTR(_T("AdapterRAM")), 0, &var, 0, 0);
            if (hr == S_OK) {
                var.ChangeType(CIM_UINT64);  // We're going to receive some integral type, but it might not be uint.
                // We might be hosed here. The parameter is documented to be UINT32, but that's only 4 GB!
                const ULONGLONG BYTES_PER_MEGABYTE = 1024 * 1024;
                _dedicatedMemoryMB = (uint) (var.ullVal / BYTES_PER_MEGABYTE);
            }
            else {
                qCDebug(shared) << "Unable to get video AdapterRAM";
            }

            _isValid = true;
        }
        hr = spEnumInst->Next(WBEM_INFINITE, 1, &spInstance.p, &uNumOfInstances);
    }
#endif
    return this;
}
