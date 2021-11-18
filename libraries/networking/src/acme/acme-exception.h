#ifndef VIRCADIA_LIBRARIES_NETWORKING_SRC_ACME_ACME_EXCEPTION_H
#define VIRCADIA_LIBRARIES_NETWORKING_SRC_ACME_ACME_EXCEPTION_H

#include <exception>
#include <string>

namespace acme_lw
{

class AcmeException : public std::exception
{
public:
    AcmeException(const std::string&);

    virtual const char * what() const noexcept override;

private:
    std::string what_;
};

}

#endif /* end of include guard */
