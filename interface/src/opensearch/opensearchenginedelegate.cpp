#include "opensearchenginedelegate.h"

/*!
    \class OpenSearchEngineDelegate
    \brief An abstract class providing custom processing of specific activities.

    OpenSearchEngineDelegate is an abstract class that can be subclassed and set on
    an OpenSearchEngine. It allows to customize some parts of the default implementation
    or even extend it with missing bits.

    Currently subclasses can only provide a custom way of handling search requests by
    reimplementing the performSearchRequest() method.

    \sa OpenSearchEngine
*/

/*!
    Constructs the delegate.
*/
OpenSearchEngineDelegate::OpenSearchEngineDelegate()
{
}

/*!
    Destructs the delegate.
*/
OpenSearchEngineDelegate::~OpenSearchEngineDelegate()
{
}

/*!
    \fn void performSearchRequest(const QNetworkRequest &request,
    QNetworkAccessManager::Operation operation, const QByteArray &data) = 0

    This method will be used after OpenSearchEngine::requestResults() is called.

    For example, a console application that uses the OpenSearchEngine class to generate
    a search URL for a search term supplied by the user would reimplement the function
    and forward the request to e.g. a web browser.

    Likewise, a web browser that uses the OpenSearchEngine class to support multiple search
    engines e.g. in a toolbar would perform the request and navigate to the search results site.
*/
