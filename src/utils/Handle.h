#pragma once
#include "pch.h"


namespace sludge::utils
{
    template <typename ObjectType>
    class Handle final
    {
    public:
        Handle() = default;
        explicit Handle(void* ptr) :
            index_(reinterpret_cast<ptrdiff_t>(ptr) & 0xffffffff), gen_((reinterpret_cast<ptrdiff_t>(ptr) >> 32) & 0xffffffff)
        {
        }

        // Handles with a generation of 0 are considered null handles.
        bool empty() const
        {
            return gen_ == 0;
        }
        bool valid() const
        {
            return gen_ != 0;
        }
        uint32_t index() const
        {
            return index_;
        }
        uint32_t gen() const
        {
            return gen_;
        }

        // This is useful for when we need to pass a handle through a C-style interface which takes in void* parameters. ImGui is the best example of this.
        void* indexAsVoid() const
        {
            return reinterpret_cast<void*>(static_cast<ptrdiff_t>(index_));
        }
        void* handleAsVoid() const
        {
            static_assert(sizeof(void*) >= sizeof(uint64_t));
            return reinterpret_cast<void*>((static_cast<ptrdiff_t>(gen_) << 32) + static_cast<ptrdiff_t>(index_));
        }
        bool operator==(const Handle<ObjectType>& other) const
        {
            return index_ == other.index_ && gen_ == other.gen_;
        }
        bool operator!=(const Handle<ObjectType>& other) const
        {
            return index_ != other.index_ || gen_ != other.gen_;
        }

        // Finally we have this explicit conversion to bool so we can use the ever convenient if(handle) in our code.
        explicit operator bool() const
        {
            return gen_ != 0;
        }

    private:
        Handle(uint32_t index, uint32_t gen) : index_(index), gen_(gen)
        {
        };

        // Handles should only be created by the pools that manage them.
        // Pools consist of two types. The objects type and the type actually stored inside the implementation array.
        // Its not visible from the handle interface and can only be accessed via a pool object.
        template<typename ObjectType_, typename ImplObjectType>
        friend class Pool;

        // Handles are pretty much supposed to function like pointers to objects within an array
        // As such, an index is enough to identify an object in the array.
        uint32_t index_ = 0;
        // Simple index based handles can have these sort of "dangling" indices. lets say we have a texture at index 2.
        // If I free up that texture at that index, then create a new texture that then is stored in this freed up space at index 2 we now have a problem.
        // why? Well the handle to our old texture, lets call it oldTex still points to the index 2. If I try to get the resource at this handle with 
        // get(oldTex) im being returned the texture at index 2, which is a completely different texture.
        // By adding generations when we call get(oldTex), get() will check the index AND the generation. When we deleted and created a new texture at index 2
        // the handle generation is incrememnted to 1, and when we call get(oldTex) and it looks at that generation at that index and sees 0 != 1 itll catch the bug
        // before it causes any problems. 
        uint32_t gen_ = 0;
    };
    using TextureHandle = Handle<struct TextureTag>;
    using TextureUAVHandle = Handle<struct TextureUAVTag>;
    using GeometryHandle = Handle<struct GeometryTag>;
    using PassConstantHandle = Handle<struct PassConstantTag>;
    using ModelConstantHandle = Handle<struct ModelConstantTag>;

    // forward declarations for our tags
    void destroy(TextureHandle handle);
    void destroy(GeometryHandle handle);
    void destroy(PassConstantHandle handle);
    void destroy(ModelConstantHandle handle);

    uint32_t offset(GeometryHandle handle);
    uint32_t offset(TextureHandle handle);
    uint32_t offset(TextureUAVHandle handle);
    uint32_t offset(PassConstantHandle handle);
    uint32_t offset(ModelConstantHandle handle);

} // utils
