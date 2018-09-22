#pragma once

#include "Scene.hpp"

#include <glm/glm.hpp>

#include <map>


namespace SolarSystemSlices {


    struct GameObject {
        Scene::Transform *transform = nullptr;
        Scene::Object *object = nullptr;

        // Perform updates to the objects transform
        virtual void update(float elapsed) = 0;
    };



    struct Player : public GameObject {

        // For simplicity, velocity is 3 dimensional, but for now,
        // player's can't move in the z direction
        glm::vec3 velocity;
        std::map<std::string, uint32_t> collected_ingredients;
        


        
    };

    // Game is only concerned with maintaining:
    // - Positions of the players
    // - Positions of asteroids
    // - Positions and states of ingredients
    struct SolarSystemSlicesGame {
        
        // Maintain a list of players, so this game can scale
        std::vector< Player > players;
        // Bounds the players, position in the y-direction
        float MAX_PLAYER_Y = 10.0f;
        float MIN_PLAYER_Y = 10.0f;
        

        void update( float elapsed );

    };
}