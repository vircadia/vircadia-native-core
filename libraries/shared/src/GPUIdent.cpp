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
#endif

#include "SharedLogging.h"
#include "GPUIdent.h"

GPUIdent GPUIdent::_instance {};

GPUIdent* GPUIdent::ensureQuery() {
	if (_isQueried) {
		return this;
	}
	_isQueried = true;  // Don't try again, even if not _isValid;
#ifdef Q_OS_WIN
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
	hr = CoSetProxyBlanket(spServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, /*EOAC_NONE*/EOAC_DEFAULT);
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
	// alternative. We shouldn't need both this and the above.
	IEnumWbemClassObject* pEnum;
	hr = spServices->ExecQuery(CComBSTR("WQL"), CComBSTR("select * from Win32_VideoController"), WBEM_FLAG_FORWARD_ONLY, NULL, &pEnum);
	if (hr != S_OK) {
		qCDebug(shared) << "Unable to query video controller";
		return this;
	}

	ULONG uNumOfInstances = 0;
	CComPtr<IWbemClassObject> spInstance = NULL;
	hr = spEnumInst->Next(WBEM_INFINITE, 1, &spInstance, &uNumOfInstances);

	if (hr == S_OK && spInstance) {
		// Get properties from the object
		CComVariant var;
		CIMTYPE type;

		hr = spInstance->Get(CComBSTR(_T("AdapterRAM")), 0, &var, 0, 0);
		if (hr == S_OK) {
			var.ChangeType(CIM_UINT32);  // We're going to receive some integral type, but it might not be uint.
			_dedicatedMemoryMB = var.uintVal / (1024 * 1024);
		} else {
			qCDebug(shared) << "Unable to get video AdapterRAM";
		}

		hr = spInstance->Get(CComBSTR(_T("Name")), 0, &var, 0, 0);
		if (hr == S_OK) {
			char sString[256];
			WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, sString, sizeof(sString), NULL, NULL);
			_name = sString;
		} else {
			qCDebug(shared) << "Unable to get video name";
		}

		hr = spInstance->Get(CComBSTR(_T("DriverVersion")), 0, &var, 0, 0);
		if (hr == S_OK) {
			char sString[256];
			WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, sString, sizeof(sString), NULL, NULL);
			_driver = sString;
		} else {
			qCDebug(shared) << "Unable to get video driver";
		}

		_isValid = true;
	} else {
		qCDebug(shared) << "Unable to enerate video adapters";
	}
#endif
	return this;
}