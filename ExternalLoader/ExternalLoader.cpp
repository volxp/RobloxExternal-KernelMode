#include <iostream>

#include "overlay/overlay.h"





void runui() {
    Overlay overlay;
    if (!overlay.Initialize()) {
        spdlog::error("Failed to initialize overlay");
        return;
    }
    spdlog::info("Overlay Running");
    overlay.Run();
    spdlog::info("Overlay Closed");
    overlay.Shutdown();

}

void instancevoid() {
	Setup();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    bool firstrun = true;
    bool isDead = false;
    bool isuirendered = false;
    bool spampldead = true;
	bool spamplalive = true;
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
		if (health <= 0) {
			isDead = true;
			if (spampldead)
			    spdlog::error("Player is dead");
			    spampldead = false;
				spamplalive = true;
                
		}
		else {
			isDead = false;
            if (spamplalive)
			    spdlog::info("Player is alive");
			    spamplalive = false;
                spampldead = true;

		}

        if (!isuirendered ) {
			
            if (!isDead) {
                std::thread(runui).detach();
                isuirendered = true;
			}
            else {
                spdlog::error("Player is dead, not rendering UI");
            }
        }




    }

}



int main() {

    
    instancevoid();
	//std::thread(instancevoid).detach();
    //std::thread(runui).detach();

	CloseHandle(Globals::handle);
	CloseHandle(driver_handle);

}