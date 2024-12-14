#pragma once

#include "game_math.hpp"
#include <Windows.h>
#include "mem.hpp"
#include "game_structures.hpp"

namespace util {
	using namespace protocol::engine::sdk;
	using namespace protocol::game::sdk;

	static std::string get_name_from_fname(int key)
	{
		static std::map<int, std::string> cached_fnames{};

		auto cached_name = cached_fnames.find(key);
		if (cached_name != cached_fnames.end())
			return cached_name->second;

		auto chunkOffset = (UINT)((int)(key) >> 16);
		auto name_offset = (USHORT)key;

		auto pool_chunk = mem::rpm<UINT64>(mem::module_base + protocol::engine::GNAMES + ((chunkOffset + 2) * 8));
		auto entry_offset = pool_chunk + (ULONG)(2 * name_offset);
		auto name_entry = mem::rpm<INT16>(entry_offset);

		auto len = name_entry >> 6;
		char buff[1028];
		if ((DWORD)len && len > 0)
		{
			memset(buff, 0, 1028);
			mem::read_raw(entry_offset + 2, buff, len);
			buff[len] = '\0';
			std::string ret(buff);
			cached_fnames.emplace(key, ret);
			return std::string(ret);
		}
		else return "";
	}

	/*
	std::vector<uintptr_t> find_objects(std::string name_find) {
		std::vector<uintptr_t> objs{};
		constexpr auto elements_per_chunk = 64 * 1024;
		auto gobjects = drv->rpm<fuobjectarray>(mem::module_base + GOBJECTS);
		for (int i = 0; i < gobjects.num_chunk; i++) {
			auto chunk_start = drv->rpm<uintptr_t>(gobjects.chunk_table + (i * 0x8));
			for (int i = 0; i < elements_per_chunk; i++) {
				auto item = drv->rpm<fuobjectitem>(chunk_start + (i * sizeof(fuobjectitem)));
				auto name = get_name_from_fname(GNAMES, ((u_object*)item.object)->fname_index());
				if (isa(item.object, name_find, false)) {
					objs.push_back(item.object);
				}
			}
		}
		return objs;
	}
*/
}