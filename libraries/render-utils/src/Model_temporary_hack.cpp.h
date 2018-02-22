#include <graphics/BufferViewHelpers.h>
#include <graphics-scripting/GraphicsScriptingUtil.h>
class MyGeometryMappingResource : public GeometryResource {
public:
    shared_ptr<FBXGeometry> fbxGeometry;
    MyGeometryMappingResource(const QUrl& url, Geometry::Pointer originalGeometry, std::shared_ptr<scriptable::ScriptableModelBase> newModel) : GeometryResource(url) {
        fbxGeometry = std::make_shared<FBXGeometry>();
        FBXGeometry& geometry = *fbxGeometry.get();
        const FBXGeometry* original;
        shared_ptr<FBXGeometry> tmpGeometry;
        if (originalGeometry) {
            original = &originalGeometry->getFBXGeometry();
        } else {
            tmpGeometry = std::make_shared<FBXGeometry>();
            original = tmpGeometry.get();
        }
        geometry.originalURL = original->originalURL;

        geometry.author = original->author;
        geometry.applicationName = original->applicationName;
        for (const auto &j : original->joints) {
            geometry.joints << j;
        }
        geometry.jointIndices = QHash<QString,int>{ original->jointIndices };

        geometry.animationFrames = QVector<FBXAnimationFrame>{ original->animationFrames };
        geometry.meshIndicesToModelNames = QHash<int, QString>{ original->meshIndicesToModelNames };
        geometry.blendshapeChannelNames = QList<QString>{ original->blendshapeChannelNames };

        geometry.hasSkeletonJoints = original->hasSkeletonJoints;
        geometry.offset = original->offset;
        geometry.leftEyeJointIndex = original->leftEyeJointIndex;
        geometry.rightEyeJointIndex = original->rightEyeJointIndex;
        geometry.neckJointIndex = original->neckJointIndex;
        geometry.rootJointIndex = original->rootJointIndex;
        geometry.leanJointIndex = original->leanJointIndex;
        geometry.headJointIndex = original->headJointIndex;
        geometry.leftHandJointIndex = original->leftHandJointIndex;
        geometry.rightHandJointIndex = original->rightHandJointIndex;
        geometry.leftToeJointIndex = original->leftToeJointIndex;
        geometry.rightToeJointIndex = original->rightToeJointIndex;
        geometry.leftEyeSize = original->leftEyeSize;
        geometry.rightEyeSize = original->rightEyeSize;
        geometry.humanIKJointIndices = original->humanIKJointIndices;
        geometry.palmDirection = original->palmDirection;
        geometry.neckPivot = original->neckPivot;
        geometry.bindExtents = original->bindExtents;

        // Copy materials
        QHash<QString, size_t> materialIDAtlas;
        for (const FBXMaterial& material : original->materials) {
            materialIDAtlas[material.materialID] = _materials.size();
            _materials.push_back(std::make_shared<NetworkMaterial>(material, _textureBaseUrl));
        }
        std::shared_ptr<GeometryMeshes> meshes = std::make_shared<GeometryMeshes>();
        std::shared_ptr<GeometryMeshParts> parts = std::make_shared<GeometryMeshParts>();
        int meshID = 0;
        if (newModel) {
            geometry.meshExtents.reset();
            for (const auto& newMesh : newModel->meshes) {
                FBXMesh mesh;
                if (meshID < original->meshes.size()) {
                    mesh = original->meshes.at(meshID); // copy
                }
                mesh._mesh = newMesh.getMeshPointer();
                // duplicate the buffers
                mesh.vertices = buffer_helpers::toVector<glm::vec3>(mesh._mesh->getVertexBuffer(), "mesh.vertices");
                mesh.normals = buffer_helpers::toVector<glm::vec3>(buffer_helpers::getBufferView(mesh._mesh, gpu::Stream::NORMAL), "mesh.normals");
                mesh.colors = buffer_helpers::toVector<glm::vec3>(buffer_helpers::getBufferView(mesh._mesh, gpu::Stream::COLOR), "mesh.colors");
                mesh.texCoords = buffer_helpers::toVector<glm::vec2>(buffer_helpers::getBufferView(mesh._mesh, gpu::Stream::TEXCOORD0), "mesh.texCoords");
                mesh.texCoords1 = buffer_helpers::toVector<glm::vec2>(buffer_helpers::getBufferView(mesh._mesh, gpu::Stream::TEXCOORD1), "mesh.texCoords1");
                mesh.createMeshTangents(true);
                mesh.createBlendShapeTangents(false);
                geometry.meshes << mesh;
                // Copy mesh pointers
                meshes->emplace_back(newMesh.getMeshPointer());//buffer_helpers::cloneMesh(ptr));
                int partID = 0;
                const auto oldParts = mesh.parts;
                mesh.parts.clear();
                for (const FBXMeshPart& fbxPart : oldParts) {
                    FBXMeshPart part; // new copy
                    part.materialID = fbxPart.materialID;
                    // Construct local parts
                    part.triangleIndices = buffer_helpers::toVector<int>(mesh._mesh->getIndexBuffer(), "part.triangleIndices");
                    mesh.parts << part;
                    auto p = std::make_shared<MeshPart>(meshID, partID, (int)materialIDAtlas[part.materialID]);
                    parts->push_back(p);
                    partID++;
                }
                {
                    // accumulate local transforms
                    // compute the mesh extents from the transformed vertices
                    foreach (const glm::vec3& vertex, mesh.vertices) {
                        glm::vec3 transformedVertex = glm::vec3(mesh.modelTransform * glm::vec4(vertex, 1.0f));
                        geometry.meshExtents.minimum = glm::min(geometry.meshExtents.minimum, transformedVertex);
                        geometry.meshExtents.maximum = glm::max(geometry.meshExtents.maximum, transformedVertex);
                        
                        mesh.meshExtents.minimum = glm::min(mesh.meshExtents.minimum, transformedVertex);
                        mesh.meshExtents.maximum = glm::max(mesh.meshExtents.maximum, transformedVertex);
                    }
                }
                            
                meshID++;
            }
        }
        _meshes = meshes;
        _meshParts = parts;
        _animGraphOverrideUrl = originalGeometry ? originalGeometry->getAnimGraphOverrideUrl() : QUrl();
        _loaded = true;
        _fbxGeometry = fbxGeometry;
    };    
};

