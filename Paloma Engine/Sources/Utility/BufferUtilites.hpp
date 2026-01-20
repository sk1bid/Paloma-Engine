//
//  BufferUtilites.hpp
//  Paloma Engine
//
//  Created by Artem on 19.01.2026.
//

#pragma once
#include <Metal/Metal.hpp>
#include <cassert>
#include <vector>
#include <numeric>

static inline size_t alignUp(size_t n, size_t alignment) {
    return (n + alignment - 1) & ~(alignment - 1);
}

struct BufferView {
    MTL::Buffer *pBuffer;
    size_t offset;
    size_t length;
    
    uint64_t gpuAddress() const { return pBuffer->gpuAddress() + offset; }
};

class RingBuffer {
public:
    RingBuffer(size_t length, MTL::Device* pDevice) : _nextOffset(0) {
        _pBuffer = NS::TransferPtr(pDevice->newBuffer(length, MTL::ResourceStorageModeShared));
        
        if (pDevice->supportsFamily(MTL::GPUFamilyApple2)) {
            _minimumAlignment = 4;
        } else if (pDevice->supportsFamily(MTL::GPUFamilyMac2)) {
            _minimumAlignment = 32;
        } else {
            _minimumAlignment = 256;
        }
    }
    
    // reset pointer (every frame)
    void reset() {
        _nextOffset = 0;
    }
    
    // allocate memory in buffer
    // if there is not enough space to reach the end of the buffer, it "loops" back to the beginning
    BufferView allocate(size_t length, size_t alignment){
        assert(length <= _pBuffer->length());
        
        size_t effectiveAligment = std::lcm(alignment, _minimumAlignment);
        size_t offset = alignUp(_nextOffset, effectiveAligment);
        
        if (offset + length >= _pBuffer->length()) {
            offset = 0;
        }
        
        _nextOffset = offset + length;
        return { _pBuffer.get(), offset, length};
    }
    
    // Copy 1 object
    template <typename T>
    BufferView copy(const T& value) {
        static_assert(std::is_trivially_copyable<T>::value, "The type must be POD-compatible to be copied to the GPU");
        
        BufferView view = allocate(sizeof(T), alignof(T));
        uint8_t* pContents = (uint8_t*)_pBuffer->contents();
        memcpy(pContents + view.offset, &value, sizeof(T));
        
        return view;
    }
    
    // Copy vector of elements
    template <typename T>
    BufferView copy(const std::vector<T>& elements) {
        static_assert(std::is_trivially_copyable<T>::value, "The type must be POD-compatible to be copied to the GPU");
        
        if (elements.empty()) return { _pBuffer.get(), 0, 0 };
        size_t totalSize = sizeof(T) * elements.size();
        
        BufferView view = allocate(totalSize, alignof(T));
        
        uint8_t* pContents = (uint8_t*)_pBuffer->contents();
        memcpy(pContents + view.offset, elements.data(), totalSize);
        
        return view;
    }
    
    MTL::Buffer* getBuffer() const { return _pBuffer.get(); }
    
private:
    NS::SharedPtr<MTL::Buffer> _pBuffer;
    size_t _minimumAlignment;
    size_t _nextOffset;
};
