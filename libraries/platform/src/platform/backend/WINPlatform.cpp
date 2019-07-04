//
//  Created by Amer Cerkic 05/02/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "WINPlatform.h"
#include "../PlatformKeys.h"

#include <thread>
#include <string>
#include <CPUIdent.h>
#include <GPUIdent.h>

#ifdef Q_OS_WIN
#include <Windows.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <QSysInfo>
#include <dxgi1_3.h>
#pragma comment(lib, "dxgi.lib")
#endif

using namespace platform;

void WINInstance::enumerateCpus() {
    json cpu = {};
    
    cpu[keys::cpu::vendor] = CPUIdent::Vendor();
    cpu[keys::cpu::model] = CPUIdent::Brand();
    cpu[keys::cpu::numCores] = std::thread::hardware_concurrency();

    _cpus.push_back(cpu);
}

void WINInstance::enumerateGpus() {
#ifdef Q_OS_WIN
    struct ConvertLargeIntegerToQString {
        QString convert(const LARGE_INTEGER& version) {
            QString value;
            value.append(QString::number(uint32_t(((version.HighPart & 0xFFFF0000) >> 16) & 0x0000FFFF)));
            value.append(".");
            value.append(QString::number(uint32_t((version.HighPart) & 0x0000FFFF)));
            value.append(".");
            value.append(QString::number(uint32_t(((version.LowPart & 0xFFFF0000) >> 16) & 0x0000FFFF)));
            value.append(".");
            value.append(QString::number(uint32_t((version.LowPart) & 0x0000FFFF)));
            return value;
        }
    } convertDriverVersionToString;

    // Create the DXGI factory
    // Let s get into DXGI land:
    HRESULT hr = S_OK;

    IDXGIFactory1* pFactory = nullptr;
    hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)(&pFactory));
    if (hr != S_OK || pFactory == nullptr) {
//        qCDebug(shared) << "Unable to create DXGI";
        return;
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

            std::wstring wDescription(adapterDesc.Description);
            std::string description(wDescription.begin(), wDescription.end());

            json gpu = {};
            gpu[keys::gpu::model] = description;
            gpu[keys::gpu::vendor] = findGPUVendorInDescription(gpu[keys::gpu::model].get<std::string>());
            const SIZE_T BYTES_PER_MEGABYTE = 1024 * 1024;
            gpu[keys::gpu::videoMemory] = (uint32_t)(adapterDesc.DedicatedVideoMemory / BYTES_PER_MEGABYTE);
            gpu[keys::gpu::driver] = convertDriverVersionToString.convert(version).toStdString();


            std::vector<int> displayIndices;

            UINT outputNum = 0;
            IDXGIOutput* pOutput;
            bool hasOutputConnectedToDesktop = false;
            while (pAdapter->EnumOutputs(outputNum, &pOutput) != DXGI_ERROR_NOT_FOUND) {
                // FOund an output attached to the adapter, get descriptor
                DXGI_OUTPUT_DESC outputDesc;
                pOutput->GetDesc(&outputDesc);

                // Grab the Monitor desc
                // MONITOR_DESC

                json display = {};

                // Desiplay name
                std::wstring wDeviceName(outputDesc.DeviceName);
                std::string deviceName(wDeviceName.begin(), wDeviceName.end());
                display[keys::display::name] = deviceName;
                display[keys::display::description] = "";

                // Rect region of the desktop in desktop units
                display["desktopRect"] = (outputDesc.AttachedToDesktop ? true : false);
                display[keys::display::coordsLeft] = outputDesc.DesktopCoordinates.left;
                display[keys::display::coordsRight] = outputDesc.DesktopCoordinates.right;
                display[keys::display::coordsBottom] = outputDesc.DesktopCoordinates.bottom;
                display[keys::display::coordsTop] = outputDesc.DesktopCoordinates.top;

                // Add the display index to the list of displays of the gpu
                displayIndices.push_back(_displays.size());
                
                // And set the gpu index to the display description
                display[keys::display::gpu] = _gpus.size();

                // One more display desc
                _displays.push_back(display);
                pOutput->Release();
                outputNum++;
            }
            gpu[keys::gpu::displays] = displayIndices;

            _gpus.push_back(gpu);
            pAdapter->Release();
            adapterNum++;
        }
    }
    pFactory->Release();
#endif

    GPUIdent* ident = GPUIdent::getInstance();
   
    json gpu = {};
    gpu[keys::gpu::model] = ident->getName().toUtf8().constData();
    gpu[keys::gpu::vendor] = findGPUVendorInDescription(gpu[keys::gpu::model].get<std::string>());
    gpu[keys::gpu::videoMemory] = ident->getMemory();
    gpu[keys::gpu::driver] = ident->getDriver().toUtf8().constData();

    _gpus.push_back(gpu);
  //  _displays = ident->getOutput();
}

void WINInstance::enumerateMemory() {
    json ram = {};
    
#ifdef Q_OS_WIN
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);
    int totalRam = statex.ullTotalPhys / 1024 / 1024;
    ram[platform::keys::memory::memTotal] = totalRam;
#endif
    _memory = ram;
}

void WINInstance::enumerateComputer(){
    _computer[keys::computer::OS] = keys::computer::OS_WINDOWS;
    _computer[keys::computer::vendor] = "";
    _computer[keys::computer::model] = "";
    
    auto sysInfo = QSysInfo();

    _computer[keys::computer::OSVersion] = sysInfo.kernelVersion().toStdString();
}

void WINInstance::enumerateNics() {
    // Start with the default from QT, getting the result into _nics:
    Instance::enumerateNics();

#ifdef Q_OS_WIN
    // We can usually do better than the QNetworkInterface::humanReadableName() by
    // matching up Iphlpapi.lib IP_ADAPTER_INFO by mac id.
    ULONG buflen = sizeof(IP_ADAPTER_INFO);
    IP_ADAPTER_INFO* pAdapterInfo = (IP_ADAPTER_INFO*) malloc(buflen);

    // Size the buffer:
    if (GetAdaptersInfo(pAdapterInfo, &buflen) == ERROR_BUFFER_OVERFLOW) {
        free(pAdapterInfo);
        pAdapterInfo = (IP_ADAPTER_INFO *) malloc(buflen);
    }

    // Now get the data...
    if (GetAdaptersInfo(pAdapterInfo, &buflen) == NO_ERROR) {
        for (json& nic : _nics) { // ... loop through the nics from above...
            // ...convert the json to a string without the colons...
            QString qtmac = nic[keys::nic::mac].get<std::string>().c_str();
            QString qtraw = qtmac.remove(QChar(':'), Qt::CaseInsensitive).toLower();
            // ... and find the matching one in pAdapter:
            for (IP_ADAPTER_INFO* pAdapter = pAdapterInfo; pAdapter; pAdapter = pAdapter->Next) {
                QByteArray wmac = QByteArray((const char*) (pAdapter->Address), pAdapter->AddressLength);
                QString wraw = wmac.toHex();
                if (qtraw == wraw) {
                    nic[keys::nic::name] = pAdapter->Description;
                    break;
                }
            }
        }
    }

    if (pAdapterInfo) {
        free(pAdapterInfo);
    }
#endif
}