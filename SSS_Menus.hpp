#pragma once

#include "Mode.hpp"
#include "MenuMode.hpp"
#include "Connection.hpp"
#include "SolarSystemSlicesMode.hpp"

#include <functional>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

// Displays at the openning of the game
struct MainMenu : public MenuMode {
    MainMenu(Client &client_) {
        choices.emplace_back("SOLAR SYSTEM SLICES");
	    choices.emplace_back("PLAY", [&client_](){
		    Mode::set_current(std::make_shared< SolarSystemSlices::SolarSystemSlicesMode >(client_));
	    });
	    choices.emplace_back("QUIT", [](){
		    Mode::set_current(nullptr);
	    });

        selected = 1;
    }
};

// Waiting Menu has no selectable options, but gives a nice
// tint to the game while displaying text
struct WaitingMenu : public MenuMode {

    std::shared_ptr<SolarSystemSlices::SolarSystemSlicesMode> game_mode;
    
    WaitingMenu(std::shared_ptr<SolarSystemSlices::SolarSystemSlicesMode> game_mode_) {
        game_mode = game_mode_;
        background = game_mode;   
        choices.emplace_back("WAITING ON OTHER PLAYER");
    }

    // Checks the background game mode to see if
    // both players are ready
    virtual void update(float elapsed) override {
        bounce += elapsed / 0.7f;
	    bounce -= std::floor(bounce);

	    if (background) {
		    background->update(elapsed * background_time_scale);
	    }

        if (game_mode->game_full) {
            Mode::set_current(game_mode);
        }
    }
};

struct EndingScreen : public MenuMode {

    EndingScreen(std::shared_ptr<SolarSystemSlices::SolarSystemSlicesMode> game_mode_ , uint32_t winner) {
        background = game_mode_;
       
        if (winner == 0) {
            choices.emplace_back("TIE");
        } else {
            std::string player_color = (winner == 1) ? "RED" : "BLUE";
            choices.emplace_back(player_color + " WINS");
        }
         
        //choices.emplace_back("QUIT", [](){
		//    Mode::set_current(nullptr);
	    //});
        
        //selected = 1;
    }

};

// Displays the order on the screen and gives a count_down for when to begin
struct ShowOrderMenu : public MenuMode {

    std::shared_ptr<SolarSystemSlices::SolarSystemSlicesMode> game_mode;
    float order_show_time = 3.0f;
    bool  game_countdown_started = false;
    float game_countdown_time = 5.0f;
    float bout_showtime = 2.0f;
    bool order_shown = false;
    std::vector< std::string > order;

    
    ShowOrderMenu(std::shared_ptr<SolarSystemSlices::SolarSystemSlicesMode> game_mode_ ,  std::vector< std::string > order_) {
        order = order_;
        game_mode = game_mode_;
        background = game_mode;

        std::string p1_name = game_mode->state.player_1.planet;
        std::transform(p1_name.begin(), p1_name.end(), p1_name.begin(), ::toupper);
        choices.emplace_back(p1_name); 
        
        choices.emplace_back("VS"); 

        std::string p2_name = game_mode->state.player_2.planet;
        std::transform(p2_name.begin(), p2_name.end(), p2_name.begin(), ::toupper);
        choices.emplace_back(p2_name); 

        selected = 1;  
    }

    virtual void update(float elapsed) override {
        bounce += elapsed / 0.7f;
	    bounce -= std::floor(bounce);

	    if (background) {
		    background->update(elapsed * background_time_scale);
	    }

        // Apply time for how long the
        if (bout_showtime > 0 ) {
            bout_showtime -= elapsed;
        }
        else {
            if (!order_shown) {
                order_shown = true;
                selected = 0; 
                choices.clear();
                choices.emplace_back("THE ORDER"); 
                for (uint32_t i = 0; i < order.size(); i++) {
                    std::string item = order[i];
                    std::transform(item.begin(), item.end(), item.begin(), ::toupper);
                    choices.emplace_back(item);
                }
            } else {
                if (order_show_time > 0) {
                    order_show_time -= elapsed;
                } else {
                    if (!game_countdown_started) {
                        game_countdown_started = true;
                        // Clear the choices and display only the current time left
                        choices.clear();
                        choices.emplace_back("START YOUR ENGINES");
                    }
                    else {
                        game_countdown_time -= elapsed;

                        if (game_countdown_time < 3.0f ) {
                            choices.clear();
                            choices.emplace_back("GET SET");
                        }

                        if (game_countdown_time < 1.0f ) {
                            choices.clear();
                            choices.emplace_back("GO");
                        }

                        if (game_countdown_time <= 0 ) {
                            game_mode->state.game_started = true;
                            Mode::set_current(game_mode);
                        }
                    }
                }
            }
        }
    }
};


