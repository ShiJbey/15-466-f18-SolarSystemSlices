#pragma once

#include "Scene.hpp"

#include <glm/glm.hpp>

#include <map>
#include <random>
#include <iterator>
#include <ctime>
#include <algorithm>
#include <iostream>


namespace SolarSystemSlices {

    // References an ingredient floating through space
    struct Ingredient {
        std::string name;
        Scene::Object* object = nullptr;
        glm::vec3 position = glm::vec3(0.0f);
        uint32_t id = -1U;
        bool active;

        Ingredient(std::string name_, uint32_t id_) {
            name = name_;
            id = id_;
        }
    };

    struct Asteroid {
        Scene::Object* object = nullptr;
        glm::vec3 position;
    };

    struct Player {
        glm::vec3 position = glm::vec3(0.0f);
        glm::quat rotation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
		glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);
        std::string planet = "";

        std::map<std::string, uint32_t> collected_ingredients;
        float arrival_time = 0.0f;

        uint32_t missing_items(std::vector< std::string > order) {
            uint32_t num_missing = 0;
            for (auto it = collected_ingredients.begin(); it != collected_ingredients.end(); it++) {
               if (std::find(order.begin(), order.end(), it->first) == order.end()) {
                   num_missing++;
               }
            }
            return num_missing;
        } 

        uint32_t get_total_collected() {
            uint32_t total = 0;
            for (auto it = collected_ingredients.begin(); it != collected_ingredients.end(); it++) {
                total += it->second;
            }
            return total;
        }   
    };

    // Game is only concerned with maintaining:
    // - Positions of the players
    // - Positions of asteroids
    // - Positions and states of ingredients
    struct SolarSystemSlicesGame {

        

        bool game_started = false;
        
        // Maintain a list of players, so this game can scale
        Player player_1;
        Player player_2;
        bool player_2_finished = false;
        bool player_1_finished = false;

        std::vector< Ingredient > floating_ingredients;
        std::vector< Asteroid > asteroids;

        std::vector< std::string > current_order;

        // Bounds the players, position in the y-direction
        float MAX_PLAYER_Y = 15.0f;
        float MIN_PLAYER_Y = -5.0f;
        float track_length = 100.0f;
        uint32_t max_ingredients = 10;

        std::map<char, std::string> ingredient_map
        {
            std::make_pair('D',"Dough"),
            std::make_pair('C',"Cheese"),
            std::make_pair('B',"BellPepper"),
            std::make_pair('P',"Pepperoni"),
            std::make_pair('T',"Tomato")
        };


        
        void update( float elapsed );

        uint32_t get_winner() {
            if(player_1.missing_items(current_order) < player_2.missing_items(current_order)) {
                std::cout << "RED is missing items" << std::endl;
                return 1;
            } else if (player_1.missing_items(current_order) > player_2.missing_items(current_order)) {
                std::cout << "BLUE is missing items" << std::endl;
                return 2;
            } else {
                if (player_1.get_total_collected() > player_2.get_total_collected()) {
                    std::cout << "RED collected more items" << std::endl;
                    return 1;
                } else if (player_1.get_total_collected() < player_2.get_total_collected()) {
                    std::cout << "BLUE collected more items" << std::endl;
                    return 2;
                } else {
                    std::cout << "TIE" << std::endl;
                    return 0;
                }
            }
        }


        // Randomly places asteroid on the path of the players
        void populate_asteroids(uint32_t num_asteroids) {
            std::srand((uint32_t)std::time(NULL));
            for (uint32_t i = 0; i < num_asteroids; i++) {
                Asteroid ast;
                // Produce a random y value between the min and Max Y
                ast.position.y = ((float)(std::rand() % 100) / 100.0f) * (MAX_PLAYER_Y * 2) - MAX_PLAYER_Y;
                ast.position.x = 30.0f + ((float)(std::rand() % 100) / 100.0f) * track_length;
                asteroids.push_back(ast);
            }
        }


        // Determines the placement of ingredients in the world
        // This is if I dont just use a blender scene
        void populate_ingredients(uint32_t num_of_each_ingredient) {
            uint32_t ingredients_added = 0;
            for (uint32_t i = 0; i < current_order.size(); i++) {
                for (uint32_t c = 0; c < num_of_each_ingredient; c++) {
                    Ingredient ingredient(current_order[i], ingredients_added);
                    ingredients_added++;

                    // Randomize the x and y
                    ingredient.position.y = ((float)(std::rand() % 100) / 100.0f) * (MAX_PLAYER_Y * 2) - MAX_PLAYER_Y;
                    ingredient.position.x = 30.0f + ((float)(std::rand() % 100) / 100.0f) * track_length;

                    floating_ingredients.push_back(ingredient);
                }
            }

        }


        // Prints the current order
        void print_order() {
            printf("Order:\n");
            for (uint32_t i = 0; i < current_order.size(); i++) {
                printf("\t%s\n", current_order[i].c_str());
            }
        }

        // Creates a new order using the supported topping choices
        void generate_order() {
            // Clear the current order
            current_order.clear();

            // Supported Ingredients
            std::vector< std::string > choices = {
                "Cheese",
                "BellPepper",
                "Pepperoni",
                "Tomato"
            };
        
            // All pizzas need crust
            current_order.push_back("Dough");

            // Randompy choose a number of desired topings [1, choices.length()]
            std::srand((uint32_t)std::time(NULL));
            uint32_t num_toppings = (std::rand() % choices.size()) + 1;

            // Create a new order (not repeating toppings)
            for (uint32_t i = 0; i < num_toppings; i++) {
                uint32_t topping_index = std::rand() % choices.size();
                current_order.push_back(choices[topping_index]);
                choices.erase(choices.begin() + topping_index);
            }
        }

    };
}