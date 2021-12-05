#ifndef VIRCADIA_LIBRARIES_NETWORKING_SRC_ACME_ACME_LW_H
#define VIRCADIA_LIBRARIES_NETWORKING_SRC_ACME_ACME_LW_H

#include "acme-exception.h"

#include <memory>
#include <vector>
#include <unordered_map>
#include <chrono>

namespace acme_lw
{

struct Certificate
{
    std::string fullchain;
    std::string privkey;

    // Note that neither of the 'Expiry' calls below require 'privkey'
    // to be set; they only rely on 'fullchain'.

    /**
        Returns the number of seconds since 1970, i.e., epoch time.

        Due to openssl quirkiness on older versions (< 1.1.1?) there
        might be a little drift from a strictly accurate result, but
        it will be close enough for the purpose of determining
        whether the certificate needs to be renewed.
    */
    ::time_t getExpiry() const;

    /**
        Returns the 'Not After' result that openssl would display if
        running the following command.

            openssl x509 -noout -in fullchain.pem -text

        For example:

            May  6 21:15:03 2018 GMT
    */
    std::string getExpiryDisplay() const;
};

struct AcmeClientImpl;

class AcmeClient
{
public:
    AcmeClient(
            std::string privateKey,
            std::string newAccountUrl,
            std::string newOrderUrl,
            std::string newNonceUrl,
            std::string eab_kid = "",
            std::string eab_hmac = "");

    /**
        The implementation of this function allows Let's Encrypt to
        verify that the requestor has control of the domain name.

        The callback may be called once for each domain name in the
        'issueCertificate' call. The callback should do whatever is
        needed so that a GET on the 'url' returns the 'keyAuthorization',
        (which is what the Acme protocol calls the expected response.)

        Note that this function may not be called in cases where
        Let's Encrypt already believes the caller has control
        of the domain name.
    */
    typedef void (*Callback) (  const std::string& domainName,
                                const std::string& url,
                                const std::string& keyAuthorization);

    std::unique_ptr<AcmeClientImpl> impl_;
};

//TODO: document/specify callback parameters
// maybe provide additional interface with std::function callback to restore interface/implementation division
/**
    The signingKey is the Acme account private key used to sign
    requests to the acme CA, in pem format.
*/
template <typename Callback>
void init(Callback, std::string signingKey,
    std::string directoryUrl = "https://acme-staging-v02.api.letsencrypt.org/directory");

template <typename Callback>
void sendRequest(Callback, AcmeClient,
    std::string url, std::string payload, const char* header = nullptr);

template <typename Callback>
void createAccount(Callback, AcmeClient);

template <typename Callback, typename ChallengeCallback>
void orderCertificate(Callback, ChallengeCallback, AcmeClient,
    std::vector<std::string> domains);

template <typename Callback>
void retrieveCertificate(Callback, AcmeClient, std::vector<std::string> domains, std::vector<std::string> challenges, std::string url, std::string finalizeUrl);

template <typename Callback>
void waitForGet(Callback, std::string url, std::chrono::milliseconds timeout, std::chrono::milliseconds interval);

}

#endif /* end of include guard */
