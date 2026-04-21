#pragma once
#include "pch.h"
#include "Handle.h"
#include "Holder.h"

// As of right now the end sentinel is the largest number we can store in a unint32_t. This needs to be changed to reflect the limit on
// what types of resources take up which portions of the larger descriptor heap. For now we can leave it since we dont populate the heap with that many textures.
// But just now this will be changed to reflect a more universal and proper allocation scheme.
namespace sludge::utils
{
	// A pool essentially is made up of two types. Object type which is essentially the handle structs like TextureTag etc.
	// and then the actual implementation of said structure ImplObjectType. IN our case its a classes like Texture or Model that holds the data and some other
	// info but it could easily just have been the D3D12Resource that represents a texture/CBV/etc. if we wanted to get rid of some abstractions.
	template<typename ObjectType, typename ImplObjectType>
	class Pool {
		static constexpr uint32_t kListEndSentinel = 0xffffffff;
		// Every element in a pool is actually a struct called a PoolEntry
		// The pool entry stores the object by value in obj_ along with its generation which is used to check
		// handles which point to this element. nextFree_ is used to maintain a linked list of free elements inside the array.
		// When a handle is deallocated a corresponding array element is added to the list to signafy a new element is available to be used.
		// freeListHead_ will always store the index of the first free element or it will hold the end sentinel if theres nothing left.
		struct PoolEntry
		{
			// This is a strange one but we will come back and see if this explicit needs to be removed.
			explicit PoolEntry(ImplObjectType& obj)
				: obj_(std::move(obj))
			{
			}
			ImplObjectType obj_ = {};
			uint32_t gen_ = 1;
			uint32_t nextFree_ = kListEndSentinel;
		};
		uint32_t freeListHead_ = kListEndSentinel;
		uint32_t numObjects_ = 0;
	public:
		std::vector<PoolEntry> objects_;

		// Create takes rvalues and will check the head of the free linked list we have.
		// If its not the end sentinel, then we can immediately move the object into the array!
		// We also set the head of the free list to point to the next free element.
		// If there isnt enough space, we just use emplace back to create the object directly into the vector
		// NOTE: obj technically a universal reference, but we just want r val reference utility so we dont use std::forward.
		Handle<ObjectType> create(ImplObjectType&& obj) {
			uint32_t idx = 0;
			if (freeListHead_ != kListEndSentinel) {
				idx = freeListHead_;
				freeListHead_ = objects_[idx].nextFree_;
				objects_[idx].obj_ = std::move(obj);
			}
			else {
				idx = (uint32_t)objects_.size();
				// It may look strange but emplace_back performs direct initilization of the PoolEntry class,
				// it's equivalent to calling the constructor for PoolEntry directly. So although it is declared explicit
				// its actually perfectly legal in THIS instance.
				objects_.emplace_back(obj);
			}
			numObjects_++;

			return Handle<ObjectType>(idx, objects_[idx].gen_);
		}

		// Destroy needs extra error handling to catch instances that might pop up such as:
		// 1. Trying to delete an empty handle
		// 2. Trying to delete a non empty handle when the pool itself is empty(logic error)
		// 3. Double delete i.e. the generation of our handle and that of the current entry in the array are different. (this is where generations come in handy)
		void destroy(Handle<ObjectType> handle) {
			if (handle.empty())
			{
				return;
			}
			assert(numObjects_ > 0);
			const uint32_t index = handle.index();
			assert(index < objects_.size());
			// double deletion
			assert(handle.gen() == objects_[index].gen_);
			objects_[index].obj_ = ImplObjectType{};
			objects_[index].gen_++;
			// Keeping in line with the fact that we are using a linked list, the next free thing is whatever the current free head is
			objects_[index].nextFree_ = freeListHead_;
			// and our new free head is this element that we just destroyed
			freeListHead_ = index;
			numObjects_--;
		}

		// To dereference a handle and retrieve the underlying type we use these get functions.
		// WE have both a const and non const version but they both function the same as per the book.
		// otherwise its a simple implementation.
		const ImplObjectType* get(Handle<ObjectType> handle) const {
			if (handle.empty())
				return nullptr;

			const uint32_t index = handle.index();
			assert(index < objects_.size());
			assert(handle.gen() == objects_[index].gen_); // accessing deleted object
			return &objects_[index].obj_;
		}
		// NOTE: Pools are going to store, in general some user structure and for the ImplyObjectType, usually a ComPtr<ID3D12Resource>.
		// This means what we are returning here is a pointer to a pointer structure. Real weird I know. 
		ImplObjectType* get(Handle<ObjectType> handle) {
			if (handle.empty())
				return nullptr;

			const uint32_t index = handle.index();
			assert(index < objects_.size());
			assert(handle.gen() == objects_[index].gen_); // accessing deleted object
			return &objects_[index].obj_;
		}

		// Check if object is in pool and get its handle
		Handle<ObjectType> findObject(const ImplObjectType* obj) {
			if (!obj)
				return {};

			for (size_t idx = 0; idx != objects_.size(); idx++) {
				if (objects_[idx].obj_ == *obj) {
					return Handle<ObjectType>((uint32_t)idx, objects_[idx].gen_);
				}
			}

			return {};
		}

		// Clear will just use the vector clear which in turn will just call the destructor for all the objects in our pool
		void clear() {
			objects_.clear();
			freeListHead_ = kListEndSentinel;
			numObjects_ = 0;
		}
		uint32_t numObjects() const {
			return numObjects_;
		}
	};
} // utils