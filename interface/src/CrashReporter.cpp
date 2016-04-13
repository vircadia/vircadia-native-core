//
//  CrashReporter.cpp
//  interface/src
//
//  Created by Ryan Huffman on 11 April 2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#ifdef HAS_BUGSPLAT
#include "CrashReporter.h"

#include <new.h>
#include <Windows.h>

#include <csignal>

#include <QDebug>

// SetUnhandledExceptionFilter can be overridden by the CRT at the point that an error occurs. More information
// can be found here: http://www.codeproject.com/Articles/154686/SetUnhandledExceptionFilter-and-the-C-C-Runtime-Li
// A fairly common approach is to patch the SetUnhandledExceptionFilter so that it cannot be overridden and so
// that the applicaiton can handle it itself.
// The CAPIHook class referenced in the above article is not openly available, but a similar implementation
// can be found here: http://blog.kalmbach-software.de/2008/04/02/unhandled-exceptions-in-vc8-and-above-for-x86-and-x64/
// The below has been adapted to work with different library and functions.
BOOL redirectLibraryFunctionToFunction(char* library, char* function, void* fn)
{
    HMODULE lib = LoadLibrary(library);
    if (lib == NULL) return FALSE;
    void *pOrgEntry = GetProcAddress(lib, function);
    if (pOrgEntry == NULL) return FALSE;

    DWORD dwOldProtect = 0;
    SIZE_T jmpSize = 5;
#ifdef _M_X64
    jmpSize = 13;
#endif
    BOOL bProt = VirtualProtect(pOrgEntry, jmpSize,
        PAGE_EXECUTE_READWRITE, &dwOldProtect);
    BYTE newJump[20];
    void *pNewFunc = fn;
#ifdef _M_IX86
    DWORD dwOrgEntryAddr = (DWORD)pOrgEntry;
    dwOrgEntryAddr += jmpSize; // add 5 for 5 op-codes for jmp rel32
    DWORD dwNewEntryAddr = (DWORD)pNewFunc;
    DWORD dwRelativeAddr = dwNewEntryAddr - dwOrgEntryAddr;
    // JMP rel32: Jump near, relative, displacement relative to next instruction.
    newJump[0] = 0xE9;  // JMP rel32
    memcpy(&newJump[1], &dwRelativeAddr, sizeof(pNewFunc));
#elif _M_X64
    // We must use R10 or R11, because these are "scratch" registers 
    // which need not to be preserved accross function calls
    // For more info see: Register Usage for x64 64-Bit
    // http://msdn.microsoft.com/en-us/library/ms794547.aspx
    // Thanks to Matthew Smith!!!
    newJump[0] = 0x49;  // MOV R11, ...
    newJump[1] = 0xBB;  // ...
    memcpy(&newJump[2], &pNewFunc, sizeof(pNewFunc));
    //pCur += sizeof (ULONG_PTR);
    newJump[10] = 0x41;  // JMP R11, ...
    newJump[11] = 0xFF;  // ...
    newJump[12] = 0xE3;  // ...
#endif
    SIZE_T bytesWritten;
    BOOL bRet = WriteProcessMemory(GetCurrentProcess(),
        pOrgEntry, newJump, jmpSize, &bytesWritten);

    if (bProt != FALSE)
    {
        DWORD dwBuf;
        VirtualProtect(pOrgEntry, jmpSize, dwOldProtect, &dwBuf);
    }
    return bRet;
}


void handleSignal(int signal) {
    // Throw so BugSplat can handle
    throw(signal);
}

void handlePureVirtualCall() {
    // Throw so BugSplat can handle
    throw("ERROR: Pure virtual call");
}

void handleInvalidParameter(const wchar_t * expression, const wchar_t * function, const wchar_t * file,
                            unsigned int line, uintptr_t pReserved ) {
    // Throw so BugSplat can handle
    throw("ERROR: Invalid parameter");
}

int handleNewError(size_t size) {
    // Throw so BugSplat can handle
    throw("ERROR: Errors calling new");
}

LPTOP_LEVEL_EXCEPTION_FILTER WINAPI noop_SetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter) {
    return nullptr;
}

_purecall_handler __cdecl noop_set_purecall_handler(_purecall_handler pNew) {
    return nullptr;
}

static const DWORD BUG_SPLAT_FLAGS = MDSF_PREVENTHIJACKING | MDSF_USEGUARDMEMORY;

CrashReporter::CrashReporter(QString bugSplatDatabase, QString bugSplatApplicationName, QString version)
    : mpSender(qPrintable(bugSplatDatabase), qPrintable(bugSplatApplicationName), qPrintable(version), nullptr, BUG_SPLAT_FLAGS)
{
    signal(SIGSEGV, handleSignal);
    signal(SIGABRT, handleSignal);
    _set_purecall_handler(handlePureVirtualCall);
    _set_invalid_parameter_handler(handleInvalidParameter);
    _set_new_mode(1);
    _set_new_handler(handleNewError);

    // Disable WER popup
    //SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    //_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);

    // QtWebEngineCore internally sets its own purecall handler, overriding our own error handling. This disables that.
    if (!redirectLibraryFunctionToFunction("msvcr120.dll", "_set_purecall_handler", &noop_set_purecall_handler)) {
        qWarning() << "Failed to patch _set_purecall_handler";
    }
    // Patch SetUnhandledExceptionFilter to keep the CRT from overriding our own error handling.
    if (!redirectLibraryFunctionToFunction("kernel32.dll", "SetUnhandledExceptionFilter", &noop_SetUnhandledExceptionFilter)) {
        qWarning() << "Failed to patch setUnhandledExceptionFilter";
    }
}
#endif
