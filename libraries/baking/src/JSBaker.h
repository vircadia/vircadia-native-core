#ifndef hifi_JSBaker_h
#define hifi_JSBaker_h

#include <QtCore/QFutureSynchronizer>
#include <QtCore/QDir>
#include <QtCore/QUrl>

#include "Baker.h"
#include "TextureBaker.h"
#include "ModelBakingLoggingCategory.h"

static const QString BAKED_JS_EXTENSION = ".baked.js";

class JSBaker : public Baker {
    Q_OBJECT

public:
    JSBaker(const QUrl& jsURL, const QString& bakedOutputDir);

public slots:
    virtual void bake() override;

signals:
    void sourceCopyReadyToLoad();

private slots:
    void bakeSourceCopy();


private :
    
    QUrl _jsURL;
    QString _bakedOutputDir;
    QDir _tempDir;
    QString _originalJSFilePath;
    QString _bakedJSFilePath;
    QByteArray _buffer;

    void loadSourceJS();
    void importScript();
    
    void exportScript(QFile*);
    void bakeScript(QFile*);
    void handleSingleLineComments(QTextStream*);
    void handleMultiLineComments(QTextStream*);

    bool isAlphanum(QChar);
    bool omitSpace(QChar, QChar);
    bool isNonAscii(QChar c);
    bool isSpecialChar(QChar c);
    bool isSpecialCharPre(QChar c);
    bool isSpecialCharPost(QChar c);
    bool omitNewLine(QChar, QChar);

};

#endif // !hifi_JSBaker_h