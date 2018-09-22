#pragma once


#include "SolarSystemSlicesGame.hpp"
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

    const std::string star_vert_shader = 
        "#version 330\n"
        "layout(location=0) in vec4 position;\n"
        "void main() {\n"
        "   gl_Position = position;\n"
        "}\n"
        ;

    const std::string star_frag_shader =
        "#version 330\n" 
        "out vec4 out_color;\n"
        "void main () {\n"
        "   out_color = vec4(1.0, 1.0, 1.0, 1.0);\n"
        "}\n"
        ;

    
    // Maintains the position of one of the stars drawn in the background
    struct Star {
        glm::vec2 position;
        float distance;
        static const float MAX_DISTANCE;

        Star(glm::vec2 position_, float distance_) {
            position = position_;
            distance = distance_;
        }

        bool is_offscreen() {
            return position.x > 1.1f || position.x < -1.1f || position.y > 1.1f || position.y < -1.1f;
        }

        void update(float elapsed, glm::vec2 player_velocity) {
            position -= player_velocity / (distance * 10);
            
            //position = position - (glm::vec2(0.01f / distance,0));

            if (is_offscreen()) {
                position.x = 1.0f;
                position.y = (float)(std::rand() % 200) / 100.0f - 1.0f;
            }
        }

        static std::vector<Star> generate_stars(uint32_t num_stars);
    };

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

        //------- game state -------
        SolarSystemSlices::SolarSystemSlicesGame state;

        //------ networking ------
        Client &client; //client object; manages connection to server.

        // TODO: Move the star code to somewhere more appropriate
        std::vector<Star> stars = Star::generate_stars(50);

        struct {
            bool forward = false;
            bool backward = false;
            bool up = false;
            bool down = false;
            bool boost = false;
        } controls;


        Scene scene;
        Scene::Object *player_1 = nullptr;
        Scene::Object *player_2 = nullptr;
        Scene::Camera *player_camera = nullptr;

        // Maintain the player's index on the server
        uint32_t player_index = -1U;

        // Stars do not need to sync accross all players
        std::vector< Scene::Object* > background_objects;
        
        
    };

}

