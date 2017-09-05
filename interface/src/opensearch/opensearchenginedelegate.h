#ifndef OPENSEARCHENGINEDELEGATE_H
#define OPENSEARCHENGINEDELEGATE_H

#include <qnetworkaccessmanager.h>
#include <qnetworkrequest.h>

class OpenSearchEngineDelegate
{
public:
    OpenSearchEngineDelegate();
    virtual ~OpenSearchEngineDelegate();

    virtual void performSearchRequest(const QNetworkRequest &request,
                                      QNetworkAccessManager::Operation operation,
                                      const QByteArray &data) = 0;
};

#endif // OPENSEARCHENGINEDELEGATE_H
