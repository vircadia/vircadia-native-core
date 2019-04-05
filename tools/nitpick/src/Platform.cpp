//
//  Platform.h
//
//  Created by Nissim Hadar on 2 Apr 2019.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Platform.h"

#include <vector>

#ifdef Q_OS_WIN
#include <dxgi1_3.h>
#pragma comment(lib, "dxgi.lib")
#elif defined Q_OS_MAC
#include <OpenGL/OpenGL.h>
#include <sstream>
#include <QStringList>
#endif

QString Platform::getGraphicsCardType() {
#ifdef Q_OS_WIN
    IDXGIFactory1* pFactory = nullptr;
    HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)(&pFactory));
    if (hr != S_OK || pFactory == nullptr) {
        return "Unable to create DXGI";
    }
    std::vector<int> validAdapterList;

    using AdapterEntry = std::pair<std::pair<DXGI_ADAPTER_DESC1, LARGE_INTEGER>, std::vector<DXGI_OUTPUT_DESC>>;
    std::vector<AdapterEntry> adapterToOutputs;
    // Enumerate adapters and outputs
    {
        UINT adapterNum = 0;
        IDXGIAdapter1* pAdapter = nullptr;
        while (pFactory->EnumAdapters1(adapterNum, &pAdapter) != DXGI_ERROR_NOT_FOUND) {

            // Found an adapter, get descriptor
            DXGI_ADAPTER_DESC1 adapterDesc;
            pAdapter->GetDesc1(&adapterDesc);

            LARGE_INTEGER version;
            hr = pAdapter->CheckInterfaceSupport(__uuidof(IDXGIDevice), &version);

            std::wstring wDescription (adapterDesc.Description);
            std::string description(wDescription.begin(), wDescription.end());

            AdapterEntry adapterEntry;
            adapterEntry.first.first = adapterDesc;
            adapterEntry.first.second = version;



            UINT outputNum = 0;
            IDXGIOutput * pOutput;
            bool hasOutputConnectedToDesktop = false;
            while (pAdapter->EnumOutputs(outputNum, &pOutput) != DXGI_ERROR_NOT_FOUND) {

                // FOund an output attached to the adapter, get descriptor
                DXGI_OUTPUT_DESC outputDesc;
                pOutput->GetDesc(&outputDesc);

                adapterEntry.second.push_back(outputDesc);

                std::wstring wDeviceName(outputDesc.DeviceName);
                std::string deviceName(wDeviceName.begin(), wDeviceName.end());

                hasOutputConnectedToDesktop |= (bool)outputDesc.AttachedToDesktop;

                pOutput->Release();
                outputNum++;
            }

            adapterToOutputs.push_back(adapterEntry);

            // add this adapter to the valid list if has output
            if (hasOutputConnectedToDesktop && !adapterEntry.second.empty()) {
                validAdapterList.push_back(adapterNum);
            }

            pAdapter->Release();
            adapterNum++;
        }
    }
    pFactory->Release();

    if (!validAdapterList.empty()) {
        auto& adapterEntry = adapterToOutputs[validAdapterList.front()];

        std::wstring wDescription(adapterEntry.first.first.Description);
        std::string description(wDescription.begin(), wDescription.end());
        return QString(description.c_str());
    }

#elif defined Q_OS_MAC
    FILE* stream = popen("system_profiler SPDisplaysDataType | grep Chipset", "r");

    std::ostringstream hostStream;
    while (!feof(stream) && !ferror(stream)) {
        char buf[128];
        int bytesRead = fread(buf, 1, 128, stream);
        hostStream.write(buf, bytesRead);
    }

    QString result = QString::fromStdString(hostStream.str());
    QStringList parts = result.split('\n');
    for (int i = 0; i < parts.size(); ++i) {
        if (parts[i].toLower().contains("radeon") || parts[i].toLower().contains("nvidia")) {
            return parts[i];
        }
    }

    // unkown graphics card
    return "UNKNOWN";
#else
    return QString("NO IMPLEMENTED");
#endif
    return "";
}
