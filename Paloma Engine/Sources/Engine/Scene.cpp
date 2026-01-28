//
//  Scene.cpp
//  Paloma Engine
//
//  Created by Artem on 20.01.2026.
//

#include "Scene.hpp"
#include "AAPLMathUtilities.h"
#include "Entity.hpp"
#include "Mesh.hpp"
#include "ObjCUtils.hpp"
#include "ResourceContext.hpp"
#include <MetalKit/MetalKit.hpp>
#include <objc/runtime.h>
#include <simd/simd.h>
#include <unordered_map>

Scene *Scene::load(const std::string &path, MTL::Device *pDevice) {

  auto scene = new Scene();

  scene->rootEntity = std::make_shared<Entity>();
  scene->rootEntity->name = "SceneRoot";

  auto bufferAllocator =
      NS::TransferPtr(MTK::MeshBufferAllocator::alloc()->init(pDevice));

  NS::String *nsPath = NS::String::string(path.c_str(), NS::UTF8StringEncoding);
  NS::URL *url = NS::URL::fileURLWithPath(nsPath);

  auto asset = NS::TransferPtr(
      MDL::Asset::alloc()->init(url, nullptr, bufferAllocator.get()));
  asset->loadTextures();

  Class mdlObjectClass = objc_getClass("MDLObject");
  Class mdlMeshClass = objc_getClass("MDLMesh");

  NS::Array *allObjects = asset->childObjectsOfClass(mdlObjectClass);

  auto resourceContext = ResourceContext(pDevice);

  std::unordered_map<MDL::Object *, std::shared_ptr<Entity>> entityMap;
  for (NS::UInteger i = 0; i < allObjects->count(); ++i) {
    auto pObject = allObjects->object<MDL::Object>(i);

    std::shared_ptr<Entity> entity;
    if (is_kind_of<MDL::Mesh>(pObject, mdlMeshClass)) {
      MDL::Mesh *mdlMesh = (MDL::Mesh *)pObject;

      auto modelEntity = std::make_shared<ModelEntity>();

      MDL::VertexDescriptor *inputs = mdlMesh->vertexDescriptor();
      NS::Array *attributes = inputs->attributes();

      int uvCount = 0;
      for (NS::UInteger j = 0; j < attributes->count(); ++j) {
        MDL::VertexAttribute *attr =
            attributes->object<MDL::VertexAttribute>(j);
        if (attr->name()->isEqual(MDL::VertexAttributeTextureCoordinate)) {
          uvCount++;
        }
      }
      bool hasMultipleUVs = uvCount > 1;

      auto vertexDescriptor =
          NS::TransferPtr(MDL::VertexDescriptor::alloc()->init());

      // Attr 0: Position
      {
        auto attr =
            vertexDescriptor->attributes()->object<MDL::VertexAttribute>(0);
        attr->setName(MDL::VertexAttributePosition);
        attr->setFormat(MDL::VertexFormatFloat4);
        attr->setOffset(0);
        attr->setBufferIndex(0);
      }
      // Attr 1: Normal
      {
        auto attr =
            vertexDescriptor->attributes()->object<MDL::VertexAttribute>(1);
        attr->setName(MDL::VertexAttributeNormal);
        attr->setFormat(MDL::VertexFormatFloat3);
        attr->setOffset(16);
        attr->setBufferIndex(0);
      }
      // Attr 2: Tangent
      {
        auto attr =
            vertexDescriptor->attributes()->object<MDL::VertexAttribute>(2);
        attr->setName(MDL::VertexAttributeTangent);
        attr->setFormat(MDL::VertexFormatFloat4);
        attr->setOffset(32);
        attr->setBufferIndex(0);
      }
      // Attr 3: TexCoord 1
      {
        auto attr =
            vertexDescriptor->attributes()->object<MDL::VertexAttribute>(3);
        attr->setName(MDL::VertexAttributeTextureCoordinate);
        attr->setFormat(MDL::VertexFormatFloat2);
        attr->setOffset(48);
        attr->setBufferIndex(0);
      }
      // Attr 4: TexCoord 2 (if hasMultipleUVs)
      if (hasMultipleUVs) {
        auto attr =
            vertexDescriptor->attributes()->object<MDL::VertexAttribute>(4);
        attr->setName(MDL::VertexAttributeTextureCoordinate);
        attr->setFormat(MDL::VertexFormatFloat2);
        attr->setOffset(56);
        attr->setBufferIndex(0);
      }

      {
        auto layout =
            vertexDescriptor->layouts()->object<MDL::VertexBufferLayout>(0);
        layout->setStride(hasMultipleUVs ? 64 : 56);
      }

      mdlMesh->setVertexDescriptor(vertexDescriptor.get());

      mdlMesh->addOrthTanBasisForTextureCoordinateAttributeNamed(
          MDL::VertexAttributeTextureCoordinate, MDL::VertexAttributeNormal,
          MDL::VertexAttributeTangent);

      modelEntity->mesh = resourceContext.convert(mdlMesh);
      entity = modelEntity;
    } else {

      entity = std::make_shared<Entity>();
    }

    if (auto named = (MDL::Named *)pObject) {
      NS::String *name = named->name();
      if (name) {
        entity->name = name->cString(NS::UTF8StringEncoding);
      }
    }

    if (MDL::TransformComponent *transformComp = pObject->transform()) {

      matrix_float4x4 modelMatrix = transformComp->localTransformAtTime(0.0);

      entity->transform = modelMatrix;
    }

    entityMap[pObject] = entity;

    MDL::Object *parentObject = pObject->parent();
    if (parentObject) {
      auto it = entityMap.find(parentObject);
      if (it != entityMap.end()) {
        it->second->addChild(entity);
      } else {
        printf("Logic error: Entity parent not found in map (load order "
               "issue?)\n");
      }
    } else {
      scene->rootEntity->addChild(entity);
    }

    scene->resources = resourceContext.resources;
  }
  return scene;
}
