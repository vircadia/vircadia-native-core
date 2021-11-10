/**
 * If you want to work through what the code is actually doing this has an excellent
 * description of the protocol being used.
 *
 * https://github.com/alexpeattie/letsencrypt-fromscratch
 */

#include "acme-lw.h"

#include "http.h"

// From here: https://github.com/nlohmann/json/blob/develop/single_include/nlohmann/json.hpp
// This should probably be a git submodule, but the repo is huge.
#include "json.hpp"

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/x509v3.h>

#include <algorithm>
#include <ctype.h>
#include <sstream>
#include <stdio.h>
#include <typeinfo>
#include <unistd.h>
#include <vector>

using namespace std;

using namespace acme_lw_internal;

namespace
{

#ifdef STAGING
const char * directoryUrl = "https://acme-staging-v02.api.letsencrypt.org/directory";
#else
const char * directoryUrl = "https://acme-v02.api.letsencrypt.org/directory";
#endif

// Smart pointers for OpenSSL types
template<typename TYPE, void (*FREE)(TYPE *)>
struct Ptr
{
    Ptr()
        : ptr_(nullptr)
    {
    }

    Ptr(TYPE * ptr)
        : ptr_(ptr)
    {
        if (!ptr_)
        {
            throw acme_lw::AcmeException("Failed to create "s + typeid(*this).name());
        }
    }

    ~Ptr()
    {
        if (ptr_)
        {
            FREE(ptr_);
        }
    }

    Ptr& operator = (Ptr&& ptr)
    {
        if (!ptr.ptr_)
        {
            throw acme_lw::AcmeException("Failed to create "s + typeid(*this).name());
        }

        ptr_ = move(ptr.ptr_);
        ptr.ptr_ = nullptr;

        return *this;
    }

    bool operator ! () const
    {
        return !ptr_;
    }

    TYPE * operator * () const
    {
        return ptr_;
    }

    void clear()
    {
        ptr_ = nullptr;
    }

private:
    TYPE * ptr_;
};

typedef Ptr<BIO, BIO_free_all>                                  BIOptr;
typedef Ptr<RSA, RSA_free>                                      RSAptr;
typedef Ptr<BIGNUM, BN_clear_free>                              BIGNUMptr;
typedef Ptr<EVP_MD_CTX, EVP_MD_CTX_free>                        EVP_MD_CTXptr;
typedef Ptr<EVP_PKEY, EVP_PKEY_free>                            EVP_PKEYptr;
typedef Ptr<X509, X509_free>                                    X509ptr;
typedef Ptr<X509_REQ, X509_REQ_free>                            X509_REQptr;

void freeStackOfExtensions(STACK_OF(X509_EXTENSION) * e)
{
    sk_X509_EXTENSION_pop_free(e, X509_EXTENSION_free);
}

typedef Ptr<STACK_OF(X509_EXTENSION), freeStackOfExtensions>    X509_EXTENSIONSptr;

template<typename T>
T toT(const vector<char>& v)
{
    return v;
}

template<>
string toT(const vector<char>& v)
{
    return string(&v.front(), v.size());
}

vector<char> toVector(BIO * bio)
{
    constexpr int buffSize = 1024;

    vector<char> buffer(buffSize);

    size_t pos = 0;
    int count = 0;
    do
    {
        count = BIO_read(bio, &buffer.front() + pos, buffSize);
        if (count > 0)
        {
            pos += count;
            buffer.resize(pos + buffSize);
        }
    }
    while (count > 0);

    buffer.resize(pos);

    return buffer;
}

string toString(BIO *bio)
{
    vector<char> v = toVector(bio);
    return string(&v.front(), v.size());
}

template<typename T>
string base64Encode(const T& t)
{
    if (!t.size())
    {
        return "";
    }
    // Use openssl to do this since we're already linking to it.

    // Don't need (or want) a BIOptr since BIO_push chains it to b64
    BIO * bio(BIO_new(BIO_s_mem()));
    BIOptr b64(BIO_new(BIO_f_base64()));

    // OpenSSL inserts new lines by default to make it look like PEM format.
    // Turn that off.
    BIO_set_flags(*b64, BIO_FLAGS_BASE64_NO_NL);

    BIO_push(*b64, bio);
    if (BIO_write(*b64, &t.front(), t.size()) <= 0 ||
        BIO_flush(*b64) < 0)
    {
        throw acme_lw::AcmeException("Failure in BIO_write / BIO_flush");
    }

    return toString(bio);
}

template<typename T>
string urlSafeBase64Encode(const T& t)
{
    string s = base64Encode(t);

    // We need url safe base64 encoding and openssl only gives us regular
    // base64, so we convert.
    size_t len = s.size();
    for (size_t i = 0; i < len; ++i)
    {
        if (s[i] == '+')
        {
            s[i] = '-';
        }
        else if (s[i] == '/')
        {
            s[i] = '_';
        }
        else if (s[i] == '=')
        {
            s.resize(i);
            break;
        }
    }

    return s;
}

string urlSafeBase64Encode(const BIGNUM * bn)
{
    int numBytes = BN_num_bytes(bn);
    vector<unsigned char> buffer(numBytes);
    BN_bn2bin(bn, &buffer.front());

    return urlSafeBase64Encode(buffer);
}

// returns pair<CSR, privateKey>
pair<string, string> makeCertificateSigningRequest(const std::list<std::string>& domainNames)
{
    BIGNUMptr bn(BN_new());
    if (!BN_set_word(*bn, RSA_F4))
    {
        throw acme_lw::AcmeException("Failure in BN_set_word");
    }

    RSAptr rsa(RSA_new());

    int bits = 2048;
    if (!RSA_generate_key_ex(*rsa, bits, *bn, nullptr))
    {
        throw acme_lw::AcmeException("Failure in RSA_generate_key_ex");
    }

    X509_REQptr req(X509_REQ_new());

    auto name = domainNames.begin();

    X509_NAME * cn = X509_REQ_get_subject_name(*req);
    if (!X509_NAME_add_entry_by_txt(cn,
                                    "CN",
                                    MBSTRING_ASC,
                                    reinterpret_cast<const unsigned char*>(name->c_str()),
                                    -1, -1, 0))
    {
        throw acme_lw::AcmeException("Failure in X509_Name_add_entry_by_txt");
    }

    if (++name != domainNames.end())
    {
        // We have one or more Subject Alternative Names
        X509_EXTENSIONSptr extensions(sk_X509_EXTENSION_new_null());

        string value;
        do
        {
            if (!value.empty())
            {
                value += ", ";
            }
            value += "DNS:" + *name;
        }
        while (++name != domainNames.end());

        if (!sk_X509_EXTENSION_push(*extensions, X509V3_EXT_conf_nid(nullptr, nullptr, NID_subject_alt_name, value.c_str())))
        {
            throw acme_lw::AcmeException("Unable to add Subject Alternative Name to extensions");
        }

        if (X509_REQ_add_extensions(*req, *extensions) != 1)
        {
            throw acme_lw::AcmeException("Unable to add Subject Alternative Names to CSR");
        }
    }

    EVP_PKEYptr key(EVP_PKEY_new());
    if (!EVP_PKEY_assign_RSA(*key, *rsa))
    {
        throw acme_lw::AcmeException("Failure in EVP_PKEY_assign_RSA");
    }
    rsa.clear();     // rsa will be freed when key is freed.

    BIOptr keyBio(BIO_new(BIO_s_mem()));
    if (PEM_write_bio_PrivateKey(*keyBio, *key, nullptr, nullptr, 0, nullptr, nullptr) != 1)
    {
        throw acme_lw::AcmeException("Failure in PEM_write_bio_PrivateKey");
    }

    string privateKey = toString(*keyBio);

    if (!X509_REQ_set_pubkey(*req, *key))
    {
        throw acme_lw::AcmeException("Failure in X509_REQ_set_pubkey");
    }

    if (!X509_REQ_sign(*req, *key, EVP_sha256()))
    {
        throw acme_lw::AcmeException("Failure in X509_REQ_sign");
    }

    BIOptr reqBio(BIO_new(BIO_s_mem()));
    if (i2d_X509_REQ_bio(*reqBio, *req) < 0)
    {
        throw acme_lw::AcmeException("Failure in i2d_X509_REQ_bio");
    }

    return make_pair(urlSafeBase64Encode(toVector(*reqBio)), privateKey);
}

string sha256(const string& s)
{
    vector<unsigned char> hash(SHA256_DIGEST_LENGTH);
    SHA256_CTX sha256;
    if (!SHA256_Init(&sha256) ||
        !SHA256_Update(&sha256, s.c_str(), s.size()) ||
        !SHA256_Final(&hash.front(), &sha256))
    {
        throw acme_lw::AcmeException("Error hashing a string");
    }

    return urlSafeBase64Encode(hash);
}

// https://tools.ietf.org/html/rfc7638
string makeJwkThumbprint(const string& jwk)
{
    string strippedJwk = jwk;

    // strip whitespace
    strippedJwk.erase(remove_if(strippedJwk.begin(), strippedJwk.end(), ::isspace), strippedJwk.end());

    return sha256(strippedJwk);
}

template<typename T>
T extractExpiryData(const acme_lw::Certificate& certificate, const function<T (const ASN1_TIME *)>& extractor)
{
    BIOptr bio(BIO_new(BIO_s_mem()));
    if (BIO_write(*bio, &certificate.fullchain.front(), certificate.fullchain.size()) <= 0)
    {
        throw acme_lw::AcmeException("Failure in BIO_write");
    }
    X509ptr x509(PEM_read_bio_X509(*bio, nullptr, nullptr, nullptr));

    ASN1_TIME * t = X509_getm_notAfter(*x509);

    return extractor(t);
}

}

namespace acme_lw
{

string newAccountUrl;
string newOrderUrl;
string newNonceUrl;

struct AcmeClientImpl
{
    AcmeClientImpl(const string& accountPrivateKey)
        : privateKey_(EVP_PKEY_new())
    {
        // Create the private key and 'header suffix', used to sign LE certs.
        BIOptr bio(BIO_new_mem_buf(accountPrivateKey.c_str(), -1));
        RSA * rsa(PEM_read_bio_RSAPrivateKey(*bio, nullptr, nullptr, nullptr));
        if (!rsa)
        {
            throw AcmeException("Unable to read private key");
        }

        // rsa will get freed when privateKey_ is freed
        if (!EVP_PKEY_assign_RSA(*privateKey_, rsa))
        {
            throw AcmeException("Unable to assign RSA to private key");
        }

        const BIGNUM *n, *e, *d;
        RSA_get0_key(rsa, &n, &e, &d);

        // Note json keys must be in lexographical order.
        string jwkValue = u8R"( {
                                    "e":")"s + urlSafeBase64Encode(e) + u8R"(",
                                    "kty": "RSA",
                                    "n":")"s + urlSafeBase64Encode(n) + u8R"("
                                })";
        jwkThumbprint_ = makeJwkThumbprint(jwkValue);

        // We use jwk for the first request, which allows us to get
        // the account id. We use that thereafter.
        headerSuffix_ = u8R"(
                "alg": "RS256",
                "jwk": )" + jwkValue + "}";

        pair<string, string> header = make_pair("location"s, ""s);
        sendRequest<string>(newAccountUrl, u8R"(
                                                {
                                                    "termsOfServiceAgreed": true,
                                                    "onlyReturnExisting": true
                                                }
                                                )", &header);
        headerSuffix_ = u8R"(
                "alg": "RS256",
                "kid": ")" + header.second + "\"}";
    }

    string sign(const string& s)
    {
        // https://wiki.openssl.org/index.php/EVP_Signing_and_Verifying
        size_t signatureLength = 0;

        EVP_MD_CTXptr context(EVP_MD_CTX_create());
        const EVP_MD * sha256 = EVP_get_digestbyname("SHA256");
        if (!sha256 ||
            EVP_DigestInit_ex(*context, sha256, nullptr) != 1 ||
            EVP_DigestSignInit(*context, nullptr, sha256, nullptr, *privateKey_) != 1 ||
            EVP_DigestSignUpdate(*context, s.c_str(), s.size()) != 1 ||
            EVP_DigestSignFinal(*context, nullptr, &signatureLength) != 1)
        {
            throw AcmeException("Error creating SHA256 digest");
        }

        vector<unsigned char> signature(signatureLength);
        if (EVP_DigestSignFinal(*context, &signature.front(), &signatureLength) != 1)
        {
            throw AcmeException("Error creating SHA256 digest in final signature");
        }

        return urlSafeBase64Encode(signature);
    }

    template<typename T>
    T sendRequest(const string& url, const string& payload, pair<string, string> * header = nullptr)
    {
        string protectd = u8R"({"nonce": ")"s +
                                    getHeader(newNonceUrl, "replay-nonce") + "\"," +
                                    u8R"("url": ")" + url + "\"," +
                                    headerSuffix_;

        protectd = urlSafeBase64Encode(protectd);
        string payld = urlSafeBase64Encode(payload);

        string signature = sign(protectd + "." + payld);

        string body = "{"s +
                        u8R"("protected": ")" + protectd + "\"," +
                        u8R"("payload": ")" + payld + "\"," +
                        u8R"("signature": ")" + signature + "\"}";

        Response response = doPost(url, body, header ? header-> first.c_str() : nullptr);

        if (header)
        {
            header->second = response.headerValue_;
        }

        return toT<T>(response.response_);
    }

    // https://tools.ietf.org/html/rfc8555#section-6.3
    string doPostAsGet(const string& url)
    {
        return sendRequest<string>(url, "");
    }

    void wait(const string& url, const char * errorText)
    {
        // Poll waiting for response to the url be 'status': 'valid'
        int counter = 0;
        constexpr int count = 10;
        do
        {
            ::usleep(1'000'000);    // sleep for a second
            string response = doPostAsGet(url);
            auto json = nlohmann::json::parse(response);
            if (json.at("status") == "valid")
            {
                return;
            }
        } while (counter++ < count);

        throw AcmeException(errorText);
    }

    // Throws if the challenge isn't accepted (or on timeout)
    void verifyChallengePassed(const nlohmann::json& challenge)
    {
        // Tell the CA we're prepared for the challenge.
        string verificationUrl = challenge.at("url");
        auto response = nlohmann::json::parse(sendRequest<string>(verificationUrl, "{}"));
        if (response.at("status") == "valid")
        {
            return;
        }

        string challengeStatusUrl = response.at("url");
        wait(challengeStatusUrl, "Failure / timeout verifying challenge passed");
    }

    Certificate issueCertificate(const list<string>& domainNames, AcmeClient::Callback callback)
    {
        if (domainNames.empty())
        {
            throw AcmeException("There must be at least one domain name in a certificate");
        }

        // Create the order
        string payload = u8R"({"identifiers": [)";
        bool first = true;
        for (const string& domain : domainNames)
        {
            if (!first)
            {
                payload += ",";
            }
            first = false;

            payload += u8R"(
                            {
                                "type": "dns",
                                "value": ")"s + domain + u8R"("
                            }
                           )";
        }
        payload += "]}";

        pair<string, string> header = make_pair("location"s, ""s);
        string response = sendRequest<string>(newOrderUrl, payload, &header);
        string currentOrderUrl = header.second;

        // Pass the challenges
        auto json = nlohmann::json::parse(response);
        auto authorizations = json.at("authorizations");
        for (const auto& authorization : authorizations)
        {
            auto authz = nlohmann::json::parse(doPostAsGet(authorization));
            /**
             * If you pass a challenge, that's good for 300 days. The cert is only good for 90.
             * This means for a while you can re-issue without passing another challenge, so we
             * check to see if we need to validate again.
             *
             * Note that this introduces a race since it possible for the status to not be valid
             * by the time the certificate is requested. The assumption is that client retries
             * will deal with this.
             */
            if (authz.at("status") != "valid")
            {
                auto challenges = authz.at("challenges");
                for (const auto& challenge : challenges)
                {
                    if (challenge.at("type") == "http-01")
                    {
                        string token = challenge.at("token");
                        string domain = authz.at("identifier").at("value");
                        string url = "http://"s + domain + "/.well-known/acme-challenge/" + token;
                        string keyAuthorization = token + "." + jwkThumbprint_;
                        callback(domain, url, keyAuthorization);

                        verifyChallengePassed(challenge);
                        break;
                    }
                }
            }
        }

        // Request the certificate
        auto r = makeCertificateSigningRequest(domainNames);
        string csr = r.first;
        string privateKey = r.second;
        string certificateUrl = nlohmann::json::parse(sendRequest<vector<char>>(json.at("finalize"),
                                                u8R"(   {
                                                            "csr": ")"s + csr + u8R"("
                                                        })")).at("certificate");

        // Wait for the certificate to be produced
        wait(currentOrderUrl, "Timeout / failure waiting for certificate to be produced");

        // Retreive the certificate
        Certificate cert;
        cert.fullchain = doPostAsGet(certificateUrl);
        cert.privkey = privateKey;
        return cert;
    }

private:
    string      headerSuffix_;
    EVP_PKEYptr privateKey_;
    string      jwkThumbprint_;
};

AcmeClient::AcmeClient(const string& accountPrivateKey)
    : impl_(new AcmeClientImpl(accountPrivateKey))
{
}

AcmeClient::~AcmeClient() = default;

Certificate AcmeClient::issueCertificate(const std::list<std::string>& domainNames, Callback callback)
{
    return impl_->issueCertificate(domainNames, callback);
}

void AcmeClient::init()
{
    initHttp();

    try
    {
        string directory = toT<string>(doGet(directoryUrl));
        auto json = nlohmann::json::parse(directory);
        newAccountUrl = json.at("newAccount");
        newOrderUrl = json.at("newOrder");
        newNonceUrl = json.at("newNonce");
    }
    catch (const exception& e)
    {
        throw AcmeException("Unable to initialize endpoints from "s + directoryUrl + ": " + e.what());
    }
}

void AcmeClient::teardown()
{
    teardownHttp();
}

::time_t Certificate::getExpiry() const
{
    return extractExpiryData<::time_t>(*this, [](const ASN1_TIME * t)
        {
#ifdef OPENSSL_TO_TM
            // Prior to openssl 1.1.1 (or so?) ASN1_TIME_to_tm didn't exist so there was no
            // good way of converting to time_t. If it exists we use the built in function.

            ::tm out;
            if (!ASN1_TIME_to_tm(t, &out))
            {
                throw AcmeException("Failure in ASN1_TIME_to_tm");
            }

            return timegm(&out);
#else
            // See this link for issues in converting from ASN1_TIME to epoch time.
            // https://stackoverflow.com/questions/10975542/asn1-time-to-time-t-conversion

            int days, seconds;
            if (!ASN1_TIME_diff(&days, &seconds, nullptr, t))
            {
                throw AcmeException("Failure in ASN1_TIME_diff");
            }

            // Hackery here, since the call to time(0) will not necessarily match
            // the equivilent call openssl just made in the 'diff' call above.
            // Nonetheless, it'll be close at worst.
            return ::time(0) + seconds + days * 3600 * 24;
#endif
        });
}

string Certificate::getExpiryDisplay() const
{
    return extractExpiryData<string>(*this, [](const ASN1_TIME * t)
        {
            BIOptr b(BIO_new(BIO_s_mem()));
            if (!ASN1_TIME_print(*b, t))
            {
                throw AcmeException("Failure in ASN1_TIME_print");
            }

            return toString(*b);
        });
}

}
