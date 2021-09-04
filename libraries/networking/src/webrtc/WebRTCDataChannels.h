//
//  WebRTCDataChannels.h
//  libraries/networking/src/webrtc
//
//  Created by David Rowe on 21 May 2021.
//  Copyright 2021 Vircadia contributors.
//

#ifndef vircadia_WebRTCDataChannels_h
#define vircadia_WebRTCDataChannels_h

#include <shared/WebRTC.h>

#if defined(WEBRTC_DATA_CHANNELS)


#include <QObject>
#include <QHash>

#undef emit  // Avoid conflict between Qt signals/slots and the WebRTC library's.
#include <api/peer_connection_interface.h>
#define emit

#include "../NodeType.h"

class WebRTCDataChannels;
class WDCConnection;


/// @addtogroup Networking
/// @{

/// @brief A WebRTC session description observer.
class WDCSetSessionDescriptionObserver : public webrtc::SetSessionDescriptionObserver {
public:

    /// @brief The call to SetLocalDescription or SetRemoteDescription succeeded.
    void OnSuccess() override;

    /// @brief The call to SetLocalDescription or SetRemoteDescription failed.
    /// @param error Error information.
    void OnFailure(webrtc::RTCError error) override;
};


/// @brief A WebRTC create session description observer.
class WDCCreateSessionDescriptionObserver : public webrtc::CreateSessionDescriptionObserver {
public:

    /// @brief Constructs a session description observer.
    /// @param parent The parent connection object.
    WDCCreateSessionDescriptionObserver(WDCConnection* parent);

    /// @brief The call to CreateAnswer succeeded.
    /// @param desc The session description.
    void OnSuccess(webrtc::SessionDescriptionInterface* desc) override;

    /// @brief The call to CreateAnswer failed.
    /// @param error Error information.
    void OnFailure(webrtc::RTCError error) override;

private:
    WDCConnection* _parent;
};


/// @brief A WebRTC peer connection observer.
class WDCPeerConnectionObserver : public webrtc::PeerConnectionObserver {
public:

    /// @brief Constructs a peer connection observer.
    /// @param parent The parent connection object.
    WDCPeerConnectionObserver(WDCConnection* parent);

    /// @brief Called when the SignalingState changes.
    /// @param newState The new signaling state.
    void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState newState) override;

    /// @brief Called when renegotiation is needed. For example, an ICE restart has begun.
    void OnRenegotiationNeeded() override;

    /// @brief Called when the ICE gather state changes.
    /// @param newState The new ICE gathering state.
    void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState newState) override;

    /// @brief Called when a new ICE candidate has been gathered.
    /// @param candidate The new ICE candidate.
    void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;

    /// @brief Called when a remote peer opens a data channel.
    /// @param dataChannel The data channel.
    void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> dataChannel) override;

    /// @brief Called when the peer connection state changes.
    /// @param newState The new peer connection state.
    void OnConnectionChange(webrtc::PeerConnectionInterface::PeerConnectionState newState) override;

private:
    WDCConnection* _parent;
};


/// @brief A WebRTC data channel observer.
class WDCDataChannelObserver : public webrtc::DataChannelObserver {
public:

    /// @brief Constructs a data channel observer.
    /// @param parent The parent connection object.
    WDCDataChannelObserver(WDCConnection* parent);

    /// @brief The data channel state changed.
    void OnStateChange() override;

    /// @brief A data channel message was received.
    /// @param The message received.
    void OnMessage(const webrtc::DataBuffer& buffer) override;

private:
    WDCConnection* _parent;
};


/// @brief A WebRTC data channel connection.
/// @details Opens and manages a WebRTC data channel connection.
class WDCConnection {

public:

    /// @brief Constructs a new WDCConnection and opens a WebRTC data connection.
    /// @param parent The parent WebRTCDataChannels object.
    /// @param webSocketID The signaling channel that initiated the opening of the WebRTC data channel.
    /// @param dataChannelID - The WebRTC data channel ID to assign to this connection.
    WDCConnection(WebRTCDataChannels* parent, quint16 webSocketID, int dataChannelID);

    /// @brief Gets the WebSocket ID.
    /// @return The ID of the WebSocket.
    quint16 getWebSocketID() const { return _webSocketID; }

    /// @brief Gets the WebRTC data channel ID.
    /// @return The WebRTC data channel ID. `-1` if not open yet.
    int getDataChannelID() const { return _dataChannelID; }


    /// @brief Sets the remote session description received from the remote client via the signaling channel.
    /// @param description The remote session description.
    void setRemoteDescription(QJsonObject& description);

    /// @brief Creates an answer to an offer received from the remote client via the signaling channel.
    void createAnswer();

    /// @brief Sends an answer to the remote client via the signaling channel.
    /// @param description The answer.
    void sendAnswer(webrtc::SessionDescriptionInterface* description);
    
    /// @brief Sets the local session description on the WebRTC data channel being connected.
    /// @param description The local session description.
    void setLocalDescription(webrtc::SessionDescriptionInterface* description);
    
    /// @brief Adds an ICE candidate received from the remote client via the signaling channel.
    /// @param data The ICE candidate.
    void addIceCandidate(QJsonObject& data);

    /// @brief Sends an ICE candidate to the remote vlient via the signaling channel.
    /// @param candidate The ICE candidate.
    void sendIceCandidate(const webrtc::IceCandidateInterface* candidate);

    /// @brief Monitors the peer connection state.
    /// @param state The new peer connection state.
    void onPeerConnectionStateChanged(webrtc::PeerConnectionInterface::PeerConnectionState state);

    /// @brief Handles the WebRTC data channel being opened.
    /// @param dataChannel The WebRTC data channel.
    void onDataChannelOpened(rtc::scoped_refptr<webrtc::DataChannelInterface> dataChannel);

    /// @brief Handles a change in the state of the WebRTC data channel.
    void onDataChannelStateChanged();


    /// @brief Handles a message being received on the WebRTC data channel.
    /// @param buffer The message received.
    void onDataChannelMessageReceived(const webrtc::DataBuffer& buffer);

    /// @brief Gets the number of bytes waiting to be sent on the WebRTC data channel.
    /// @return The number of bytes waiting to be sent on the WebRTC data channel.
    qint64 getBufferedAmount() const;


    /// @brief Sends a message on the WebRTC data channel.
    /// @param buffer The message to send.
    /// @return `true` if the message was sent, otherwise `false`.
    bool sendDataMessage(const webrtc::DataBuffer& buffer);

    /// @brief Closes the WebRTC peer connection.
    void closePeerConnection();
    
private:
    WebRTCDataChannels* _parent;
    quint16 _webSocketID { 0 };
    int _dataChannelID { -1 };

    rtc::scoped_refptr<WDCSetSessionDescriptionObserver> _setSessionDescriptionObserver { nullptr };
    rtc::scoped_refptr<WDCCreateSessionDescriptionObserver> _createSessionDescriptionObserver { nullptr };

    std::shared_ptr<WDCDataChannelObserver> _dataChannelObserver { nullptr };
    rtc::scoped_refptr<webrtc::DataChannelInterface> _dataChannel { nullptr };

    std::shared_ptr<WDCPeerConnectionObserver> _peerConnectionObserver { nullptr };
    rtc::scoped_refptr<webrtc::PeerConnectionInterface> _peerConnection { nullptr };
};


/// @brief Manages WebRTC data channels on the domain server or an assignment client that Interface clients can connect to.
///
/// @details Presents multiple individual WebRTC data channels as a single one-to-many WebRTCDataChannels object. Interface
/// clients may use WebRTC data channels for Vircadia protocol network communications instead of UDP.
/// A WebRTCSignalingServer is used in the process of setting up a WebRTC data channel between an Interface client and the
/// domain server or assignment client.
/// The Interface client initiates the connection - including initiating the data channel - and the domain server or assignment
/// client responds.
///
/// Additionally, for debugging purposes, instead of containing a Vircadia protocol payload, a WebRTC message may be an echo
/// request. This is bounced back to the client.
/// 
class WebRTCDataChannels : public QObject {
    Q_OBJECT

public:

    /// @brief Constructs a new WebRTCDataChannels object.
    /// @param parent The parent Qt object.
    WebRTCDataChannels(QObject* parent);

    /// @brief Destroys a WebRTCDataChannels object.
    ~WebRTCDataChannels();

    /// @brief Gets the type of node that the WebRTCDataChannels object is being used in.
    /// @return The type of node.
    NodeType_t getNodeType() {
        return _nodeType;
    }

    /// @brief Immediately closes all connections and resets the socket.
    void reset();
    
    /// @brief Gets a new data channel ID to uniquely identify a WDCConnection.
    /// @details This ID is assigned by WebRTCDataChannels; it is <em>not</em> the WebRTC data channel ID because that is only
    ///     unique within a peer connection.
    /// @return A new data channel ID.
    quint16 getNewDataChannelID();

    /// @brief Gets the data channel ID associated with a WebSocket.
    /// @param webSocketID The WebSocket.
    /// @return The data channel ID associated with the WebSocket if found, `0` if the WebSocket was not found.
    int getDataChannelIDForWebSocket(quint16 webSocketID) const;

    /// @brief Handles a WebRTC data channel opening.
    /// @param connection The WebRTC data channel connection.
    /// @param dataChannelID The WebRTC data channel ID.
    void onDataChannelOpened(WDCConnection* connection, quint16 dataChannelID);

    /// @brief Emits a signalingMessage to be sent to the Interface client.
    /// @param message The WebRTC signaling message to send.
    void sendSignalingMessage(const QJsonObject& message);

    /// @brief Emits a dataMessage received from the Interface client.
    /// @param dataChannelID The WebRTC data channel the message was received on.
    /// @param byteArray The data message received.
    void emitDataMessage(int dataChannelID, const QByteArray& byteArray);

    /// @brief Sends a data message to an Interface client.
    /// @param dataChannelID The WebRTC channel ID of the Interface client.
    /// @param message The data message to send.
    /// @return `true` if the data message was sent, otherwise `false`.
    bool sendDataMessage(int dataChannelID, const QByteArray& message);

    /// @brief Gets the number of bytes waiting to be sent on a data channel.
    /// @param dataChannelID The data channel ID.
    /// @return The number of bytes waiting to be sent on the data channel.
    qint64 getBufferedAmount(int dataChannelID) const;

    /// @brief Creates a new WebRTC peer connection for connecting to an Interface client.
    /// @param peerConnectionObserver An observer to monitor the WebRTC peer connection.
    /// @return The new WebRTC peer connection.
    rtc::scoped_refptr<webrtc::PeerConnectionInterface> createPeerConnection(
        const std::shared_ptr<WDCPeerConnectionObserver> peerConnectionObserver);

    /// @brief Initiates closing the peer connection for a WebRTC data channel.
    /// @details Emits a {@link WebRTCDataChannels.closePeerConnectionSoon} signal which is connected to
    ///     {@link WebRTCDataChannels.closePeerConnectionNow} in order to close the peer connection on a new call stack. This is
    ///     necessary to work around a WebRTC library limitation.
    /// @param connection The WebRTC data channel connection.
    void closePeerConnection(WDCConnection* connection);

public slots:

    /// @brief Handles a WebRTC signaling message received from the Interface client.
    /// @param message The WebRTC signaling message.
    void onSignalingMessage(const QJsonObject& message);

    /// @brief Closes the peer connection for a WebRTC data channel.
    /// @details Used by {@link WebRTCDataChannels.closePeerConnection}.
    /// @param connection The WebRTC data channel connection.
    void closePeerConnectionNow(WDCConnection* connection);

signals:

    /// @brief A WebRTC signaling message to be sent to the Interface client.
    /// @details This message is for the WebRTCSignalingServer to send.
    /// @param message The WebRTC signaling message to send.
    void signalingMessage(const QJsonObject& message);

    /// @brief A WebRTC data message received from the Interface client.
    /// @details This message is for handling at a higher level in the Vircadia protocol.
    /// @param dataChannelID The WebRTC data channel ID.
    /// @param byteArray The Vircadia protocol message.
    void dataMessage(int dataChannelID, const QByteArray& byteArray);

    /// @brief Signals that the peer connection for a WebRTC data channel should be closed.
    /// @details Used by {@link WebRTCDataChannels.closePeerConnection}.
    /// @param connection The WebRTC data channel connection.
    void closePeerConnectionSoon(WDCConnection* connection);

private:

    QObject* _parent;

    NodeType_t _nodeType { NodeType::Unassigned };

    std::unique_ptr<rtc::Thread> _rtcNetworkThread { nullptr };
    std::unique_ptr<rtc::Thread> _rtcWorkerThread { nullptr };
    std::unique_ptr<rtc::Thread> _rtcSignalingThread { nullptr };

    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> _peerConnectionFactory { nullptr };

    quint16 _lastDataChannelID { 0 };

    QHash<quint16, WDCConnection*> _connectionsByWebSocket;
    QHash<quint16, WDCConnection*> _connectionsByDataChannel;
};


/// @}

#endif // WEBRTC_DATA_CHANNELS

#endif // vircadia_WebRTCDataChannels_h
