#pragma once
#include <QtCore>
#include <QVariant>
#include <QObject>
#include <gpu/Format.h>
#include <gpu/Stream.h>
//#include <gpu/Buffer.h>

Q_DECLARE_METATYPE(gpu::Type);
#ifdef QT_MOC_RUN
class DebugNames {
        Q_OBJECT
public:
#else
  namespace DebugNames {
    Q_NAMESPACE
        #endif
    
enum Type : uint8_t {

    FLOAT = 0,
    INT32,
    UINT32,
    HALF,
    INT16,
    UINT16,
    INT8,
    UINT8,

    NINT32,
    NUINT32,
    NINT16,
    NUINT16,
    NINT8,
    NUINT8,

    COMPRESSED,

    NUM_TYPES,

    BOOL = UINT8,
    NORMALIZED_START = NINT32,
};
    
    Q_ENUM_NS(Type)
    enum InputSlot {
        POSITION = 0,
        NORMAL = 1,
        COLOR = 2,
        TEXCOORD0 = 3,
        TEXCOORD = TEXCOORD0,
        TANGENT = 4,
        SKIN_CLUSTER_INDEX = 5,
        SKIN_CLUSTER_WEIGHT = 6,
        TEXCOORD1 = 7,
        TEXCOORD2 = 8,
        TEXCOORD3 = 9,
        TEXCOORD4 = 10,

        NUM_INPUT_SLOTS,

        DRAW_CALL_INFO = 15, // Reserve last input slot for draw call infos
    };

    Q_ENUM_NS(InputSlot)
    inline QString stringFrom(Type t) { return QVariant::fromValue(t).toString(); }
    inline QString stringFrom(InputSlot t) { return QVariant::fromValue(t).toString(); }
    inline QString stringFrom(gpu::Type t) { return stringFrom((Type)t); }
    inline QString stringFrom(gpu::Stream::Slot t) { return stringFrom((InputSlot)t); }

    extern const QMetaObject staticMetaObject;
  };
