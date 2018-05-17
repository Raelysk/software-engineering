#include <Windows.h>

#include "XLib.Heap.h"

using namespace XLib;

void* Heap::Allocate(uintptr size)
{
	if (!size)
		return nullptr;

	void *ptr = HeapAlloc(GetProcessHeap(), 0, size);
	if (!ptr)
		throw;
	return ptr;
}
void* Heap::ReAllocate(void* ptr, uintptr size)
{
	if (size)
	{
		void *newPtr = ptr ? HeapReAlloc(GetProcessHeap(), 0, ptr, size) : HeapAlloc(GetProcessHeap(), 0, size);
		if (!newPtr)
			throw;
		return newPtr;
	}
	if (ptr)
		HeapFree(GetProcessHeap(), 0, ptr);
	return nullptr;
}
bool Heap::ReAllocateInplace(void* ptr, uintptr size)
{
	void *newPtr = HeapReAlloc(GetProcessHeap(), 0, ptr, size);
	return newPtr ? true : false;
}
void Heap::Release(void* ptr)
{
	HeapFree(GetProcessHeap(), 0, ptr);
}