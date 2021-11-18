#include "acme-lw.hpp"

using namespace acme_lw;

AcmeClient::AcmeClient(
            std::string privateKey,
            std::string newAccountUrl,
            std::string newOrderUrl,
            std::string newNonceUrl,
            std::string eab_kid,
            std::string eab_hmac)
    : impl_(std::make_unique<AcmeClientImpl>(
        std::move(privateKey),
        std::move(newAccountUrl),
        std::move(newOrderUrl),
        std::move(newNonceUrl),
        std::move(eab_kid),
        std::move(eab_hmac)
    ))
{
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

std::string Certificate::getExpiryDisplay() const
{
    return extractExpiryData<std::string>(*this, [](const ASN1_TIME * t)
        {
            BIOptr b(BIO_new(BIO_s_mem()));
            if (!ASN1_TIME_print(b.get(), t))
            {
                throw AcmeException("Failure in ASN1_TIME_print");
            }

            return toString(b.get());
        });
}
