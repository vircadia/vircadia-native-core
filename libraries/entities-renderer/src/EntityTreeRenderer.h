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
#include <ScriptCache.h>
#include <TextureCache.h>
#include <OctreeProcessor.h>
#include <render/Forward.h>

class AbstractScriptingServicesInterface;
class AbstractViewStateInterface;
class Model;
class ScriptEngine;
class ZoneEntityItem;
class EntityItem;

namespace render { namespace entities {
    class EntityRenderer;
    using EntityRendererPointer = std::shared_ptr<EntityRenderer>;
    using EntityRendererWeakPointer = std::weak_ptr<EntityRenderer>;
} }

// Allow the use of std::unordered_map with QUuid keys
namespace std { template<> struct hash<EntityItemID> { size_t operator()(const EntityItemID& id) const; }; }

using EntityRenderer = render::entities::EntityRenderer;
using EntityRendererPointer = render::entities::EntityRendererPointer;
using EntityRendererWeakPointer = render::entities::EntityRendererWeakPointer;
class Model;
using ModelPointer = std::shared_ptr<Model>;
using ModelWeakPointer = std::weak_ptr<Model>;

using CalculateEntityLoadingPriority = std::function<float(const EntityItem& item)>;

// Generic client side Octree renderer class.
class EntityTreeRenderer : public OctreeProcessor, public Dependency {
    Q_OBJECT
public:
    static void setEntitiesShouldFadeFunction(std::function<bool()> func) { _entitiesShouldFadeFunction = func; }
    static std::function<bool()> getEntitiesShouldFadeFunction() { return _entitiesShouldFadeFunction; }

    EntityTreeRenderer(bool wantScripts, AbstractViewStateInterface* viewState,
                                AbstractScriptingServicesInterface* scriptingServices);
    virtual ~EntityTreeRenderer();

    QSharedPointer<EntityTreeRenderer> getSharedFromThis() {
        return qSharedPointerCast<EntityTreeRenderer>(sharedFromThis());
    }

    virtual char getMyNodeType() const override { return NodeType::EntityServer; }
    virtual PacketType getMyQueryMessageType() const override { return PacketType::EntityQuery; }
    virtual PacketType getExpectedPacketType() const override { return PacketType::EntityData; }

    // Returns the priority at which an entity should be loaded. Higher values indicate higher priority.
    static float getEntityLoadingPriority(const EntityItem& item) { return _calculateEntityLoadingPriorityFunc(item); }
    static void setEntityLoadingPriorityFunction(CalculateEntityLoadingPriority fn) { _calculateEntityLoadingPriorityFunc = fn; }

    void setMouseRayPickID(QUuid rayPickID) { _mouseRayPickID = rayPickID; }
    void setMouseRayPickResultOperator(std::function<RayToEntityIntersectionResult(QUuid)> getPrevRayPickResultOperator) { _getPrevRayPickResultOperator = getPrevRayPickResultOperator;  }
    void setSetPrecisionPickingOperator(std::function<void(QUuid, bool)> setPrecisionPickingOperator) { _setPrecisionPickingOperator = setPrecisionPickingOperator; }

    void shutdown();
    void update(bool simulate);

    EntityTreePointer getTree() { return std::static_pointer_cast<EntityTree>(_tree); }

    void processEraseMessage(ReceivedMessage& message, const SharedNodePointer& sourceNode);

    virtual void init() override;

    /// clears the tree
    virtual void clear() override;

    /// reloads the entity scripts, calling unload and preload
    void reloadEntityScripts();

    // event handles which may generate entity related events
    void mouseReleaseEvent(QMouseEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseDoublePressEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);

    /// connect our signals to anEntityScriptingInterface for firing of events related clicking,
    /// hovering over, and entering entities
    void connectSignalsToSlots(EntityScriptingInterface* entityScriptingInterface);

    // For Scene.shouldRenderEntities
    QList<EntityItemID>& getEntitiesLastInScene() { return _entityIDsLastInScene; }

    std::shared_ptr<ZoneEntityItem> myAvatarZone() { return _layeredZones.getZone(); }

    bool wantsKeyboardFocus(const EntityItemID& id) const;
    QObject* getEventHandler(const EntityItemID& id);
    bool wantsHandControllerPointerEvents(const EntityItemID& id) const;
    void setProxyWindow(const EntityItemID& id, QWindow* proxyWindow);
    void setCollisionSound(const EntityItemID& id, const SharedSoundPointer& sound);
    EntityItemPointer getEntity(const EntityItemID& id);
    void onEntityChanged(const EntityItemID& id);

signals:
    void mousePressOnEntity(const EntityItemID& entityItemID, const PointerEvent& event);
    void mouseDoublePressOnEntity(const EntityItemID& entityItemID, const PointerEvent& event);
    void mouseMoveOnEntity(const EntityItemID& entityItemID, const PointerEvent& event);
    void mouseReleaseOnEntity(const EntityItemID& entityItemID, const PointerEvent& event);
    void mousePressOffEntity();
    void mouseDoublePressOffEntity();

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
    void entityScriptChanging(const EntityItemID& entityID, const bool reload);
    void entityCollisionWithEntity(const EntityItemID& idA, const EntityItemID& idB, const Collision& collision);
    void updateEntityRenderStatus(bool shouldRenderEntities);
    void updateZone(const EntityItemID& id);

    // optional slots that can be wired to menu items
    void setDisplayModelBounds(bool value) { _displayModelBounds = value; }
    void setPrecisionPicking(bool value) { _setPrecisionPickingOperator(_mouseRayPickID, value); }
    EntityRendererPointer renderableForEntityId(const EntityItemID& id) const;
    render::ItemID renderableIdForEntityId(const EntityItemID& id) const;

protected:
    virtual OctreePointer createTree() override {
        EntityTreePointer newTree = EntityTreePointer(new EntityTree(true));
        newTree->createRootElement();
        return newTree;
    }

private:
    EntityRendererPointer renderableForEntity(const EntityItemPointer& entity) const { return renderableForEntityId(entity->getID()); }
    render::ItemID renderableIdForEntity(const EntityItemPointer& entity) const { return renderableIdForEntityId(entity->getID()); }

    void resetEntitiesScriptEngine();

    void addEntityToScene(const EntityItemPointer& entity);
    bool findBestZoneAndMaybeContainingEntities(QVector<EntityItemID>* entitiesContainingAvatar = nullptr);

    bool applyLayeredZones();

    void checkAndCallPreload(const EntityItemID& entityID, bool reload = false, bool unloadFirst = false);

    EntityItemID _currentHoverOverEntityID;
    EntityItemID _currentClickingOnEntityID;

    QScriptValueList createEntityArgs(const EntityItemID& entityID);
    bool checkEnterLeaveEntities();
    void leaveAllEntities();
    void forceRecheckEntities();

    glm::vec3 _avatarPosition { 0.0f };
    QVector<EntityItemID> _currentEntitiesInside;

    bool _wantScripts;
    ScriptEnginePointer _entitiesScriptEngine;

    void playEntityCollisionSound(const EntityItemPointer& entity, const Collision& collision);

    bool _lastPointerEventValid;
    PointerEvent _lastPointerEvent;
    AbstractViewStateInterface* _viewState;
    AbstractScriptingServicesInterface* _scriptingServices;
    bool _displayModelBounds;

    bool _shuttingDown { false };

    QMultiMap<QUrl, EntityItemID> _waitingOnPreload;

    QUuid _mouseRayPickID;
    std::function<RayToEntityIntersectionResult(QUuid)> _getPrevRayPickResultOperator;
    std::function<void(QUuid, bool)> _setPrecisionPickingOperator;

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

    ReadWriteLockable _changedEntitiesGuard;
    std::unordered_set<EntityItemID> _changedEntities;

    std::unordered_map<EntityItemID, EntityRendererPointer> _renderablesToUpdate;
    std::unordered_map<EntityItemID, EntityRendererPointer> _entitiesInScene;
    // For Scene.shouldRenderEntities
    QList<EntityItemID> _entityIDsLastInScene;

    static int _entitiesScriptEngineCount;
    static CalculateEntityLoadingPriority _calculateEntityLoadingPriorityFunc;
    static std::function<bool()> _entitiesShouldFadeFunction;
};


#endif // hifi_EntityTreeRenderer_h
