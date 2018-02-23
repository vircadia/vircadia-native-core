// FIXME: temporary workaround for duplicating the FBXModel when dynamically replacing an underlying mesh part
#include <graphics/BufferViewHelpers.h>
#include <graphics-scripting/GraphicsScriptingUtil.h>
class MyGeometryResource : public GeometryResource {
public:
    shared_ptr<FBXGeometry> fbxGeometry;
    MyGeometryResource(const QUrl& url, Geometry::Pointer originalGeometry, scriptable::ScriptableModelBasePointer newModel) : GeometryResource(url) {
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
        geometry.bindExtents = original->bindExtents;

        for (const auto &j : original->joints) {
            geometry.joints << j;
        }
        for (const FBXMaterial& material : original->materials) {
            _materials.push_back(std::make_shared<NetworkMaterial>(material, _textureBaseUrl));
        }
        std::shared_ptr<GeometryMeshes> meshes = std::make_shared<GeometryMeshes>();
        std::shared_ptr<GeometryMeshParts> parts = std::make_shared<GeometryMeshParts>();
        int meshID = 0;
        if (newModel) {
            geometry.meshExtents.reset();
            for (const auto& newMesh : newModel->meshes) {
                // qDebug() << "newMesh #" << meshID;
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
                meshes->emplace_back(newMesh.getMeshPointer());
                int partID = 0;
                const auto oldParts = mesh.parts;
                mesh.parts.clear();
                for (const FBXMeshPart& fbxPart : oldParts) {
                    FBXMeshPart part; // new copy
                    part.materialID = fbxPart.materialID;
                    // Construct local parts
                    part.triangleIndices = buffer_helpers::toVector<int>(mesh._mesh->getIndexBuffer(), "part.triangleIndices");
                    mesh.parts << part;
                    auto p = std::make_shared<MeshPart>(meshID, partID, 0);
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
        _loaded = true;
        _fbxGeometry = fbxGeometry;
    };
};

