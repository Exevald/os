#include "MemoryManager.h"

int main()
{
	alignas(alignof(std::max_align_t)) char buffer[1000];

	MemoryManager memoryManager(buffer, std::size(buffer));

	auto ptr = memoryManager.Allocate(sizeof(double));

	auto value = std::construct_at(static_cast<double*>(ptr), 3.1415927);
	assert(*value == 3.1415927);

	memoryManager.Free(ptr);
}