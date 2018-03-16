//
// HmacAuth.h
// libraries/shared/src

#ifndef hifi_HmacAuth_h
#define hifi_HmacAuth_h

#include <vector>
#include <memory>

struct hmac_ctx_st;
class QUuid;

class HmacAuth {
public:
    enum AuthMethod { SHA1, RIPEMD160 };
    typedef std::vector<unsigned char> HmacHash;
    
    HmacAuth(AuthMethod authMethod = SHA1);
    ~HmacAuth();

    bool setKey(const char * keyValue, int keyLen);
    bool setKey(const QUuid& uidKey);
    bool addData(const char * data, int dataLen);
    HmacHash result();

private:
    std::unique_ptr<hmac_ctx_st> _hmacContext;
    AuthMethod _authMethod { SHA1 };
};

#endif  // hifi_HmacAuth_h
