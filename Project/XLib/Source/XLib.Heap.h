#pragma once

#include "XLib.Types.h"
#include "XLib.Util.h"
#include "XLib.NonCopyable.h"

namespace XLib
{
	struct Heap abstract final
	{
		static void* Allocate(uintptr size);
		static void* ReAllocate(void* ptr, uintptr size);
		static bool ReAllocateInplace(void* ptr, uintptr size);
		static void Release(void* ptr);

		template <typename Type>
		static Type* Allocate(uintptr count = 1) { return to<Type*>(Allocate(sizeof(Type) * count)); }
	};

	template <typename Type>
	class HeapPtr : public NonCopyable
	{
	private:
		Type *ptr;

	public:
		inline explicit HeapPtr(uintptr size)
			{ ptr = size ? Heap::Allocate<Type>(size) : nullptr; }
		inline HeapPtr() : ptr(nullptr) {}
		~HeapPtr() { release(); }

		inline HeapPtr(HeapPtr&& that) : ptr(that.ptr) { that.ptr = nullptr; }
		inline void operator = (HeapPtr&& that) { swap(ptr, that.ptr); }

		void release()
		{
			if (ptr)
			{
				Heap::Release(ptr);
				ptr = nullptr;
			}
		}
		void resize(uintptr size) { ptr = (Type*)Heap::ReAllocate(ptr, size * sizeof(Type)); }
		bool resizeInplace(uintptr size) { return Heap::ReAllocateInplace(ptr, size * sizeof(Type)); }

		inline operator Type* () { return ptr; }
		inline bool isAllocated() { return ptr ? true : false; }

		template <typename OtherType>
		inline explicit operator OtherType () const { return OtherType(ptr); }
	};
}