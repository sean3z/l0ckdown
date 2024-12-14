#include <iostream>
#include <thread>
#include <iomanip>
#include <cstdlib>
#include <unordered_set>
#include <algorithm>
#include <set>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <unordered_map>
#include <limits>
#include <unordered_set>
#include <algorithm>
#include <set>
#include "globals.h"
#include "mem.hpp"
#include "game_structures.hpp"
#include "menu.h"
#include "bootstrap.h"
#include "util.hpp"

#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <chrono>

using namespace std;
using namespace globals;
using namespace protocol::engine::sdk;
using namespace protocol::game::sdk;
using namespace protocol::engine;

std::mutex player_cache_mutex;
std::mutex world_item_cache_mutex;
std::mutex task_cache_mutex;

std::vector<mec_pawn*> player_cache;
std::vector<world_item*> world_item_cache;
std::vector<task_vents*> task_vents_cache;
std::vector<task_machines*> task_machines_cache;
std::vector<task_alimentations*> task_alims_cache;
std::vector<task_deliveries*> task_delivery_cache;
std::vector<task_pizzushis*> task_pizzushi_cache;
std::vector<task_data*> task_data_cache;
std::vector<task_scanner*> task_scanner_cache;
std::vector<a_weapon_case_code_c*> weapon_case_cache;

std::atomic<bool> data_populated(false); // Atomic flag to track data readiness

std::condition_variable cv;

static void cache_useful() {
	bool items_populated = false;  // Flag to track if items have been populated once

	while (true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(50));

		gworld = u_world::get_world(mem::module_base);
		if (!gworld) continue;
		game_state = gworld->get_game_state();
		if (!game_state) continue;
		owning_instance = gworld->get_owning_game_instance();
		if (!owning_instance) continue;
		local_player = owning_instance->get_localplayer();
		if (!local_player) continue;
		local_controller = local_player->get_player_controller();
		if (!local_controller) continue;
		local_camera_manager = local_controller->get_camera_manager();
		if (!local_camera_manager) continue;
		local_mec = (mec_pawn*)local_controller->get_pawn();
		if (!local_mec) continue;

		std::vector<mec_pawn*> temp_player_cache;
        std::vector<world_item*> temp_world_item_cache;
        std::vector<task_vents*> temp_task_vents_cache;
        std::vector<task_machines*> temp_task_machines_cache;
        std::vector<task_alimentations*> temp_task_alims_cache;
        std::vector<task_deliveries*> temp_task_delivery_cache;
        std::vector<task_pizzushis*> temp_task_pizzushi_cache;
        std::vector<task_data*> temp_task_data_cache;
        std::vector<task_scanner*> temp_task_scanner_cache;
        std::vector<a_weapon_case_code_c*> temp_weapon_case_cache;

		// std::vector<FStr_ScannerDot> scanner_targets;

		auto levels = gworld->get_levels();
		for (auto level : levels.list()) {
			auto actors = level->get_actors();

			for (auto actor : actors.list()) {
				auto class_name = util::get_name_from_fname(actor->class_private()->fname_index());
				auto name = util::get_name_from_fname(actor->outer()->fname_index());

				if (class_name.find("WorldItem_C") != std::string::npos) {
					auto item = static_cast<world_item*>(actor);
					auto item_data = item->get_data();
					auto item_name = item_data->get_name().read_string();


					temp_world_item_cache.push_back((world_item*)actor);
				}
				if (class_name.find("Task_Vents_C") != std::string::npos) {
					temp_task_vents_cache.push_back((task_vents*)actor);
				}
				if (class_name.find("Task_Machine_C") != std::string::npos) {
					temp_task_machines_cache.push_back((task_machines*)actor);
				}
				if (class_name.find("Task_Alim_C") != std::string::npos) {
					temp_task_alims_cache.push_back((task_alimentations*)actor);
				}
				if (class_name.find("Task_DelivryIn_C") != std::string::npos) {
					temp_task_delivery_cache.push_back((task_deliveries*)actor);
				}
				if (class_name.find("Task_Pizzushi_C") != std::string::npos) {
					temp_task_pizzushi_cache.push_back((task_pizzushis*)actor);
				}
				if (class_name.find("Task_Data_C") != std::string::npos) {
					temp_task_data_cache.push_back((task_data*)actor);
				}
				if (class_name.find("Task_Scanner_C") != std::string::npos) {
					temp_task_scanner_cache.push_back((task_scanner*)actor);
				}
				if (class_name.find("Mec_C") != std::string::npos) {
					temp_player_cache.push_back((mec_pawn*)actor);
				}
				if (class_name.find("WeaponCaseCode_C") != std::string::npos) {
					temp_weapon_case_cache.push_back((a_weapon_case_code_c*)actor);
				}
			}
		}

		// Locking the mutexes before updating shared data
        {
            std::lock_guard<std::mutex> lock_player_cache(globals::player_cache_mutex);
            globals::player_cache = temp_player_cache;  // Use the global player_cache
        }

        {
            std::lock_guard<std::mutex> lock_world_item_cache(world_item_cache_mutex);
            globals::world_item_cache = temp_world_item_cache;
        }

        {
            std::lock_guard<std::mutex> lock_task_cache(task_cache_mutex);
            globals::task_vents_cache = temp_task_vents_cache;
            globals::task_machines_cache = temp_task_machines_cache;
            globals::task_alims_cache = temp_task_alims_cache;
            globals::task_delivery_cache = temp_task_delivery_cache;
            globals::task_pizzushi_cache = temp_task_pizzushi_cache;
            globals::task_data_cache = temp_task_data_cache;
            globals::task_scanner_cache = temp_task_scanner_cache;
            globals::weapon_case_cache = temp_weapon_case_cache;
        }

		// Call PopulateUniqueItems only once after items are populated
		if (!items_populated && !temp_world_item_cache.empty()) {
			globals::data_populated = true; // Set flag to true when data is populated
        	globals::cv.notify_all();  // Notify main thread that data is ready
			items_populated = true;  // Ensure this only happens once
		}
	}
}

int main() {
    std::cout << ("attaching to game") << std::endl;

    if (mem::attach("LOCKDOWN Protocol  ") != 0) {
		std::cout << ("open the game") << std::endl;
		std::this_thread::sleep_for(std::chrono::seconds(2));
		exit(1);
	}

	std::cout << "process id " << mem::process_id << std::endl;

	mem::module_base = mem::get_module_base("LockdownProtocol-Win64-Shipping.exe", mem::process_id);
	if (!mem::module_base) {
		std::cout << "module base invalid" << std::endl;
		std::this_thread::sleep_for(std::chrono::seconds(2));
		exit(1);
	}

	Bootstrap::Initialize();  // Initializes DirectX and ImGui

	// std::this_thread::sleep_for(std::chrono::milliseconds(500));

	std::thread cache_thread(cache_useful);
	cache_thread.detach();
	//cache_thread.join();

	while(true) {
		Bootstrap::RenderLoop();
		
		// Sleep briefly to reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
	}
	

	// while(true) {
    //     // Perform any necessary cleanup or rendering (handled by Bootstrap)
        

	// 	auto players = game_state->player_array();

	// 	auto hand_item = local_mec->get_hand_item();
	// 	auto melee_item_data = (u_data_melee*)hand_item;

	// 	if (hand_item) {
	// 	   auto mtype = melee_item_data->get_melee_type();
	// 	   // Retrieve the properties
	// 	   double castTime = mtype->get_cast_time();
	// 	   double recoverTime = mtype->get_recover_time();
	// 	   double stun = mtype->get_stun();
	// 	   int cost = mtype->get_cost();
	// 	   int range = mtype->get_range();

	// 	   // Format and output the string to the console
	// 	   std::cout << "itemData[\"" << hand_item->get_name().read_string() << "\"] = "
	// 	   << "ItemProperties(" << castTime << ", "
	// 	   << recoverTime << ", "
	// 	   << stun << ", "
	// 	   << range << ", "
	// 	   << cost << ");" << std::endl;
	// 	}

	// 	auto player_state = local_mec->player_state();
	// 	if (!player_state) continue;

	// 	auto name = player_state->get_player_name_private().read_string();
	// 	auto role = local_mec->get_player_role();

	// 	std::cout << "player name: " << name << std::endl;
	// 	std::cout << "player role: " << role << std::endl;

	// 	for (auto mec : player_cache) {
	// 		auto state = mec->player_state();
	// 		if (!state) continue;

	// 		if (mec != local_mec) {
	// 			auto name = state->get_player_name_private().read_string();
	// 			auto role = mec->get_player_role();

	// 			std::cout << "player name: " << name << std::endl;
	// 			std::cout << "player role: " << role << std::endl;
	// 		}
	// 	}

	// 	for (auto weapon_case : weapon_case_cache) {
	// 		if (!weapon_case) continue;

	// 		auto role = local_mec->get_player_role();
	// 		if (role == 3 || role == 4) {
	// 			auto weapon_case_code = weapon_case->get_target_values();
	// 			auto case_code = weapon_case_code.list();
	// 			auto case_timer = weapon_case->get_opening_timer().Handle;
	// 			auto open_delay = weapon_case->get_open_delay();

	// 			auto box_to_open = weapon_case->get_box_to_open();
	// 			auto case_weapon = box_to_open->get_selected_weapon_qsdsf();
	// 			auto case_open = box_to_open->get_case_open();
	// 			//if (!case_weapon) continue;

	// 			auto case_weapon_state = box_to_open->get_item_slot();
	// 			auto case_weapon_ammo = case_weapon_state->get_item_state().Value_8;

	// 			// Build the string
	// 			std::string weapon_case_code_string;
	// 			std::ostringstream oss;

	// 			for (const auto& byte : case_code) {
	// 				oss << static_cast<int>(byte); // Convert UINT8 to integer and append directly
	// 			}

	// 			weapon_case_code_string = oss.str(); // Extract the string

	// 			auto caseRoot = weapon_case->get_root_component();
	// 			if (!caseRoot) {
	// 				std::cout << "weapon case root is null!" << std::endl;
	// 				continue;
	// 			}

	// 			auto caseLocation = caseRoot->get_relative_location();
	// 			auto selected_weapon = box_to_open->get_selected_weapon();
	// 			std::unordered_map<int, std::string> weapon_map = {
	// 				{1, "Empty"},
	// 				{2, "Pistol"},
	// 				{3, "Revolver"},
	// 				{4, "Shorty"}
	// 			};

	// 			std::string case_weapon_name = weapon_map.count(selected_weapon) ? weapon_map[selected_weapon] : "Unknown";

	// 			std::cout << "case_weapon_name: " << case_weapon_name << std::endl;
	// 			std::cout << "weapon_case_code_string: " << weapon_case_code_string << std::endl;
	// 		}
	// 	}

	// 	// Sleep briefly to reduce CPU usage
    //     std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
	// }

	// Cleanup ImGui and DirectX resources
    Bootstrap::Cleanup();
    return 0;
}

