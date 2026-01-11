//
//  MeshLoader.mm
//  Paloma Engine
//
//  Created by Artem on 07.01.2026.
//

#import "MeshLoader.hpp"
#import <Foundation/Foundation.h>
#import <MetalKit/MetalKit.h>
#import <ModelIO/ModelIO.h>

namespace Paloma {
namespace MeshLoader {

// -- Helper: MDLMesh -> our Mesh --
static Mesh *convertMTKMesh(MTKMesh *mtkMesh, MTL::Device *device) {

  /// Get vertex buffer
  /// MTKMesh created buffers
  MTKMeshBuffer *vertexBuffer = mtkMesh.vertexBuffers.firstObject;
  if (!vertexBuffer) {
    NSLog(@"MeshLoader: No vertex buffer");
    return nullptr;
  }

  /// Get submeshes
  std::vector<Submesh> submeshes;
  for (MTKSubmesh *sub in mtkMesh.submeshes) {
    Submesh s;
    s.indexStart = 0; // MTKSubmesh keep offset in buffer
    s.indexCount = (uint32_t)sub.indexCount;
    s.indexType = (MTL::IndexType)sub.indexType;
    submeshes.push_back(s);
  }

  /// Get index buffer (from first submesh)
  MTKSubmesh *firstSub = mtkMesh.submeshes.firstObject;
  if (!firstSub) {
    NSLog(@"MeshLoader: No submesh");
    return nullptr;
  }

  /// Convert Obj-C buffers to C++
  /// retain for
  MTL::Buffer *vb = (__bridge_retained MTL::Buffer *)vertexBuffer.buffer;
  MTL::Buffer *ib =
      (__bridge_retained MTL::Buffer *)firstSub.indexBuffer.buffer;

  /// Create mesh
  return new Mesh(vb, ib, std::move(submeshes));
}

// -- Loading from file --

Mesh *loadFromFile(MTL::Device *device, const char *path) {
  @autoreleasepool {
    /// Convert path
    NSString *nsPath = [NSString stringWithUTF8String:path];
    NSURL *url = [NSURL fileURLWithPath:nsPath];

    /// Create Metal buffers allocator
    id<MTLDevice> mtlDevice = (__bridge id<MTLDevice>)device;
    MTKMeshBufferAllocator *allocator =
        [[MTKMeshBufferAllocator alloc] initWithDevice:mtlDevice];

    /// Load MDLAsset
    MDLAsset *asset = [[MDLAsset alloc] initWithURL:url
                                   vertexDescriptor:nil
                                    bufferAllocator:allocator];
    if (!asset || asset.count == 0) {
      NSLog(@"MeshLoader: Failed to load %@", nsPath);
      return nullptr;
    }

    /// Get 1 obj (mesh)
    MDLObject *object = [asset objectAtIndex:0];
    if (![object isKindOfClass:[MDLMesh class]]) {
      NSLog(@"MeshLoader: First object is not a mesh");
      return nullptr;
    }
    MDLMesh *mdlMesh = (MDLMesh *)object;

    /// Convert to MTKMesh
    NSError *error = nil;
    MTKMesh *mtkMesh = [[MTKMesh alloc] initWithMesh:mdlMesh
                                              device:mtlDevice
                                               error:&error];

    if (!mtkMesh || error) {
      NSLog(@"MeshLoader: MTKMesh error: %@", error);
      return nullptr;
    }

    /// Convert to our Mesh
    return convertMTKMesh(mtkMesh, device);
  }
}

// -- Creating the primitive

Mesh *loadPrimitive(MTL::Device *device, const char *name) {
  @autoreleasepool {
    id<MTLDevice> mtlDevice = (__bridge id<MTLDevice>)device;
    MTKMeshBufferAllocator *allocator =
        [[MTKMeshBufferAllocator alloc] initWithDevice:mtlDevice];


    MDLVertexDescriptor *mdlDesc = [[MDLVertexDescriptor alloc] init];
    
    // Position (float3, offset 0)
    mdlDesc.attributes[0] = [[MDLVertexAttribute alloc]
        initWithName:MDLVertexAttributePosition
              format:MDLVertexFormatFloat3
              offset:0
         bufferIndex:0];
    
    // Normal (float3, offset 12)
    mdlDesc.attributes[1] = [[MDLVertexAttribute alloc]
        initWithName:MDLVertexAttributeNormal
              format:MDLVertexFormatFloat3
              offset:12
         bufferIndex:0];
    
    // Texcoord (float2, offset 24)
    mdlDesc.attributes[2] = [[MDLVertexAttribute alloc]
        initWithName:MDLVertexAttributeTextureCoordinate
              format:MDLVertexFormatFloat2
              offset:24
         bufferIndex:0];
    
    // Layout: stride = 32
    mdlDesc.layouts[0] = [[MDLVertexBufferLayout alloc] initWithStride:32];


    MDLMesh *mdlMesh = nil;
    NSString *primName = [NSString stringWithUTF8String:name];

    if ([primName isEqualToString:@"box"]) {
      mdlMesh = [MDLMesh newBoxWithDimensions:(vector_float3){1, 1, 1}
                                     segments:(vector_uint3){1, 1, 1}
                                 geometryType:MDLGeometryTypeTriangles
                                inwardNormals:NO
                                    allocator:allocator];
    } else if ([primName isEqualToString:@"sphere"]) {
      mdlMesh = [MDLMesh newEllipsoidWithRadii:(vector_float3){0.5, 0.5, 0.5}
                                radialSegments:50
                              verticalSegments:32
                                  geometryType:MDLGeometryTypeTriangles
                                 inwardNormals:NO
                                    hemisphere:NO
                                     allocator:allocator];
    } else if ([primName isEqualToString:@"plane"]) {
      mdlMesh = [MDLMesh newPlaneWithDimensions:(vector_float2){1, 1}
                                       segments:(vector_uint2){1, 1}
                                   geometryType:MDLGeometryTypeTriangles
                                      allocator:allocator];
    }

    if (!mdlMesh) {
      NSLog(@"MeshLoader: Unknown primitive: %s", name);
      return nullptr;
    }
    

    mdlMesh.vertexDescriptor = mdlDesc;

    NSError *error = nil;
    MTKMesh *mtkMesh = [[MTKMesh alloc] initWithMesh:mdlMesh
                                              device:mtlDevice
                                               error:&error];
    if (!mtkMesh || error) {
      NSLog(@"MeshLoader: MTKMesh error: %@", error);
      return nullptr;
    }

    return convertMTKMesh(mtkMesh, device);
  }
}

} // namespace MeshLoader
} // namespace Paloma
