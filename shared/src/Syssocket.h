#ifndef __Syssocket__
#define __Syssocket__

#ifdef _WIN32
#define WINSOCK_API_LINKAGE
#include <winsock2.h>
#define _timeval_

typedef SSIZE_T ssize_t;
typedef ULONG32 in_addr_t; 
typedef USHORT in_port_t;
typedef USHORT uint16_t;
typedef ULONG32 socklen_t;

#endif _Win32

#endif __Syssocket__