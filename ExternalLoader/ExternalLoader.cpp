#include <iostream>
#include "TaskScheduler/Memory.hpp"

namespace actions {
	void Walkspeed(int val) {
		spdlog::warn("attempting to set Walkspeed to {}", val);
		driver::write_mem<float>(driver_handle, Globals::humanoid + Offsets::walkspeed, val);
		driver::write_mem<float>(driver_handle, Globals::humanoid + Offsets::walkspeedcheck, val);

		if (driver::read_mem<float>(driver_handle, Globals::humanoid + Offsets::walkspeed) != val) {
			spdlog::error("Failed to set Walkspeed");
			return;
		}
		
		spdlog::info("Walkspeed set to {}", val);
	}

	void Health(int val) {
		spdlog::warn("attempting to set Health to {}", val);
		driver::write_mem<float>(driver_handle, Globals::humanoid + Offsets::health, val);
		if (driver::read_mem<float>(driver_handle, Globals::humanoid + Offsets::health) != val) {
			spdlog::error("Failed to set Health");
			return;
		}
		spdlog::info("Health set to {}", val);
	}
}

void instancevoid() {
	Setup();


    bool firstrun = true;
    bool isDead = false;

    while (true) {
        Instance Datamodel(TaskScheduler::Datamodel::evaluate::GetDataModel());
        Globals::datamodel = Datamodel.Address;

        Instance Workspace(Datamodel.FindFirstChild("Workspace"));
        Globals::workspace = Workspace.Address;

        Instance Players(Datamodel.FindFirstChild("Players"));
        Instance LocalPlayer(driver::read_mem<uintptr_t>(driver_handle, Players.Address + Offsets::localplr));

        Instance Character(Workspace.FindFirstChild(LocalPlayer.GetName()));
        Globals::player = Character.Address;

        Instance Humanoid(Character.FindFirstChild("Humanoid"));
        Globals::humanoid = Humanoid.Address;

        Instance Camera(Workspace.FindFirstChild("Camera"));
        Globals::camera = Camera.Address;

        float health = driver::read_mem<float>(driver_handle, Globals::humanoid + Offsets::health);

        if (health < 1.0f) {
            if (!isDead) {
                spdlog::warn("Player Dead");
                isDead = true;
            }
        }
        else {
            if (isDead) {
				spdlog::warn("Player Alive");
                firstrun = true;
                isDead = false;
            }

            if (firstrun) {
                actions::Walkspeed(10000);
                actions::Health(10000);
                firstrun = false;
            }
        }
    }

}



int main() {
	instancevoid();

	CloseHandle(Globals::handle);
	CloseHandle(driver_handle);

}