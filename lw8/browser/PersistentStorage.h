#pragma once

#include "DocumentStorage.h"
#include "InvertedIndex.h"

#include <string>

class PersistentStorage
{
public:
	static void Save(const DocumentStorage& store, const std::string& filename);
	static void Load(DocumentStorage& store, InvertedIndex& index, const std::string& filename);
};