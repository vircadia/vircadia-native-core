#include "EntityScriptClient.h"
#include "NodeList.h"
#include "NetworkLogging.h"
#include "EntityScriptUtils.h"

#include <QThread>

MessageID EntityScriptClient::_currentID = 0;

GetScriptStatusRequest::GetScriptStatusRequest(QUuid entityID) : _entityID(entityID) {
}

GetScriptStatusRequest::~GetScriptStatusRequest() {

}

void GetScriptStatusRequest::start() {
    auto client = DependencyManager::get<EntityScriptClient>();
    client->getEntityServerScriptStatus(_entityID, [this](bool responseReceived, bool isRunning, EntityScriptStatus status, QString errorInfo) {
        _responseReceived = responseReceived;
        _isRunning = isRunning;
        _status = status;
        _errorInfo = errorInfo;

        emit finished(this);
    });
}

EntityScriptClient::EntityScriptClient() {
    setCustomDeleter([](Dependency* dependency){
        static_cast<EntityScriptClient*>(dependency)->deleteLater();
    });
    
    auto nodeList = DependencyManager::get<NodeList>();
    auto& packetReceiver = nodeList->getPacketReceiver();

    packetReceiver.registerListener(PacketType::EntityScriptGetStatusReply, this, "handleGetScriptStatusReply");

    connect(nodeList.data(), &LimitedNodeList::nodeKilled, this, &EntityScriptClient::handleNodeKilled);
    connect(nodeList.data(), &LimitedNodeList::clientConnectionToNodeReset,
            this, &EntityScriptClient::handleNodeClientConnectionReset);
}

GetScriptStatusRequest* EntityScriptClient::createScriptStatusRequest(QUuid entityID) {
    auto request = new GetScriptStatusRequest(entityID);

    request->moveToThread(thread());

    return request;
}

bool EntityScriptClient::reloadServerScript(QUuid entityID) {
    // Send packet to entity script server
    auto nodeList = DependencyManager::get<NodeList>();
    SharedNodePointer entityScriptServer = nodeList->soloNodeOfType(NodeType::EntityScriptServer);

    if (entityScriptServer) {
        auto id = entityID.toRfc4122();
        auto payloadSize = id.size();
        auto packet = NLPacket::create(PacketType::ReloadEntityServerScript, payloadSize, true);
        
        packet->write(id);

        if (nodeList->sendPacket(std::move(packet), *entityScriptServer) != -1) {
            return true;
        }
    }

    return false;
}

MessageID EntityScriptClient::getEntityServerScriptStatus(QUuid entityID, GetScriptStatusCallback callback) {
    auto nodeList = DependencyManager::get<NodeList>();
    SharedNodePointer entityScriptServer = nodeList->soloNodeOfType(NodeType::EntityScriptServer);

    if (entityScriptServer) {
        auto packetList = NLPacketList::create(PacketType::EntityScriptGetStatus, QByteArray(), true, true);

        auto messageID = ++_currentID;
        packetList->writePrimitive(messageID);

        packetList->write(entityID.toRfc4122());

        if (nodeList->sendPacketList(std::move(packetList), *entityScriptServer) != -1) {
            _pendingEntityScriptStatusRequests[entityScriptServer][messageID] = callback;

            return messageID;
        }
    }

    callback(false, false, EntityScriptStatus::ERROR_LOADING_SCRIPT, "");
    return INVALID_MESSAGE_ID;
}

void EntityScriptClient::handleGetScriptStatusReply(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode) {
    Q_ASSERT(QThread::currentThread() == thread());

    MessageID messageID;
    bool isKnown { false };
    EntityScriptStatus status = EntityScriptStatus::ERROR_LOADING_SCRIPT;
    QString errorInfo { "" };

    message->readPrimitive(&messageID);
    message->readPrimitive(&isKnown);

    if (isKnown) {
        message->readPrimitive(&status);
        errorInfo = message->readString();
    }

    // Check if we have any pending requests for this node
    auto messageMapIt = _pendingEntityScriptStatusRequests.find(senderNode);
    if (messageMapIt != _pendingEntityScriptStatusRequests.end()) {

        // Found the node, get the MessageID -> Callback map
        auto& messageCallbackMap = messageMapIt->second;

        // Check if we have this pending request
        auto requestIt = messageCallbackMap.find(messageID);
        if (requestIt != messageCallbackMap.end()) {
            auto callback = requestIt->second;
            callback(true, isKnown, status, errorInfo);
            messageCallbackMap.erase(requestIt);
        }

        // Although the messageCallbackMap may now be empty, we won't delete the node until we have disconnected from
        // it to avoid constantly creating/deleting the map on subsequent requests.
    }
}

void EntityScriptClient::handleNodeKilled(SharedNodePointer node) {
    Q_ASSERT(QThread::currentThread() == thread());

    if (node->getType() != NodeType::EntityScriptServer) {
        return;
    }

    forceFailureOfPendingRequests(node);
}

void EntityScriptClient::handleNodeClientConnectionReset(SharedNodePointer node) {
    // a client connection to a Node was reset
    // if it was an EntityScriptServer we need to cause anything pending to fail so it is re-attempted

    if (node->getType() != NodeType::EntityScriptServer) {
        return;
    }

    //qCDebug(entity_script_client) << "EntityScriptClient detected client connection reset handshake with Asset Server - failing any pending requests";

    forceFailureOfPendingRequests(node);
}

void EntityScriptClient::forceFailureOfPendingRequests(SharedNodePointer node) {

    {
        auto messageMapIt = _pendingEntityScriptStatusRequests.find(node);
        if (messageMapIt != _pendingEntityScriptStatusRequests.end()) {
            for (const auto& value : messageMapIt->second) {
                value.second(false, false, EntityScriptStatus::ERROR_LOADING_SCRIPT, "");
            }
            messageMapIt->second.clear();
        }
    }
}
