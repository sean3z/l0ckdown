#pragma once

#include <vector>
#include "game_structures.hpp"
#include <d3d11.h>
#include "overlay/imgui/imgui.h"
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <thread>
#include <chrono>

namespace globals {
    using namespace protocol::engine::sdk;
	using namespace protocol::game::sdk;
    using namespace protocol::engine;

	inline u_world* gworld = 0;
	inline a_game_state_base* game_state = 0;
	inline u_game_instance* owning_instance = 0;
	inline u_localplayer* local_player = 0;
	inline a_player_controller* local_controller = 0;
	inline a_player_camera_manager* local_camera_manager = 0;
	inline mec_pawn* local_mec = 0;

	inline std::vector < mec_pawn* > player_cache{};
	inline std::vector < world_item* > world_item_cache{};
	inline std::vector < task_vents* > task_vents_cache{};
	inline std::vector < task_machines* > task_machines_cache{};
	inline std::vector < task_alimentations* > task_alims_cache{};
	inline std::vector < task_deliveries* > task_delivery_cache{};
	inline std::vector < task_pizzushis* > task_pizzushi_cache{};
	inline std::vector < task_data* > task_data_cache{};
	inline std::vector < task_scanner* > task_scanner_cache{};
	inline std::vector < a_weapon_case_code_c* > weapon_case_cache{};

	 // Synchronization tools
    inline std::mutex player_cache_mutex;
    inline std::condition_variable cv;
    inline std::atomic<bool> data_populated{false};  // Flag to indicate if data is populated

	inline ImVec4 employee_color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green
	inline ImVec4 dissident_color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red 
	inline ImVec4 ghost_employee_color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green
	inline ImVec4 ghost_dissident_color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red 
}