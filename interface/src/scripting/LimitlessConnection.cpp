#include <QJsonDocument>
#include <QJsonArray>
#include <src/InterfaceLogging.h>
#include <src/ui/AvatarInputs.h>
#include "LimitlessConnection.h"
#include "LimitlessVoiceRecognitionScriptingInterface.h"

LimitlessConnection::LimitlessConnection() :
        _streamingAudioForTranscription(false)
{
}

void LimitlessConnection::startListening(QString authCode) {
    _transcribeServerSocket.reset(new QTcpSocket(this));
    connect(_transcribeServerSocket.get(), &QTcpSocket::readyRead, this,
            &LimitlessConnection::transcriptionReceived);
    connect(_transcribeServerSocket.get(), &QTcpSocket::disconnected, this, [this](){stopListening();});

    static const auto host = "gserv_devel.studiolimitless.com";
    _transcribeServerSocket->connectToHost(host, 1407);
    _transcribeServerSocket->waitForConnected();
    QString requestHeader = QString::asprintf("Authorization: %s\r\nfs: %i\r\n",
                                              authCode.toLocal8Bit().data(), AudioConstants::SAMPLE_RATE);
    qCDebug(interfaceapp) << "Sending Limitless Audio Stream Request: " << requestHeader;
    _transcribeServerSocket->write(requestHeader.toLocal8Bit());
    _transcribeServerSocket->waitForBytesWritten();
}

void LimitlessConnection::stopListening() {
    emit onFinishedSpeaking(_currentTranscription);
    _streamingAudioForTranscription = false;
    _currentTranscription = "";
    if (!isConnected())
        return;
    _transcribeServerSocket->close();
    disconnect(_transcribeServerSocket.get(), &QTcpSocket::readyRead, this,
            &LimitlessConnection::transcriptionReceived);
    _transcribeServerSocket.release()->deleteLater();
    disconnect(DependencyManager::get<AudioClient>().data(), &AudioClient::inputReceived, this,
            &LimitlessConnection::audioInputReceived);
    qCDebug(interfaceapp) << "Connection to Limitless Voice Server closed.";
}

void LimitlessConnection::audioInputReceived(const QByteArray& inputSamples) {
    if (isConnected()) {
        _transcribeServerSocket->write(inputSamples.data(), inputSamples.size());
        _transcribeServerSocket->waitForBytesWritten();
    }
}

void LimitlessConnection::transcriptionReceived() {
    while (_transcribeServerSocket && _transcribeServerSocket->bytesAvailable() > 0) {
        const QByteArray data = _transcribeServerSocket->readAll();
        _serverDataBuffer.append(data);
        int begin = _serverDataBuffer.indexOf('<');
        int end = _serverDataBuffer.indexOf('>');
        while (begin > -1 && end > -1) {
            const int len = end - begin;
            const QByteArray serverMessage = _serverDataBuffer.mid(begin+1, len-1);
            if (serverMessage.contains("1407")) {
                qCDebug(interfaceapp) << "Limitless Speech Server denied the request.";
                // Don't spam the server with further false requests please.
                DependencyManager::get<LimitlessVoiceRecognitionScriptingInterface>()->setListeningToVoice(true);
                stopListening();
                return;
            } else if (serverMessage.contains("1408")) {
                qCDebug(interfaceapp) << "Limitless Audio request authenticated!";
                _serverDataBuffer.clear();
                connect(DependencyManager::get<AudioClient>().data(), &AudioClient::inputReceived, this,
                        &LimitlessConnection::audioInputReceived);
                return;
            }
            QJsonObject json = QJsonDocument::fromJson(serverMessage.data()).object();
            _serverDataBuffer.remove(begin, len+1);
            _currentTranscription = json["alternatives"].toArray()[0].toObject()["transcript"].toString();
            emit onReceivedTranscription(_currentTranscription);
            if (json["isFinal"] == true) {
                qCDebug(interfaceapp) << "Final transcription: " << _currentTranscription;
                stopListening();
                return;
            }
            begin = _serverDataBuffer.indexOf('<');
            end = _serverDataBuffer.indexOf('>');
        }
    }
}

bool LimitlessConnection::isConnected() const {
    return _transcribeServerSocket.get() && _transcribeServerSocket->isWritable()
    && _transcribeServerSocket->state() != QAbstractSocket::SocketState::UnconnectedState;
}
