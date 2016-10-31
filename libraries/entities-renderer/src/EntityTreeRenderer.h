//
//  EntityTreeRenderer.h
//  interface/src
//
//  Created by Brad Hefta-Gaub on 12/6/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EntityTreeRenderer_h
#define hifi_EntityTreeRenderer_h

#include <QSet>
#include <QStack>

#include <AbstractAudioInterface.h>
#include <EntityScriptingInterface.h> // for RayToEntityIntersectionResult
#include <EntityTree.h>
#include <QMouseEvent>
#include <PointerEvent.h>
#include <OctreeRenderer.h>
#include <ScriptCache.h>
#include <TextureCache.h>

class AbstractScriptingServicesInterface;
class AbstractViewStateInterface;
class Model;
class ScriptEngine;
class ZoneEntityItem;
class EntityItem;

class Model;
using ModelPointer = std::shared_ptr<Model>;
using ModelWeakPointer = std::weak_ptr<Model>;

using CalculateEntityLoadingPriority = std::function<float(const EntityItem& item)>;

// Generic client side Octree renderer class.
class EntityTreeRenderer : public OctreeRenderer, public EntityItemFBXService, public Dependency {
    Q_OBJECT
public:
    EntityTreeRenderer(bool wantScripts, AbstractViewStateInterface* viewState,
                                AbstractScriptingServicesInterface* scriptingServices);
    virtual ~EntityTreeRenderer();

    QSharedPointer<EntityTreeRenderer> getSharedFromThis() {
        return qSharedPointerCast<EntityTreeRenderer>(sharedFromThis());
    }

    virtual char getMyNodeType() const override { return NodeType::EntityServer; }
    virtual PacketType getMyQueryMessageType() const override { return PacketType::EntityQuery; }
    virtual PacketType getExpectedPacketType() const override { return PacketType::EntityData; }
    virtual void setTree(OctreePointer newTree) override;

    // Returns the priority at which an entity should be loaded. Higher values indicate higher priority.
    float getEntityLoadingPriority(const EntityItem& item) const { return _calculateEntityLoadingPriorityFunc(item); }
    void setEntityLoadingPriorityFunction(CalculateEntityLoadingPriority fn) { this->_calculateEntityLoadingPriorityFunc = fn; }

    void shutdown();
    void update();

    EntityTreePointer getTree() { return std::static_pointer_cast<EntityTree>(_tree); }

    void processEraseMessage(ReceivedMessage& message, const SharedNodePointer& sourceNode);

    virtual void init() override;

    virtual const FBXGeometry* getGeometryForEntity(EntityItemPointer entityItem) override;
    virtual ModelPointer getModelForEntityItem(EntityItemPointer entityItem) override;

    /// clears the tree
    virtual void clear() override;

    /// reloads the entity scripts, calling unload and preload
    void reloadEntityScripts();

    /// if a renderable entity item needs a model, we will allocate it for them
    Q_INVOKABLE ModelPointer allocateModel(const QString& url, float loadingPriority = 0.0f);

    /// if a renderable entity item needs to update the URL of a model, we will handle that for the entity
    Q_INVOKABLE ModelPointer updateModel(ModelPointer original, const QString& newUrl);

    /// if a renderable entity item is done with a model, it should return it to us
    void releaseModel(ModelPointer model);

    void deleteReleasedModels();

    // event handles which may generate entity related events
    void mouseReleaseEvent(QMouseEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);

    /// connect our signals to anEntityScriptingInterface for firing of events related clicking,
    /// hovering over, and entering entities
    void connectSignalsToSlots(EntityScriptingInterface* entityScriptingInterface);

    // For Scene.shouldRenderEntities
    QList<EntityItemID>& getEntitiesLastInScene() { return _entityIDsLastInScene; }

    std::shared_ptr<ZoneEntityItem> myAvatarZone() { return _layeredZones.getZone(); }

signals:
    void mousePressOnEntity(const EntityItemID& entityItemID, const PointerEvent& event);
    void mouseMoveOnEntity(const EntityItemID& entityItemID, const PointerEvent& event);
    void mouseReleaseOnEntity(const EntityItemID& entityItemID, const PointerEvent& event);
    void mousePressOffEntity();

    void clickDownOnEntity(const EntityItemID& entityItemID, const PointerEvent& event);
    void holdingClickOnEntity(const EntityItemID& entityItemID, const PointerEvent& event);
    void clickReleaseOnEntity(const EntityItemID& entityItemID, const PointerEvent& event);

    void hoverEnterEntity(const EntityItemID& entityItemID, const PointerEvent& event);
    void hoverOverEntity(const EntityItemID& entityItemID, const PointerEvent& event);
    void hoverLeaveEntity(const EntityItemID& entityItemID, const PointerEvent& event);

    void enterEntity(const EntityItemID& entityItemID);
    void leaveEntity(const EntityItemID& entityItemID);
    void collisionWithEntity(const EntityItemID& idA, const EntityItemID& idB, const Collision& collision);

public slots:
    void addingEntity(const EntityItemID& entityID);
    void deletingEntity(const EntityItemID& entityID);
    void entitySciptChanging(const EntityItemID& entityID, const bool reload);
    void entityCollisionWithEntity(const EntityItemID& idA, const EntityItemID& idB, const Collision& collision);
    void updateEntityRenderStatus(bool shouldRenderEntities);
    void updateZone(const EntityItemID& id);

    // optional slots that can be wired to menu items
    void setDisplayModelBounds(bool value) { _displayModelBounds = value; }
    void setDontDoPrecisionPicking(bool value) { _dontDoPrecisionPicking = value; }

protected:
    virtual OctreePointer createTree() override {
        EntityTreePointer newTree = EntityTreePointer(new EntityTree(true));
        newTree->createRootElement();
        return newTree;
    }

private:
    void resetEntitiesScriptEngine();

    void addEntityToScene(EntityItemPointer entity);
    bool findBestZoneAndMaybeContainingEntities(QVector<EntityItemID>* entitiesContainingAvatar = nullptr);

    bool applyZoneAndHasSkybox(const std::shared_ptr<ZoneEntityItem>& zone);
    bool layerZoneAndHasSkybox(const std::shared_ptr<ZoneEntityItem>& zone);
    bool applySkyboxAndHasAmbient();

    void checkAndCallPreload(const EntityItemID& entityID, const bool reload = false);

    QList<ModelPointer> _releasedModels;
    RayToEntityIntersectionResult findRayIntersectionWorker(const PickRay& ray, Octree::lockType lockType,
                                                                bool precisionPicking, const QVector<EntityItemID>& entityIdsToInclude = QVector<EntityItemID>(),
                                                                const QVector<EntityItemID>& entityIdsToDiscard = QVector<EntityItemID>(), bool visibleOnly=false,
                                                                bool collidableOnly = false);

    EntityItemID _currentHoverOverEntityID;
    EntityItemID _currentClickingOnEntityID;

    QScriptValueList createEntityArgs(const EntityItemID& entityID);
    bool checkEnterLeaveEntities();
    void leaveAllEntities();
    void forceRecheckEntities();

    glm::vec3 _avatarPosition { 0.0f };
    QVector<EntityItemID> _currentEntitiesInside;

    bool _wantScripts;
    QSharedPointer<ScriptEngine> _entitiesScriptEngine;

    bool isCollisionOwner(const QUuid& myNodeID, EntityTreePointer entityTree,
                          const EntityItemID& id, const Collision& collision);

    void playEntityCollisionSound(const QUuid& myNodeID, EntityTreePointer entityTree,
                                  const EntityItemID& id, const Collision& collision);

    bool _lastPointerEventValid;
    PointerEvent _lastPointerEvent;
    AbstractViewStateInterface* _viewState;
    AbstractScriptingServicesInterface* _scriptingServices;
    bool _displayModelBounds;
    bool _dontDoPrecisionPicking;

    bool _shuttingDown { false };

    QMultiMap<QUrl, EntityItemID> _waitingOnPreload;

    class LayeredZone {
    public:
        LayeredZone(std::shared_ptr<ZoneEntityItem> zone, QUuid id, float volume) : zone(zone), id(id), volume(volume) {}
        LayeredZone(std::shared_ptr<ZoneEntityItem> zone) : LayeredZone(zone, zone->getID(), zone->getVolumeEstimate()) {}

        bool operator<(const LayeredZone& r) const { return std::tie(volume, id) < std::tie(r.volume, r.id); }
        bool operator==(const LayeredZone& r) const { return id == r.id; }
        bool operator<=(const LayeredZone& r) const { return (*this < r) || (*this == r); }

        std::shared_ptr<ZoneEntityItem> zone;
        QUuid id;
        float volume;
    };

    class LayeredZones : public std::set<LayeredZone> {
    public:
        LayeredZones(EntityTreeRenderer* parent) : _entityTreeRenderer(parent) {}
        LayeredZones(LayeredZones&& other);

        // avoid accidental misconstruction
        LayeredZones() = delete;
        LayeredZones(const LayeredZones&) = delete;
        LayeredZones& operator=(const LayeredZones&) = delete;
        LayeredZones& operator=(LayeredZones&&) = delete;

        void clear();
        std::pair<iterator, bool> insert(const LayeredZone& layer);

        void apply();
        void update(std::shared_ptr<ZoneEntityItem> zone);

        bool contains(const LayeredZones& other);

        std::shared_ptr<ZoneEntityItem> getZone() { return empty() ? nullptr : begin()->zone; }

    private:
        void applyPartial(iterator layer);

        std::map<QUuid, iterator> _map;
        iterator _skyboxLayer{ end() };
        EntityTreeRenderer* _entityTreeRenderer;
    };

    LayeredZones _layeredZones;
    QString _zoneUserData;
    NetworkTexturePointer _ambientTexture;
    NetworkTexturePointer _skyboxTexture;
    QString _ambientTextureURL;
    QString _skyboxTextureURL;
    bool _pendingAmbientTexture { false };
    bool _pendingSkyboxTexture { false };

    quint64 _lastZoneCheck { 0 };
    const quint64 ZONE_CHECK_INTERVAL = USECS_PER_MSEC * 100; // ~10hz
    const float ZONE_CHECK_DISTANCE = 0.001f;

    QHash<EntityItemID, EntityItemPointer> _entitiesInScene;
    // For Scene.shouldRenderEntities
    QList<EntityItemID> _entityIDsLastInScene;

    static int _entitiesScriptEngineCount;

    CalculateEntityLoadingPriority _calculateEntityLoadingPriorityFunc = [](const EntityItem& item) -> float {
        return 0.0f;
    };
};


#endif // hifi_EntityTreeRenderer_h
