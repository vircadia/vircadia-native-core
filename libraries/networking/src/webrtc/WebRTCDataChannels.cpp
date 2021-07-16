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
const int MAX_WEBRTC_BUFFER_SIZE = 16777216;  // 16MB

// #define WEBRTC_DEBUG

using namespace webrtc;


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
#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "WDCPeerConnectionObserver::OnConnectionChange()" << (uint)newState;
#endif
    _parent->onPeerConnectionStateChanged(newState);
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


WDCConnection::WDCConnection(WebRTCDataChannels* parent, quint16 webSocketID) :
    _parent(parent),
    _webSocketID(webSocketID)
{
#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "WDCConnection::WDCConnection() :" << webSocketID;
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

void WDCConnection::onPeerConnectionStateChanged(PeerConnectionInterface::PeerConnectionState state) {
#ifdef WEBRTC_DEBUG
    const char* STATES[] = {
        "New",
        "Connecting",
        "Connected",
        "Disconnected",
        "Failed",
        "Closed"
    };
    qCDebug(networking_webrtc) << "WDCConnection::onPeerConnectionStateChanged() :" << (int)state << STATES[(int)state];
#endif
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
    _dataChannelID = _parent->getNewDataChannelID();  // Not dataChannel->id() because it's only unique per peer connection.
    _dataChannel->RegisterObserver(_dataChannelObserver.get());

    _parent->onDataChannelOpened(this, _dataChannelID);
}

void WDCConnection::onDataChannelStateChanged() {
    auto state = _dataChannel->state();
#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "WDCConnection::onDataChannelStateChanged() :" << (int)state
        << DataChannelInterface::DataStateString(state);
#endif
    if (state == DataChannelInterface::kClosed) {
        // Close data channel.
        _dataChannel->UnregisterObserver();
        _dataChannelObserver = nullptr;
        _dataChannel = nullptr;
#ifdef WEBRTC_DEBUG
        qCDebug(networking_webrtc) << "Disposed of data channel";
#endif
        // Close peer connection.
        _parent->closePeerConnection(this);
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

qint64 WDCConnection::getBufferedAmount() const {
#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "WDCConnection::getBufferedAmount()";
#endif
    return _dataChannel->buffered_amount();
}

bool WDCConnection::sendDataMessage(const DataBuffer& buffer) {
#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "WDCConnection::sendDataMessage()";
#endif
    if (_dataChannel->buffered_amount() + buffer.size() > MAX_WEBRTC_BUFFER_SIZE) {
        // Don't send, otherwise the data channel will be closed.
        return false;
    } else {
        qCDebug(networking_webrtc) << "WebRTC send buffer overflow";
    }
    return _dataChannel->Send(buffer);
}

void WDCConnection::closePeerConnection() {
#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "WDCConnection::closePeerConnection()";
#endif
    _peerConnection->Close();
    _peerConnection = nullptr;
    _peerConnectionObserver = nullptr;
#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "Disposed of peer connection";
#endif
}


WebRTCDataChannels::WebRTCDataChannels(QObject* parent, NodeType_t nodeType) :
    QObject(parent),
    _parent(parent),
    _nodeType(nodeType)
{
#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "WebRTCDataChannels::WebRTCDataChannels()" << nodeType << NodeType::getNodeTypeName(nodeType);
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

    // Set up mechanism for closing peer connections.
    connect(this, &WebRTCDataChannels::closePeerConnectionSoon, this, &WebRTCDataChannels::closePeerConnectionNow);
}

WebRTCDataChannels::~WebRTCDataChannels() {
#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "WebRTCDataChannels::~WebRTCDataChannels()";
#endif
    reset();
    _peerConnectionFactory = nullptr;
    _rtcSignalingThread->Stop();
    _rtcSignalingThread = nullptr;
    _rtcWorkerThread->Stop();
    _rtcWorkerThread = nullptr;
    _rtcNetworkThread->Stop();
    _rtcNetworkThread = nullptr;
}

void WebRTCDataChannels::reset() {
    QHashIterator<quint16, WDCConnection*> i(_connectionsByDataChannel);
    while (i.hasNext()) {
        i.next();
        delete i.value();
    }
    _connectionsByWebSocket.clear();
    _connectionsByDataChannel.clear();
}

quint16 WebRTCDataChannels::getNewDataChannelID() {
    static const int QUINT16_LIMIT = std::numeric_limits<uint16_t>::max() + 1;
    _lastDataChannelID = std::max((_lastDataChannelID + 1) % QUINT16_LIMIT, 1);
    return _lastDataChannelID;
}

void WebRTCDataChannels::onDataChannelOpened(WDCConnection* connection, quint16 dataChannelID) {
#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "WebRTCDataChannels::onDataChannelOpened() :" << dataChannelID;
#endif
    _connectionsByDataChannel.insert(dataChannelID, connection);
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
        connection = new WDCConnection(this, from);
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
    qCDebug(networking_webrtc) << "WebRTCDataChannels::emitDataMessage() :" << dataChannelID << byteArray;
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

/// @brief Gets the number of bytes waiting to be written on a data channel.
/// @param port The data channel ID.
/// @return The number of bytes waiting to be written on the data channel.
qint64 WebRTCDataChannels::getBufferedAmount(int dataChannelID) const {
    auto connection = _connectionsByDataChannel.value(dataChannelID);
    return connection->getBufferedAmount();
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
    qCDebug(networking_webrtc) << "2. Create a new peer connection";
#endif
    PeerConnectionDependencies dependencies(peerConnectionObserver.get());
    auto result = _peerConnectionFactory->CreatePeerConnection(configuration, std::move(dependencies));
#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "Created peer connection";
#endif
    return result;
}


void WebRTCDataChannels::closePeerConnection(WDCConnection* connection) {
#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "WebRTCDataChannels::closePeerConnection()";
#endif
    // Use Qt's signals/slots mechanism to close the peer connection on its own call stack, separate from the DataChannel 
    // callback that initiated the peer connection.
    // https://bugs.chromium.org/p/webrtc/issues/detail?id=3721
    emit closePeerConnectionSoon(connection);
}


void WebRTCDataChannels::closePeerConnectionNow(WDCConnection* connection) {
#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "WebRTCDataChannels::closePeerConnectionNow()";
#endif
    // Close the peer connection.
    connection->closePeerConnection();

    // Delete the WDCConnection.
    _connectionsByWebSocket.remove(connection->getWebSocketID());
    _connectionsByDataChannel.remove(connection->getDataChannelID());
    delete connection;
#ifdef WEBRTC_DEBUG
    qCDebug(networking_webrtc) << "Disposed of connection";
#endif
}

#endif // WEBRTC_DATA_CHANNELS
