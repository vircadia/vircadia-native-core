#include "acme-exception.h"

#include <openssl/err.h>

using namespace std;

namespace acme_lw
{

AcmeException::AcmeException(const string& s)
{
    // We assume the error is going to be from openssl. If it's not, we just
    // use the string passed in as the error message.
    unsigned long err = ERR_get_error();
    while (ERR_get_error()); // clear any previous errors we didn't deal with for some reason;

    if (err)
    {
        constexpr int buffSize = 120;
        char buffer[buffSize];
        ERR_error_string(err, buffer);

        what_ = (s.size() ? s + ": " : s) + "OpenSSL error (" + to_string(err) + "): " + buffer;
    }
    else
    {
        what_ = s;
    }
}

const char * AcmeException::what() const noexcept
{
    return  what_.c_str();
}

}
