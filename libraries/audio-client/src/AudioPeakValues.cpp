//
//  AudioPeakValues.cpp
//  interface/src
//
//  Created by Zach Pomerantz on 6/26/2017
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AudioClient.h"

#ifdef Q_OS_WIN

#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <audioclient.h>

#include <QString>

#define RETURN_ON_FAIL(result) if (FAILED(result)) { return; }
#define CONTINUE_ON_FAIL(result) if (FAILED(result)) { continue; }

extern QString getWinDeviceName(IMMDevice* pEndpoint);
extern std::mutex _deviceMutex;

std::map<std::wstring, std::shared_ptr<IAudioClient>> activeClients;

template <class T>
void release(T* t) {
    t->Release();
}

template <>
void release(IAudioClient* audioClient) {
    audioClient->Stop();
    audioClient->Release();
}

void AudioClient::checkPeakValues() {
    // prepare the windows environment
    CoInitialize(NULL);

    // if disabled, clean up active clients
    if (!_enablePeakValues) {
        activeClients.clear();
        return;
    }

    // lock the devices so the _inputDevices list is static
    std::unique_lock<std::mutex> lock(_deviceMutex);
    HRESULT result;

    // initialize the payload
    QList<float> peakValueList;
    for (int i = 0; i < _inputDevices.size(); ++i) {
        peakValueList.push_back(0.0f);
    }

    std::shared_ptr<IMMDeviceEnumerator> enumerator;
    {
        IMMDeviceEnumerator* pEnumerator;
        result = CoCreateInstance(
            __uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER,
            __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
        RETURN_ON_FAIL(result);
        enumerator = std::shared_ptr<IMMDeviceEnumerator>(pEnumerator, &release<IMMDeviceEnumerator>);
    }

    std::shared_ptr<IMMDeviceCollection> endpoints;
    {
        IMMDeviceCollection* pEndpoints;
        result = enumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &pEndpoints);
        RETURN_ON_FAIL(result);
        endpoints = std::shared_ptr<IMMDeviceCollection>(pEndpoints, &release<IMMDeviceCollection>);
    }

    UINT count;
    {
        result = endpoints->GetCount(&count);
        RETURN_ON_FAIL(result);
    }

    IMMDevice* pDevice;
    std::shared_ptr<IMMDevice> device;
    IAudioMeterInformation* pMeterInfo;
    std::shared_ptr<IAudioMeterInformation> meterInfo;
    IAudioClient* pAudioClient;
    std::shared_ptr<IAudioClient> audioClient;
    DWORD hardwareSupport;
    LPWSTR pDeviceId = NULL;
    LPWAVEFORMATEX format;
    float peakValue;
    QString deviceName;
    int deviceIndex;
    for (UINT i = 0; i < count; ++i) {
        result = endpoints->Item(i, &pDevice);
        CONTINUE_ON_FAIL(result);
        device = std::shared_ptr<IMMDevice>(pDevice, &release<IMMDevice>);

        // if the device isn't listed through Qt, skip it
        deviceName = ::getWinDeviceName(pDevice);
        deviceIndex = 0;
        for (;  deviceIndex < _inputDevices.size(); ++deviceIndex) {
            if (deviceName == _inputDevices[deviceIndex].deviceName()) {
                break;
            }
        }
        if (deviceIndex >= _inputDevices.size()) {
            continue;
        }

        //continue;

        result = device->Activate(__uuidof(IAudioMeterInformation), CLSCTX_ALL, NULL, (void**)&pMeterInfo);
        CONTINUE_ON_FAIL(result);
        meterInfo = std::shared_ptr<IAudioMeterInformation>(pMeterInfo, &release<IAudioMeterInformation>);

        //continue;

        hardwareSupport;
        result = meterInfo->QueryHardwareSupport(&hardwareSupport);
        CONTINUE_ON_FAIL(result);

        //continue;

        // if the device has no hardware support (USB)...
        if (!(hardwareSupport & ENDPOINT_HARDWARE_SUPPORT_METER)) {
            result = device->GetId(&pDeviceId);
            CONTINUE_ON_FAIL(result);
            std::wstring deviceId(pDeviceId);
            CoTaskMemFree(pDeviceId);

            //continue;

            // ...and no active client...
            if (activeClients.find(deviceId) == activeClients.end()) {
                result = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&pAudioClient);
                CONTINUE_ON_FAIL(result);
                audioClient = std::shared_ptr<IAudioClient>(pAudioClient, &release<IAudioClient>);

                //continue;

                // ...activate a client
                audioClient->GetMixFormat(&format);
                audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 0, 0, format, NULL);
                audioClient->Start();

                //continue;

                activeClients[deviceId] = audioClient;
            }
        }

        // get the peak value and put it in the payload
        meterInfo->GetPeakValue(&peakValue);
        peakValueList[deviceIndex] = peakValue;
    }

    emit peakValueListChanged(peakValueList);
}

bool AudioClient::peakValuesAvailable() const {
    return true;
}

#else
void AudioClient::checkPeakValues() {
}

bool AudioClient::peakValuesAvailable() const {
    return false;
}
#endif