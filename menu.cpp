#include "menu.h"
#include "overlay/imgui/imgui.h"
#include "game_math.hpp"
#include "game_structures.hpp"
#include "globals.h"
#include "data_cache.h"
#include <sstream>
#include <fstream>
#include <unordered_map>
#include <iomanip>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <thread>
#include <chrono>

using namespace globals;

namespace Menu {
    // Static flag to track if the menu has already been rendered in this frame
    static bool menuRendered = false;
    static int xx = 0;

    static bool enable_invincible = false;
    static bool enable_stamina = false;

    std::string MenuCalculateDistance(const FVector& location1, const vector3& location2) {
        // Calculate the difference in coordinates
        double dx = location1.X - location2.x;
        double dy = location1.Y - location2.y;
        double dz = location1.Z - location2.z;

        // Calculate the distance and divide by 100.0 to get distance in meters (assuming 100 units per meter)
        double distance = std::sqrt(dx * dx + dy * dy + dz * dz) / 100.0;

        std::ostringstream stream;
        stream << std::fixed << std::setprecision(2) << distance;
        return stream.str();
    }

    double MenuCalculateDistanceMeters(const vector3& location1, const vector3& location2) {
        double dx = location1.x - location2.x;
        double dy = location1.y - location2.y;
        double dz = location1.z - location2.z;

        return std::sqrt(dx * dx + dy * dy + dz * dz) / 100.0;
    }

    std::string TranslateLocation(int index) {
        switch(index) {
            case 0: return "Storage"; break;
            case 1: return "Machine"; break;
            case 2: return "Computers"; break;
            case 3: return "Botanical"; break;
            case 4: return "Medical"; break;
            case 5: return "Pizza"; break;
            case 6: return "Cams"; break;
        }

        return "Unknown";
    }

    void RenderMenu() {
        // Wait for the data to be populated
        std::unique_lock<std::mutex> lock(globals::player_cache_mutex);
        globals::cv.wait(lock, []{ return globals::data_populated.load(); });

        // Set the window to start at the top-left corner of the application window
        ImGui::SetNextWindowPos(ImVec2(0, 0));

        // Set the size of the window to match the display size
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

        // Begin the fullscreen window with no title bar, resizing, or collapsing
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

        ImGui::Begin("Lockd0wn Protocol", nullptr, flags);  // Begin a new ImGui window

        ImVec2 availableSpace = ImGui::GetContentRegionAvail(); // Get the available space

        int numChildren = 4;               // Number of children
        float childWidth = availableSpace.x / numChildren; // Divide width equally
        float childHeight = availableSpace.y;      // Use full height (optional)

        ImGui::BeginChild("About me", ImVec2(childWidth, childHeight), true);

        auto player_state = globals::local_mec->player_state();

        auto name = player_state->get_player_name_private().read_string();
        auto role = globals::local_mec->get_player_role();

        auto role_text = "[?]";
        switch(role) {
            case 1:
                role_text = "[L]";
                break;

            case 3:
                role_text = "[E]";
                break;

            case 4:
                role_text = "[D]";
                break;
        }

        std::string display_name = name + " " + role_text;

        ImGui::Text("Player: %s", display_name.c_str());

        auto hand_item = globals::local_mec->get_hand_item();
		auto melee_item_data = (u_data_melee*)hand_item;

        auto equipped = hand_item->get_name().read_string();

        ImGui::Text("Equipped: %s", equipped.c_str());

        if (ImGui::Button("Start Game")) {
            xx++;
        }

        ImGui::Text("Score: %d", xx);

        ImGui::EndChild();  // End About Me

        ImGui::SameLine(); // Place the next item on the same line

        // ImGui::SetNextWindowPos(ImVec2(window1_pos.x + window1_size.x, window1_pos.y));
        ImGui::BeginChild("Cases",  ImVec2(childWidth, childHeight), true);

        int location = 0;

        for (auto weapon_case : globals::weapon_case_cache) {
			if (!weapon_case) continue;

			auto role = globals::local_mec->get_player_role();
			if (role == 3 || role == 4) {
				auto weapon_case_code = weapon_case->get_target_values();
				auto case_code = weapon_case_code.list();
				auto case_timer = weapon_case->get_opening_timer().Handle;
				auto open_delay = weapon_case->get_open_delay();

				auto box_to_open = weapon_case->get_box_to_open();
				auto case_weapon = box_to_open->get_selected_weapon_qsdsf();
				auto case_open = box_to_open->get_case_open();
				//if (!case_weapon) continue;

				auto case_weapon_state = box_to_open->get_item_slot();
				auto case_weapon_ammo = case_weapon_state->get_item_state().Value_8;

				// Build the string
				std::string weapon_case_code_string;
				std::ostringstream oss;

				for (const auto& byte : case_code) {
					oss << static_cast<int>(byte); // Convert UINT8 to integer and append directly
				}

				weapon_case_code_string = oss.str(); // Extract the string

				auto caseRoot = weapon_case->get_root_component();
				if (!caseRoot) {
					std::cout << "weapon case root is null!" << std::endl;
					continue;
				}

				auto caseLocation = caseRoot->get_relative_location();
				auto selected_weapon = box_to_open->get_selected_weapon();
				std::unordered_map<int, std::string> weapon_map = {
					{1, "Empty"},
					{2, "Pistol"},
					{3, "Revolver"},
					{4, "Shotgun"}
				};

				std::string case_weapon_name = weapon_map.count(selected_weapon) ? weapon_map[selected_weapon] : "Unknown";

                if (ImGui::CollapsingHeader(TranslateLocation(location++).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::Text(case_weapon_name.c_str());
                    ImGui::Text("Code: %s", weapon_case_code_string.c_str());
                    ImGui::Text("Open: %s", open_delay > 0 ? std::to_string(open_delay).c_str() : case_open ? "Yes" : "No");
                }
            }
        }

        ImGui::EndChild(); // end Cases

        ImGui::SameLine(); // Place the next item on the same line

        ImGui::BeginChild("Players",  ImVec2(childWidth, childHeight), true);

        // Temporary vectors to separate employees and dissidents
		std::vector<std::pair<std::string, ImVec4>> employees;
		std::vector<std::pair<std::string, ImVec4>> dissidents;

		// Variables to calculate dynamic size
		float max_text_width = 0.0f;

		for (auto mec : globals::player_cache) {
            auto state = mec->player_state();
            if (!state) continue;

            auto name = state->get_player_name_private().read_string();
            auto role = mec->get_player_role();

            auto mec_root = mec->get_root_component();
            if (!mec_root) continue;

            auto position = mec_root->get_relative_location();
            auto distance = MenuCalculateDistance(local_mec->get_net_location(), position);

            auto ghost_root = mec->get_ghost_root();
            bool is_ghost = false;
            double ghost_distance = 0.0;

            if (ghost_root) {
                auto ghostPosition = ghost_root->get_relative_location();
                ghost_distance = MenuCalculateDistanceMeters(position, ghostPosition);

                if (ghost_distance > 2) {
                    is_ghost = true; // User is a ghost
                }
            }

            // Determine the display name with [D] for dissident and [G] for ghost
            std::string display_name =
                (role == 4 ? std::string("[D]") : std::string("")) +
                (is_ghost ? std::string("[G]") : std::string("")) +
                name +
                " [" + distance + "m]";

            // Fetch the player's color from the configuration
            ImVec4 color = (role == 4) ? dissident_color : employee_color;

            // Add to the respective list
            if (role == 4) {
                dissidents.emplace_back(display_name, color);
            }
            else {
                employees.emplace_back(display_name, color);
            }

            // Calculate text width for dynamic sizing
            float text_width = ImGui::CalcTextSize(display_name.c_str()).x;
            max_text_width = (std::max)(max_text_width, text_width);
        }

        // Render Dissidents
        if (ImGui::CollapsingHeader("Dissidents", ImGuiTreeNodeFlags_DefaultOpen)) {
            for (const auto& [display_name, color] : dissidents) {
                ImGui::TextColored(color, "%s", display_name.c_str());
            }
        }

        // Render Employees
        if (ImGui::CollapsingHeader("Employees", ImGuiTreeNodeFlags_DefaultOpen)) {
            for (const auto& [display_name, color] : employees) {
                ImGui::TextColored(color, "%s", display_name.c_str());
            }
        }

        ImGui::EndChild();

        ImGui::SameLine(); // Place the next item on the same line

        ImGui::BeginChild("Mods",  ImVec2(childWidth, childHeight), true);

        ImGui::Checkbox("Invincble", &enable_invincible);

        ImGui::Checkbox("Stamina", &enable_stamina);

        ImGui::EndChild();
        
        ImGui::End();
    }

    void EnableMods() {
        // Wait for the data to be populated
        std::unique_lock<std::mutex> lock(globals::player_cache_mutex);
        globals::cv.wait(lock, []{ return globals::data_populated.load(); });

        if (enable_invincible) {
            globals::local_mec->set_health(10000);
        }

        if (enable_stamina) {
            globals::local_mec->set_stamina(1.);
        }
    }
}
