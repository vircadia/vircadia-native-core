#ifndef VIRCADIA_LIBRARIES_NETWORKING_SRC_SSLCOMMON_H
#define VIRCADIA_LIBRARIES_NETWORKING_SRC_SSLCOMMON_H

#include <QString>

struct CertificatePaths {
    QString cert;
    QString key;
    QString trustedAuthorities;
};

#endif /* end of include guard */
