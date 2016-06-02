#ifndef hifi_Packed_h
#define hifi_Packed_h

#if defined(_MSC_VER)
#define PACKED_BEGIN __pragma(pack(push, 1))
#define PACKED_END __pragma(pack(pop));
#else
#define PACKED_BEGIN
#define PACKED_END __attribute__((__packed__));
#endif

#endif
