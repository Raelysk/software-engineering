#pragma once

#include "XLib.Types.h"
#include "XLib.Util.h"
#include "XLib.NonCopyable.h"
#include "XLib.Heap.h"

namespace XLib
{
	struct VectorHeapUsagePolicy abstract final
	{
		class SingleDynamicHeapBuffer abstract final {};

		template <uint32 minSizeLog2, uint32 maxSizeLog2>
		class MultipleStaticHeapBuffers abstract final
			{ static_assert(minSizeLog2 <= maxSizeLog2, "invalid params"); };

		template <uint32 bufferSize>
		class StaticBuffer abstract final {};
	};

	template <typename Type, typename HeapUsagePolicy =
		VectorHeapUsagePolicy::SingleDynamicHeapBuffer>
	class Vector;

	template <typename Type>
	class Vector<Type, VectorHeapUsagePolicy::SingleDynamicHeapBuffer>
		: public NonCopyable
	{
	private:
		static constexpr uint32 initialBufferSize = 16;

		HeapPtr<Type> buffer;
		uint32 bufferSize, vectorSize;

		inline void expandBuffer(uint32 minNewSize)
		{
			if (bufferSize < minNewSize)
			{
				if (bufferSize)
					bufferSize = max(vectorSize * 2, minNewSize);
				else
					bufferSize = max(initialBufferSize, minNewSize);
				buffer.resize(bufferSize);
			}
		}

	public:
		explicit inline Vector(uint32 initSize = 0)
			: buffer(initSize), bufferSize(initSize), vectorSize(initSize)
		{
			for (uint32 i = 0; i < vectorSize; i++)
				construct(buffer[i]);
		}
		inline ~Vector() { bufferSize = 0; vectorSize = 0; }

		inline Vector(Vector&& that) : buffer(move(that.buffer))
		{
			bufferSize = that.bufferSize;
			vectorSize = that.vectorSize;
			that.bufferSize = 0;
			that.vectorSize = 0;
		}
		inline void operator = (Vector&& that)
		{
			swap(buffer, that.buffer);
			swap(bufferSize, that.bufferSize);
			swap(vectorSize, that.vectorSize);
			return *this;
		}

		inline Type& pushBack(const Type& value)
		{
			expandBuffer(vectorSize + 1);
			buffer[vectorSize] = value;
			return buffer[vectorSize++];
		}
		inline Type popBack() { return buffer[--vectorSize]; }
		//inline Type& allocateBack() {} 
		inline void dropBack() { vectorSize--; }
		inline Type& front() { return buffer[0]; }
		inline Type& back() { return buffer[vectorSize - 1]; }

		inline Type* allocateBack(uint32 count)
		{
			expandBuffer(vectorSize + count);
			Type *result = buffer + vectorSize;
			vectorSize += count;

			for (uint32 i = 0; i < count; i++)
				construct(result[i]);

			return result;
		}
		inline Type& allocateBack() { return *allocateBack(1); }

		inline void resize(uint32 newSize)
		{
			vectorSize = newSize;
			expandBuffer(vectorSize);
		}
		inline void clear() { resize(0); }
		inline void compact() { buffer.resize(vectorSize); }
		inline HeapPtr<Type> takeBuffer()
		{
			compact();
			bufferSize = 0;
			vectorSize = 0;
			return move(buffer);
		}

		inline operator Type*() { return buffer; }
		inline uint32 getSize() const { return vectorSize; }
		inline uint32 getByteSize() const { return vectorSize * sizeof(Type); }
		inline bool isEmpty() const { return vectorSize == 0; }

		template <typename OtherType>
		inline OtherType to() { return OtherType(ptr); }

		inline Type* begin() { return buffer; }
		inline Type* end() { return (Type*)buffer + vectorSize; }
	};

	template <typename Type, uint32 minSizeLog2, uint32 maxSizeLog2>
	class Vector<Type, VectorHeapUsagePolicy::MultipleStaticHeapBuffers<minSizeLog2, maxSizeLog2>>
		: public NonCopyable
	{
	private:
		static constexpr uint8 bufferCount = maxSizeLog2 - minSizeLog2 + 1;

		Type *buffers[bufferCount] = {};
		uint8 usedBufferCount = 0, allocatedBufferCount = 0;
		uint32 lastUsedBufferOccup = 0;

		static inline uint32 computeBufferSize(uint32 bufferIndex)
			{ return 1 << (bufferIndex - sgn(bufferIndex) + minSizeLog2); }

	public:
		inline ~Vector()
		{
			for (uint8 i = 0; i < allocatedBufferCount; i++)
			{
				Heap::Release(buffers[i]);
				buffers[i] = nullptr;
			}
		}

		class Iterator final
		{
			friend Vector;

		private:
			Vector &parent;
			uint32 elementIndex;
			uint32 bufferSize;
			uint8 bufferIndex;

			Iterator() = delete;
			inline Iterator(Vector& _parent, uint32 _elementIndex, uint8 bufferIndex)
				: parent(_parent), elementIndex(_elementIndex), bufferIndex(bufferIndex)
			{
				bufferSize = computeBufferSize(bufferIndex);
			}

		public:
			inline void operator ++()
			{
				elementIndex++;
				if (elementIndex >= bufferSize)
				{
					bufferSize <<= sgn(bufferIndex);
					bufferIndex++;
					elementIndex = 0;
				}
			}

			inline bool operator != (const Iterator& that) const
			{
				return elementIndex != that.elementIndex ||
					bufferIndex != that.bufferIndex;
			}

			inline Type& operator *() { return parent.buffers[bufferIndex][elementIndex]; }
		};

		inline Type& allocateBack()
		{
			if (usedBufferCount)
			{
				if (lastUsedBufferOccup < computeBufferSize(usedBufferCount - 1))
					lastUsedBufferOccup++;
				else
				{
					usedBufferCount++;
					lastUsedBufferOccup = 1;
					if (usedBufferCount > allocatedBufferCount)
					{
						buffers[allocatedBufferCount] =
							Heap::Allocate<Type>(computeBufferSize(allocatedBufferCount));
						allocatedBufferCount++;
					}
				}

				Type& result = buffers[usedBufferCount - 1][lastUsedBufferOccup - 1];
				construct(result);
				return result;
			}
			else
			{
				if (!allocatedBufferCount)
				{
					allocatedBufferCount = 1;
					buffers[0] = Heap::Allocate<Type>(computeBufferSize(minSizeLog2));
				}
				usedBufferCount = 1;
				lastUsedBufferOccup = 1;

				Type& result = buffers[0][0];
				construct(result);
				return result;
			}
		}

		inline void clear()
		{
			usedBufferCount = 0;
			lastUsedBufferOccup = 0;
		}

		inline Type& operator [] (uint32 index)
		{
			uint32 bufferIndex = max<sint32>(flo(index) - minSizeLog2, 0);
			uint32 elementIndexMask = (1 << ((bufferIndex - 1) * sgn(bufferIndex) + minSizeLog2)) - 1;
			uint32 elementIndex = index & elementIndexMask;
			return buffers[bufferIndex][elementIndex];
		}

		inline uint32 getSize()
		{
			uint32 base = (1 << (usedBufferCount + minSizeLog2 - 2)) * sgn(usedBufferCount - 1);
			return (base + lastUsedBufferOccup) * sgn(usedBufferCount);
		}

		inline bool isEmpty() { return usedBufferCount == 0; }

		inline Iterator begin() { return Iterator(*this, 0, 0); }
		inline Iterator end()
		{
			if (usedBufferCount == 0 || lastUsedBufferOccup >= computeBufferSize(usedBufferCount - 1))
				return Iterator(*this, 0, usedBufferCount);
			else
				return Iterator(*this, lastUsedBufferOccup, usedBufferCount - 1);
		}
	};

	template <typename Type, uint32 bufferSize>
	class Vector<Type, VectorHeapUsagePolicy::StaticBuffer<bufferSize>>
		: public NonCopyable
	{
	private:
		Type buffer[bufferSize];
		uint32 size;

	public:
		inline Type& pushBack(const Type& value);
		inline Type popBack();
		inline void clear();

		inline operator Type*() { return buffer; }
		inline uint32 getSize() { return size; }
		inline bool isEmpty() { return size == 0; }
		inline bool isFull() { return size == bufferSize; }

		inline Type* begin();
		inline Type* end();
	};
}