#pragma once

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
