#pragma once
#include "pch.h"
#include "Handle.h"

namespace sludge::utils
{
    static constexpr UINT TEXTURE_START = 0;
    static constexpr UINT TEXTURE_COUNT = 100;

    static constexpr UINT STRUCTURED_BUFFER_START = 101;
    static constexpr UINT STRUCTURED_BUFFER_COUNT = 30;

    static constexpr UINT PASS_CONSANT_START = 132;
    static constexpr UINT PASS_CONSTANT_COUNT = 5;

    static constexpr UINT MODEL_CONSTANT_START = 138;
    static constexpr UINT MODEL_CONSTANT_COUNT = 100;

    static constexpr UINT TEXTURE_UAV_START = 239;
    static constexpr UINT TEXTURE_UAV_COUNT = 100;
    // Handles dont actually own the objects they point to! Instead the holder class does.
    // Holder is what does the clean up for us, and is essentially what provides RAII.
    template<typename HandleType>
    class Holder final {
    public:
        // A holder functions something like a unique_ptr in that it only consist of move only semantics.
        Holder() = default;
        Holder(HandleType handle) : handle_(handle)
        {
        }
        ~Holder()
        {
            //destroy(handle_);
        }
        // Here is our move ctor. Notice we delete the copy ctor.
        Holder(const Holder&) = delete;
        // move ctor
        Holder(Holder&& other) : handle_(other.handle_)
        {
            other.handle_ = HandleType{};
        }

        // and here is our move assignment with the same deletion of the copy assignment as above.
        Holder& operator=(const Holder&) = delete;
        Holder& operator=(Holder&& other)
        {
            std::swap(handle_, other.handle_);
            return *this;
        }
        // Here is for assigning nullptr to our holders.
        Holder& operator=(std::nullptr_t)
        {
            this->reset();
            return *this;
        }

        // Conversion into underlying handle type
        inline operator HandleType() const
        {
            return handle_;
        }

        // Given that a holder encapsulates a handle, we just leverage the functions that already come with handles.
        bool valid() const
        {
            return handle_.valid();
        }
        bool empty() const
        {
            return handle_.empty();
        }
        uint32_t gen() const
        {
            return handle_.gen();
        }
        uint32_t index() const
        {
            return handle_.index() + offset(handle_);
        }

        void reset()
        {
            destroy(handle_);
            handle_ = HandleType{};
        }

        HandleType release()
        {
            return std::exchange(handle_, HandleType{});
        }

        void* indexAsVoid() const
        {
            return handle_.indexAsVoid();
        }
        void* handleAsVoid() const
        {
            return handle_.handleAsVoid();
        }

    private:
        HandleType handle_ = {};
    };
} // utils
