#pragma once


#include "SolarSystemSlicesGame.hpp"

#include "Mode.hpp"
#include "MeshBuffer.hpp"
#include "GL.hpp"
#include "Connection.hpp"
#include "Scene.hpp"


#include <SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <vector>
#include <string>

namespace SolarSystemSlices {

    struct SolarSystemSlicesMode : public Mode {
        SolarSystemSlicesMode(Client &client_);
        virtual ~SolarSystemSlicesMode();

        //handle_event is called when new mouse or keyboard events are received:
        // (note that this might be many times per frame or never)
        //The function should return 'true' if it handled the event.
        virtual bool handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) override;

        //update is called at the start of a new frame, after events are handled:
        virtual void update(float elapsed) override;

        //draw is called after update:
        virtual void draw(glm::uvec2 const &drawable_size) override;

        void show_planet_selection_menu();
        void show_waiting_menu();
        void show_order_menu();
        void show_endscreen();


        //------- game state -------
        SolarSystemSlices::SolarSystemSlicesGame state;

        //------ networking ------
        Client &client; //client object; manages connection to server.
        void send_position();
        void send_planet_name();
        void receive_other_player_planet(Connection *C, uint32_t name_length);
        void receive_other_player_state();
        void add_other_player();
        void receive_order();


        
        //------ Controls --------
        struct {
            bool forward = false;
            bool backward = false;
            bool up = false;
            bool down = false;
            bool boost = false;
        } controls;

        //------- Scene Drawing -------
        Scene scene;
        
        Scene::Object *my_ship = nullptr;
        Scene::Object *other_ship = nullptr;
        
        Scene::Camera *player_camera = nullptr;

        // Maintain the player's index on the server
        int player_number = -1;
        bool game_full = false;
        bool both_players_ready = false;
        bool player_ready = false;
        Player *my_player;
        Player *other_player;
        bool finish_line_reached = false;

        bool winner_received = false;
        uint32_t winner;

        glm::vec3 start_position;

        std::vector< Scene::Object* > finish_line;


        // Background Obects
        std::vector< Scene::Object* > background_objects;
        // Bounding Asteroids
        std::vector< Scene::Object* > bounding_asteroids;

    };

}

