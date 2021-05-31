//
//  WebRTCDataChannels.cpp
//  libraries/networking/src/webrtc
//
//  Created by David Rowe on 21 May 2021.
//  Copyright 2021 Vircadia contributors.
//

#include "WebRTCDataChannels.h"

#if defined(WEBRTC_DATA_CHANNELS)

#include <QJsonDocument>
#include <QJsonObject>

#include "../NetworkLogging.h"


// References:
// - https://webrtc.github.io/webrtc-org/native-code/native-apis/
// - https://webrtc.googlesource.com/src/+/master/api/peer_connection_interface.h

const std::string ICE_SERVER_URI = "stun://ice.vircadia.com:7337";

#define WEBRTC_DEBUG


void WDCSetSessionDescriptionObserver::OnSuccess() {
#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "WDCSetSessionDescriptionObserver::OnSuccess()";
#endif
}

void WDCSetSessionDescriptionObserver::OnFailure(RTCError error) {
#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "WDCSetSessionDescriptionObserver::OnFailure() :" << error.message();
#endif
}


WDCCreateSessionDescriptionObserver::WDCCreateSessionDescriptionObserver(WDCConnection* parent) :
    _parent(parent)
{ }

void WDCCreateSessionDescriptionObserver::OnSuccess(SessionDescriptionInterface* description) {
#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "WDCCreateSessionDescriptionObserver::OnSuccess()";
#endif
    _parent->sendAnswer(description);
    _parent->setLocalDescription(description);
}

void WDCCreateSessionDescriptionObserver::OnFailure(RTCError error) {
#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "WDCCreateSessionDescriptionObserver::OnFailure() :" << error.message();
#endif
}


WDCPeerConnectionObserver::WDCPeerConnectionObserver(WDCConnection* parent) :
    _parent(parent)
{ }

void WDCPeerConnectionObserver::OnSignalingChange(PeerConnectionInterface::SignalingState newState) {
#ifdef WEBRTC_DEBUG
    QStringList states{
        "Stable",
        "HaveLocalOffer",
        "HaveLocalPrAnswer",
        "HaveRemoteOffer",
        "HaveRemotePrAnswer",
        "Closed"
    };
    qCDebug(networking_webrtc) << "WDCPeerConnectionObserver::OnSignalingChange()" << newState << states[newState];
#endif
}

void WDCPeerConnectionObserver::OnRenegotiationNeeded() {
#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "WDCPeerConnectionObserver::OnRenegotiationNeeded()";
#endif
}

void WDCPeerConnectionObserver::OnIceGatheringChange(PeerConnectionInterface::IceGatheringState newState) {
#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "WDCPeerConnectionObserver::OnIceGatheringChange()" << newState;
#endif
}

void WDCPeerConnectionObserver::OnIceCandidate(const IceCandidateInterface* candidate) {
#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "WDCPeerConnectionObserver::OnIceCandidate()";
#endif
    _parent->sendIceCandidate(candidate);
}

void WDCPeerConnectionObserver::OnDataChannel(rtc::scoped_refptr<DataChannelInterface> dataChannel) {
#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "WDCPeerConnectionObserver::OnDataChannel()";
#endif
    _parent->onDataChannelOpened(dataChannel);
}

void WDCPeerConnectionObserver::OnConnectionChange(PeerConnectionInterface::PeerConnectionState newState) {
}


WDCDataChannelObserver::WDCDataChannelObserver(WDCConnection* parent) :
    _parent(parent)
{ }

void WDCDataChannelObserver::OnStateChange() {
#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "WDCDataChannelObserver::OnStateChange()";
#endif
    _parent->onDataChannelStateChanged();
}

void WDCDataChannelObserver::OnMessage(const DataBuffer& buffer) {
#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "WDCDataChannelObserver::OnMessage()";
#endif
    _parent->onDataChannelMessageReceived(buffer);
}


WDCConnection::WDCConnection(quint16 webSocketID, WebRTCDataChannels* parent) :
    _webSocketID(webSocketID),
    _parent(parent)
{
#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "WebRTCDataChannels::WebRTCDataChannels()";
#endif

    // Create observers.
    _setSessionDescriptionObserver = new rtc::RefCountedObject<WDCSetSessionDescriptionObserver>();
    _createSessionDescriptionObserver = new rtc::RefCountedObject<WDCCreateSessionDescriptionObserver>(this);
    _dataChannelObserver = std::make_shared<WDCDataChannelObserver>(this);
    _peerConnectionObserver = std::make_shared<WDCPeerConnectionObserver>(this);

    // Create new peer connection.
    _peerConnection = _parent->createPeerConnection(_peerConnectionObserver);
};

void WDCConnection::setRemoteDescription(QJsonObject& description) {
#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "WDCConnection::setRemoteDescription() :" << description;
#endif

    SdpParseError sdpParseError;
    auto sessionDescription = CreateSessionDescription(
        description.value("type").toString().toStdString(),
        description.value("sdp").toString().toStdString(),
        &sdpParseError);
    if (!sessionDescription) {
        qCWarning(networking_webrtc) << "Error creating WebRTC remote description:"
            << QString::fromStdString(sdpParseError.description);
        return;
    }

#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "3. Set remote description:" << sessionDescription;
#endif
    _peerConnection->SetRemoteDescription(_setSessionDescriptionObserver, sessionDescription);
}

void WDCConnection::createAnswer() {
#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "WDCConnection::createAnswer()";
    qCDebug(networking_webrtc) << "4.a Create answer";
#endif
    _peerConnection->CreateAnswer(_createSessionDescriptionObserver, PeerConnectionInterface::RTCOfferAnswerOptions());
}

void WDCConnection::sendAnswer(SessionDescriptionInterface* description) {
#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "WDCConnection::sendAnswer()";
    qCDebug(networking_webrtc) << "4.b Send answer to the remote peer";
#endif

    QJsonObject jsonDescription;
    std::string descriptionString;
    description->ToString(&descriptionString);
    jsonDescription.insert("sdp", QString::fromStdString(descriptionString));
    jsonDescription.insert("type", "answer");

    QJsonObject jsonWebRTCPayload;
    jsonWebRTCPayload.insert("description", jsonDescription);

    QJsonObject jsonObject;
    jsonObject.insert("from", QString(_parent->getNodeType()));
    jsonObject.insert("to", _webSocketID);
    jsonObject.insert("data", jsonWebRTCPayload);

    _parent->sendSignalingMessage(jsonObject);
}

void WDCConnection::setLocalDescription(SessionDescriptionInterface* description) {
#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "WDCConnection::setLocalDescription()";
    qCDebug(networking_webrtc) << "5. Set local description";
#endif
    _peerConnection->SetLocalDescription(_setSessionDescriptionObserver, description);
}

void WDCConnection::addIceCandidate(QJsonObject& data) {
#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "WDCConnection::addIceCandidate()";
#endif

    SdpParseError sdpParseError;
    auto iceCandidate = CreateIceCandidate(
        data.value("sdpMid").toString().toStdString(),
        data.value("sdpMLineIndex").toInt(),
        data.value("candidate").toString().toStdString(),
        &sdpParseError);
    if (!iceCandidate) {
        qCWarning(networking_webrtc) << "Error adding WebRTC ICE candidate:"
            << QString::fromStdString(sdpParseError.description);
        return;
    }

#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "6. Add ICE candidate";
#endif
    _peerConnection->AddIceCandidate(iceCandidate);
}

void WDCConnection::sendIceCandidate(const IceCandidateInterface* candidate) {
#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "WDCConnection::sendIceCandidate()";
#endif

    std::string candidateString;
    candidate->ToString(&candidateString);
    QJsonObject jsonCandidate;
    jsonCandidate.insert("candidate", QString::fromStdString(candidateString));
    jsonCandidate.insert("sdpMid", QString::fromStdString(candidate->sdp_mid()));
    jsonCandidate.insert("sdpMLineIndex", candidate->sdp_mline_index());

    QJsonObject jsonWebRTCData;
    jsonWebRTCData.insert("candidate", jsonCandidate);

    QJsonObject jsonObject;
    jsonObject.insert("from", QString(_parent->getNodeType()));
    jsonObject.insert("to", _webSocketID);
    jsonObject.insert("data", jsonWebRTCData);
    QJsonDocument jsonDocument = QJsonDocument(jsonObject);

#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "7. Send ICE candidate to the remote peer";
#endif
    _parent->sendSignalingMessage(jsonObject);
}

void WDCConnection::onDataChannelOpened(rtc::scoped_refptr<DataChannelInterface> dataChannel) {
#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "WDCConnection::onDataChannelOpened() :"
        << dataChannel->id()
        << QString::fromStdString(dataChannel->label())
        << QString::fromStdString(dataChannel->protocol())
        << dataChannel->negotiated()
        << dataChannel->maxRetransmitTime()
        << dataChannel->maxRetransmits()
        << dataChannel->maxPacketLifeTime().value_or(-1)
        << dataChannel->maxRetransmitsOpt().value_or(-1);
#endif

    _dataChannel = dataChannel;
    _dataChannelID = dataChannel->id();
    _dataChannel->RegisterObserver(_dataChannelObserver.get());

    _parent->onDataChannelOpened(this, _dataChannelID);
}

void WDCConnection::onDataChannelStateChanged() {
    auto state = _dataChannel->state();
#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "WDCConnection::dataChannelStateChanged() :" << (int)state
        << DataChannelInterface::DataStateString(state);
#endif
    if (state == DataChannelInterface::kClosed) {
        _parent->onDataChannelClosed(this, _dataChannelID);
    }
}

void WDCConnection::onDataChannelMessageReceived(const DataBuffer& buffer) {
#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "WDCConnection::onDataChannelMessageReceived()";
#endif

    auto byteArray = QByteArray(buffer.data.data<char>(), (int)buffer.data.size());

    // Echo message back to sender.
    if (byteArray.startsWith("echo:")) {
#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "Echo message back";
#endif
        _parent->sendDataMessage(_dataChannelID, byteArray);  // Use parent method to exercise the code stack.
        return;
    }

    _parent->emitDataMessage(_dataChannelID, byteArray);
}

bool WDCConnection::sendDataMessage(const DataBuffer& buffer) {
#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "WDCConnection::sendDataMessage()";
#endif
    const int MAX_WEBRTC_BUFFER_SIZE = 16 * 1024 * 1024;  // 16MB
    if (_dataChannel->buffered_amount() + buffer.size() > MAX_WEBRTC_BUFFER_SIZE) {
        // Don't send, otherwise the data channel will be closed.
        return false;
    }
    return _dataChannel->Send(buffer);
}


WebRTCDataChannels::WebRTCDataChannels(NodeType_t nodeType, QObject* parent) :
    _nodeType(nodeType),
    _parent(parent)
{
#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "WebRTCDataChannels::WebRTCDataChannels()";
#endif

    // Create a peer connection factory.
#ifdef WEBRTC_DEBUG
    // Numbers are per WebRTC's peer_connection_interface.h.
    qCDebug(networking_webrtc) << "1. Create a new PeerConnectionFactoryInterface";
#endif
    _rtcNetworkThread = rtc::Thread::CreateWithSocketServer();
    _rtcNetworkThread->Start();
    _rtcWorkerThread = rtc::Thread::Create();
    _rtcWorkerThread->Start();
    _rtcSignalingThread = rtc::Thread::Create();
    _rtcSignalingThread->Start();
    PeerConnectionFactoryDependencies dependencies;
    dependencies.network_thread = _rtcNetworkThread.get();
    dependencies.worker_thread = _rtcWorkerThread.get();
    dependencies.signaling_thread = _rtcSignalingThread.get();
    _peerConnectionFactory = CreateModularPeerConnectionFactory(std::move(dependencies));
    if (!_peerConnectionFactory) {
        qCWarning(networking_webrtc) << "Failed to create WebRTC peer connection factory";
    }
}

WebRTCDataChannels::~WebRTCDataChannels() {
    QHashIterator<int, WDCConnection*> i(_connectionsByDataChannel);
    while (i.hasNext()) {
        i.next();
        delete i.value();
    }
    _connectionsByWebSocket.clear();
    _connectionsByDataChannel.clear();

    _peerConnectionFactory = nullptr;
    _rtcSignalingThread->Stop();
    _rtcSignalingThread = nullptr;
    _rtcWorkerThread->Stop();
    _rtcWorkerThread = nullptr;
    _rtcNetworkThread->Stop();
    _rtcNetworkThread = nullptr;
}

void WebRTCDataChannels::onDataChannelOpened(WDCConnection* connection, int dataChannelID) {
#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "WebRTCDataChannels::onDataChannelOpened() :" << dataChannelID;
#endif
    _connectionsByDataChannel.insert(dataChannelID, connection);
}

void WebRTCDataChannels::onDataChannelClosed(WDCConnection* connection, int dataChannelID) {
#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "WebRTCDataChannels::onDataChannelClosed() :" << dataChannelID;
#endif

    // Delete WDCConnection.
    _connectionsByWebSocket.remove(connection->getWebSocketID());
    _connectionsByDataChannel.remove(dataChannelID);
    delete connection;
}

void WebRTCDataChannels::onSignalingMessage(const QJsonObject& message) {
#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "WebRTCDataChannel::onSignalingMessage()" << message;
#endif

    // Validate message.
    const int MAX_DEBUG_DETAIL_LENGTH = 64;
    auto data = message.value("data").isObject() ? message.value("data").toObject() : QJsonObject();
    int from = message.value("from").isDouble() ? (quint16)(message.value("from").toInt()) : 0;
    if (from <= 0 || from > MAXUINT16 || !data.contains("description") && !data.contains("candidate")) {
        qCWarning(networking_webrtc) << "Unexpected signaling message:"
            << QJsonDocument(message).toJson(QJsonDocument::Compact).left(MAX_DEBUG_DETAIL_LENGTH);
        return;
    }

    // Find or create a connection.
    WDCConnection* connection;
    if (_connectionsByWebSocket.contains(from)) {
        connection = _connectionsByWebSocket.value(from);
    } else {
        connection = new WDCConnection(from, this);
        _connectionsByWebSocket.insert(from, connection);
    }

    // Set the remote description and reply with an answer.
    if (data.contains("description")) {
        auto description = data.value("description").toObject();
        if (description.value("type").toString() == "offer") {
            connection->setRemoteDescription(description);
            connection->createAnswer();
        } else {
            qCWarning(networking_webrtc) << "Unexpected signaling description:"
                << QJsonDocument(description).toJson(QJsonDocument::Compact).left(MAX_DEBUG_DETAIL_LENGTH);
        }
    }

    // Add a remote ICE candidate.
    if (data.contains("candidate")) {
        connection->addIceCandidate(data);
    }

}

void WebRTCDataChannels::sendSignalingMessage(const QJsonObject& message) {
#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "WebRTCDataChannels::sendSignalingMessage() :" << QJsonDocument(message).toJson(QJsonDocument::Compact);
#endif
    emit signalingMessage(message);
}

void WebRTCDataChannels::emitDataMessage(int dataChannelID, const QByteArray& byteArray) {
#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "WebRTCDataChannels::emitDataMessage() :" << dataChannelID;
#endif
    emit dataMessage(dataChannelID, byteArray);
}

bool WebRTCDataChannels::sendDataMessage(int dataChannelID, const QByteArray& byteArray) {
#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "WebRTCDataChannels::sendDataMessage() :" << dataChannelID;
#endif

    // Find connection.
    if (!_connectionsByDataChannel.contains(dataChannelID)) {
        qCWarning(networking_webrtc) << "Could not find data channel to send message on!";
        return false;
    }

    auto connection = _connectionsByDataChannel.value(dataChannelID);
    DataBuffer buffer(byteArray.toStdString(), true);
    return connection->sendDataMessage(buffer);
}

rtc::scoped_refptr<PeerConnectionInterface> WebRTCDataChannels::createPeerConnection(
        const std::shared_ptr<WDCPeerConnectionObserver> peerConnectionObserver) {
#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "WebRTCDataChannels::createPeerConnection()";
#endif

    PeerConnectionInterface::RTCConfiguration configuration;
    PeerConnectionInterface::IceServer iceServer;
    iceServer.uri = ICE_SERVER_URI;
    configuration.servers.push_back(iceServer);

#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "2. Create a new PeerConnection";
#endif
    return _peerConnectionFactory->CreatePeerConnection(configuration, nullptr, nullptr, peerConnectionObserver.get());
}


#endif // WEBRTC_DATA_CHANNELS
