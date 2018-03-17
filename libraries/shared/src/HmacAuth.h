//
// HmacAuth.h
// libraries/shared/src

#ifndef hifi_HmacAuth_h
#define hifi_HmacAuth_h

#include <vector>
#include <memory>
#include <QtCore/QMutex>

struct hmac_ctx_st;
class QUuid;

class HmacAuth {
public:
    enum AuthMethod { MD5, SHA1, SHA224, SHA256, RIPEMD160 };
    typedef std::vector<unsigned char> HmacHash;
    
    explicit HmacAuth(AuthMethod authMethod = MD5);
    ~HmacAuth();

    bool setKey(const char * keyValue, int keyLen);
    bool setKey(const QUuid& uidKey);
    bool addData(const char * data, int dataLen);
    HmacHash result();

private:
    QMutex _lock;
    std::unique_ptr<hmac_ctx_st> _hmacContext;
    AuthMethod _authMethod { MD5 };
};

#endif  // hifi_HmacAuth_h
