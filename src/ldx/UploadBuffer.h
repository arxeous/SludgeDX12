#pragma once
#include "utils/utils.h"

// Frankluna upload buffer, essentially does the same thing our createGPUBuffer function is doing, but with extra bells n whistles for uploading data on the fly.
namespace sludge
{
    template <typename T>
    class UploadBuffer
    {
    public:
        UploadBuffer(ID3D12Device* device, UINT elementCount, bool isConstantBuffer, std::wstring_view name) :
            mIsConstantBuffer(isConstantBuffer)
        {
            mElementByteSize = sizeof(T);

            // Constant buffer elements need to be multiples of 256 bytes.
            // This is because the hardware can only view constant data 
            // at m*256 byte offsets and of n*256 byte lengths. 
            // typedef struct D3D12_CONSTANT_BUFFER_VIEW_DESC {
            // UINT64 OffsetInBytes; // multiple of 256
            // UINT   SizeInBytes;   // multiple of 256
            // } D3D12_CONSTANT_BUFFER_VIEW_DESC;
            if (isConstantBuffer)
                mElementByteSize = utils::CalcConstantBufferByteSize(sizeof(T));

            auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
            auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(mElementByteSize * elementCount);
            ThrowIfFailed(device->CreateCommittedResource(
                &heapProp,
                D3D12_HEAP_FLAG_NONE,
                &bufferDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&mUploadBuffer)));

            ThrowIfFailed(mUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData)));
            mUploadBuffer->SetName(name.data());

            // We do not need to unmap until we are done with the resource.  However, we must not write to
            // the resource while it is in use by the GPU (so we must use synchronization techniques).
        }

        UploadBuffer(const UploadBuffer& rhs) = delete;
        UploadBuffer& operator=(const UploadBuffer& rhs) = delete;
        ~UploadBuffer()
        {
            if (mUploadBuffer != nullptr)
                mUploadBuffer->Unmap(0, nullptr);

            mMappedData = nullptr;
        }

        ID3D12Resource* Resource()const
        {
            return mUploadBuffer.Get();
        }

        void CopyData(int elementIndex, const T& data)
        {
            memcpy(&mMappedData[elementIndex * mElementByteSize], &data, sizeof(T));
        }

        void GetData(int elementIndex, T& data)
        {
            memcpy(&data, &mMappedData[elementIndex * mElementByteSize], sizeof(T));
        }
    private:
        Microsoft::WRL::ComPtr<ID3D12Resource> mUploadBuffer;
        BYTE* mMappedData = nullptr;

        UINT mElementByteSize = 0;
        bool mIsConstantBuffer = false;
    };
}