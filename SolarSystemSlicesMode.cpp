#include "SolarSystemSlicesMode.hpp"
#include "SolarSystemSlicesGame.hpp"
#include "SSS_Menus.hpp"

#include "MenuMode.hpp"
#include "Load.hpp"
#include "MeshBuffer.hpp"
#include "Scene.hpp"
#include "gl_errors.hpp" //helper for dumpping OpenGL error messages
#include "read_chunk.hpp" //helper for reading a vector of structures from a file
#include "data_path.hpp" //helper to get paths relative to executable
#include "compile_program.hpp" //helper to compile opengl shader programs
#include "draw_text.hpp" //helper to... um.. draw text
#include "vertex_color_program.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <map>
#include <cstddef>
#include <random>
#include <ctime>
#include <math.h>
#include <cmath>
#include <cctype>
#include <memory>

Load< MeshBuffer > sss_meshes(LoadTagDefault, [](){
	return new MeshBuffer(data_path("sss_assets.pnc"));
});

Load< GLuint > sss_meshes_for_vertex_color_program(LoadTagDefault, [](){
	return new GLuint(sss_meshes->make_vao_for_program(vertex_color_program->program));
});

SolarSystemSlices::SolarSystemSlicesMode::SolarSystemSlicesMode( Client &client_ ) : client( client_ ) {
    
    // Initiate a connection to a server lobby 
    client.connection.send_raw("h", 1);

    // Get a player number from the server lobby
    while (player_number == -1) {
        client.poll([&](Connection *c, Connection::Event event){
            if (event == Connection::OnOpen) {
                //probably won't get this.
            } else if (event == Connection::OnClose) {
                std::cerr << "Lost connection to server." << std::endl;
            } else { assert(event == Connection::OnRecv);

                // Check for player number
                if (c->recv_buffer[0] = 'N') {
                    if (c->recv_buffer.size() < 1 + sizeof(int)) {
                        return;
                    } else {
                        std::cout << "Received Player Number" << std::endl;
                        c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1);
                        memcpy(&player_number, c->recv_buffer.data(), sizeof(int));
                        c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + sizeof(int));
                        printf("Connected to lobby as player: %d\n", player_number);

                        if (player_number == 1) {
                            my_player = &state.player_1;
                            other_player = &state.player_2;
                        } else {
                            my_player = &state.player_2;
                            other_player = &state.player_1;
                        }
                    }  
                }

                std::cerr << "Ignoring " << c->recv_buffer.size() << " bytes from server." << std::endl;
            }
        });
    }

    auto attach_object = [this](Scene::Transform *transform, std::string const &name)
    {
        Scene::Object *object = scene.new_object(transform);
        object->program = vertex_color_program->program;
        object->program_mvp_mat4 = vertex_color_program->object_to_clip_mat4;
        object->program_mv_mat4x3 = vertex_color_program->object_to_light_mat4x3;
        object->program_itmv_mat3 = vertex_color_program->normal_to_light_mat3;
        object->vao = *sss_meshes_for_vertex_color_program;
        MeshBuffer::Mesh const &mesh = sss_meshes->lookup(name);
        object->start = mesh.start;
        object->count = mesh.count;
        return object;
    };

    { // Add the player and the camera

        int other_player = (player_number == 1) ? 2 : 1;
        if (player_number == 1) {
            // Create the object for player 1
            Scene::Transform *transform_1 = scene.new_transform();
            transform_1->position = glm::vec3(1.0f, 7.0f, 0.0f);
            transform_1->scale = glm::vec3(0.5f);
            transform_1->rotation *= glm::angleAxis((float)M_PI / 2.0f, glm::vec3(0.0f, 1.0f, 0.0f));
            transform_1->rotation *= glm::angleAxis((float)M_PI, glm::vec3(1.0f, 0.0f, 0.0f));
            my_ship = attach_object(transform_1, "Player" + std::to_string(player_number));
            start_position = transform_1->position;
        } else {
            // Create the object for player 1
            Scene::Transform *transform_1 = scene.new_transform();
            transform_1->position = glm::vec3(1.0f, 5.0f, 0.0f);
            transform_1->scale = glm::vec3(0.5f);
            transform_1->rotation *= glm::angleAxis((float)M_PI / 2.0f, glm::vec3(0.0f, 1.0f, 0.0f));
            transform_1->rotation *= glm::angleAxis((float)M_PI, glm::vec3(1.0f, 0.0f, 0.0f));
            my_ship = attach_object(transform_1, "Player" + std::to_string(player_number));
            start_position = transform_1->position;
        }
        
        // Create Camera
        Scene::Transform *camera_transform = scene.new_transform();
        camera_transform->rotation *= glm::angleAxis((float)M_PI, glm::vec3(0.0f, 0.0f, 1.0f));
        camera_transform->position = glm::vec3(11.0f, 5.0f, 20.0f);
        player_camera = scene.new_camera(camera_transform);
    }

    { // Create and place the stars with quad meshes
        std::srand((uint32_t)std::time(NULL));
        Scene::Transform *star_trans = nullptr;
        
        uint32_t num_background_objects = 35;
       
        for (uint32_t i = 0; i < num_background_objects; i++) {
            // Create a new star
            star_trans = scene.new_transform();
            star_trans->position = player_camera->transform->position;
            star_trans->position.x += ((float)(std::rand() % 100) / 100.0f) * 20.0f - 10.0f;
            star_trans->position.y += ((float)(std::rand() % 100) / 100.0f) * 20.0f - 10.0f;
            star_trans->rotation *= glm::angleAxis((float)M_PI / 2.0f, glm::vec3(0.0f, 0.0f, 1.0f));
            float dist = std::min(5.0f, std::max(1.0f, (float)(std::rand() % 7)));
            star_trans->position.z = -dist;
            star_trans->scale = glm::vec3(0.08f / dist, 0.1f / dist, 1.0f);
            Scene::Object * star_obj = attach_object(star_trans, "Star");
            background_objects.push_back(star_obj);
        }
    }

    { // Create the asteroids that will line the top and bottom of the screen
        uint32_t asteroids_per_row = 16;
        Scene::Transform *temp_trans = nullptr;

        // Create the top set of asteroids
        for (uint32_t i = 0; i < asteroids_per_row; i++) {
            temp_trans = scene.new_transform();
            temp_trans->position.y = state.MAX_PLAYER_Y;
            temp_trans->position.x = 1.5f * i;
            temp_trans->scale = glm::vec3(0.5f, 0.5f, 0.5f);
            Scene::Object *asteroid_object = attach_object(temp_trans, "Asteroid");
            bounding_asteroids.push_back(asteroid_object);
        }

        // Create the bottom set of asteroids
        for (uint32_t i = 0; i < asteroids_per_row; i++) {
            temp_trans = scene.new_transform();
            temp_trans->position.y = state.MIN_PLAYER_Y;
            temp_trans->position.x = 1.5f * i;
            temp_trans->scale = glm::vec3(0.5f, 0.5f, 0.5f);
            Scene::Object *asteroid_object = attach_object(temp_trans, "Asteroid");
            bounding_asteroids.push_back(asteroid_object);
        }
    }

    { // Use quads to represent the finish line

        for (uint32_t r = 0; r < 15; r++) {
            for (uint32_t c = 0; c < 3; c++) {
                if ((r % 2  == 0 && c % 2 == 1 ) || (r % 2  == 1 && c % 2 == 0)) {
                    // Place quad
                    Scene::Transform *temp_transform = scene.new_transform();
                    temp_transform->position.x = state.track_length + 30.0f + (2.0f * (float)c);
                    temp_transform->position.y = -9.0f + (2.0f * (float)r);
                    temp_transform->position.z = -2.0f;
                    Scene::Object *temp_object = attach_object(temp_transform, "Star");
                    finish_line.push_back(temp_object);
                }
            }
        }

    }
}

SolarSystemSlices::SolarSystemSlicesMode::~SolarSystemSlicesMode() {
    // Do Nothing
}

bool SolarSystemSlices::SolarSystemSlicesMode::handle_event( SDL_Event const &evt, glm::uvec2 const &window_size ) {
    if (!state.game_started) {
        return false;
    }

    if (evt.type == SDL_KEYDOWN && evt.key.repeat) {
		return false;
	}

    if (evt.type == SDL_KEYDOWN || evt.type == SDL_KEYUP) {
        if (evt.key.keysym.scancode == SDL_SCANCODE_W) {
            controls.up = (evt.type == SDL_KEYDOWN);
            return true;
        } else if (evt.key.keysym.scancode == SDL_SCANCODE_S) {
            controls.down = (evt.type == SDL_KEYDOWN);
            return true;
        } else if (evt.key.keysym.scancode == SDL_SCANCODE_A) {
            controls.backward = (evt.type == SDL_KEYDOWN);
            return true;
        } else if (evt.key.keysym.scancode == SDL_SCANCODE_D) {
            controls.forward = (evt.type == SDL_KEYDOWN);
            return true;
        } else if (evt.key.keysym.scancode == SDL_SCANCODE_SPACE) {
            controls.boost = (evt.type == SDL_KEYDOWN);
            return true;
        }
    }

    return false;
}

void SolarSystemSlices::SolarSystemSlicesMode::send_position() {
    client.connection.send_raw("s", 1);
    client.connection.send_raw(&my_ship->transform->position.x, sizeof(float));
    client.connection.send_raw(&my_ship->transform->position.y, sizeof(float));
}

void SolarSystemSlices::SolarSystemSlicesMode::send_planet_name() {
    // Send signal to the server that we are ready;
    client.connection.send_raw("P", 1);
    // Send the planet name length
    uint32_t planet_name_length = (uint32_t)my_player->planet.size();
    client.connection.send_raw(&planet_name_length, sizeof(uint32_t));
    // Send planet name
    client.connection.send_raw(&my_player->planet, planet_name_length);
}

void SolarSystemSlices::SolarSystemSlicesMode::receive_other_player_planet(Connection *c, uint32_t name_length) {
    c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1);
    c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + sizeof(uint32_t));

    char *planet_name = new char[name_length + 1];
    planet_name[name_length] = '\0';
    memcpy(planet_name, c->recv_buffer.data(), name_length);
    other_player->planet = std::string(planet_name);
    c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + name_length);

    std::cout << "Received other Player's Planet: " << other_player->planet << std::endl;
}

void SolarSystemSlices::SolarSystemSlicesMode::add_other_player() {
    
}

void SolarSystemSlices::SolarSystemSlicesMode::receive_order() {
    
}


void SolarSystemSlices::SolarSystemSlicesMode::update( float elapsed ) {

    if (Mode::current.get() == this && !game_full) {
        show_waiting_menu();
    }

    if (Mode::current.get() == this && my_player->planet == "") {
        show_planet_selection_menu();
    }

    if (!both_players_ready && my_player->planet != ""  && other_player->planet != "") {
        show_order_menu();
        std::cout << "both players ready!!" << std::endl;
        both_players_ready = true;   
    }

    if (winner_received) {
        show_endscreen();
    }

    glm::vec3 player_vel(10.0f, 0.0f, 0.0f);
    glm::vec3 player_displacement(0.0f);
    float player_rot = 0.0f;

    // Adjust position of ship on the screen horizontally
    if (controls.forward) {
       // player_vel += glm::vec3(15.0f, 0.0f, 0.0f);
        player_displacement.x += 3.0f * elapsed;
    }
    if (controls.backward) {
       // player_vel += glm::vec3(-5.0f, 0.0f, 0.0f);
        player_displacement.x -= 5.0f * elapsed;
    }
    if (controls.up) {
        player_rot -= (float)M_PI / 4.0f;
        player_vel += glm::vec3(0.0f, 15.0f, 0.0f);
        player_displacement.y += 5.0f * elapsed;
    }
    if (controls.down) {
        player_rot += (float)M_PI / 4.0f;
        player_vel += glm::vec3(0.0f, -15.0f, 0.0f);
        player_displacement.y -= 5.0f * elapsed;
    }
    if (controls.boost) {
        player_displacement.x += 1.5f * elapsed;
       // player_vel *= 2;
    }

    my_ship->transform->position += player_displacement;

    { // Update the objects in the background
        for (uint32_t i = 0; i < background_objects.size(); i++) {
            // Scale the displacement to give the impression of paralax
            background_objects[i]->transform->position.x -= (player_vel.x * elapsed) / std::abs(background_objects[i]->transform->position.z);

            // Recycle objects after they pass out of the camera's view
            if (player_camera->transform->position.x - background_objects[i]->transform->position.x >= 15.0f) {
                // Moves objects in front of the player
                background_objects[i]->transform->position.x = player_camera->transform->position.x + 15.0f;

                // Randomizes a new distance and rescales
                float dist = std::min(5.0f, std::max(1.0f, (float)(std::rand() % 7)));
                background_objects[i]->transform->position.z = -dist;
                background_objects[i]->transform->scale = glm::vec3(0.1f / dist, 0.3f / dist, 1.0f);

                // Randomizes its y-position
                background_objects[i]->transform->position.y = player_camera->transform->position.y + ((float)(std::rand() % 100) / 100.0f) * 16.0f - 8.0f;                
            }
        }
    }

    { // Update floating ingredient and asteroid  positions
        if (state.game_started) {
            // Update ingredients
            for (uint32_t i = 0; i < state.floating_ingredients.size(); i++) {
                state.floating_ingredients[i].object->transform->position.x -= (player_vel.x * elapsed);
                // Check for collisions
                if (glm::distance(my_ship->transform->position, state.floating_ingredients[i].object->transform->position) < 0.3f) {
                    std::cout << "Collided with Ingredient: " << state.floating_ingredients[i].id << std::endl;
                    
                    // Tell the sever that this ingredient was collided with
                    if (client.connection) {
                        client.connection.send_raw("H", 1);
                        client.connection.send_raw(&state.floating_ingredients[i].id, sizeof(uint32_t));
                    }

                }
            }

            // Update asteroids
            for (uint32_t i = 0; i < state.asteroids.size(); i++) {
                state.asteroids[i].object->transform->position.x -= (player_vel.x * elapsed);
                // Check for collisions
                if (glm::distance(my_ship->transform->position, state.asteroids[i].object->transform->position) < 0.75f) {
                    std::cout << "Collided with asteroid" << std::endl;
                    my_ship->transform->position = start_position;
                }
            }

            // Move the finish line
            for (uint32_t i = 0; i < finish_line.size(); i++) {
                finish_line[i]->transform->position.x -= (player_vel.x * elapsed);
            }


            

        }
        
    }

    if (finish_line[0]->transform->position.x < my_ship->transform->position.x) {
        finish_line_reached = true;
    }

    if (client.connection) {

		//send game state to server:
		send_position();

        // Send what planet the player is from
        if (!player_ready && my_player->planet != "") {
            send_planet_name();            
            player_ready = true; 
        }

        if (finish_line_reached) {
            client.connection.send_raw("D", 1);
        }
    }

	client.poll([&](Connection *c, Connection::Event event){
		if (event == Connection::OnOpen) {
			//probably won't get this.
		} else if (event == Connection::OnClose) {
			std::cerr << "Lost connection to server." << std::endl;
            Mode::set_current(nullptr);
		} else { assert(event == Connection::OnRecv);

            if(c->recv_buffer[0] == 'H') {
                if (c->recv_buffer.size() < 1 + sizeof(uint32_t)) {
					return;
                } else {
                    c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1);
                    uint32_t ingredient_to_remove = -1U;
                    memcpy(&ingredient_to_remove, c->recv_buffer.data(), sizeof(uint32_t));
                    c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + sizeof(uint32_t));

                    // Remove the item from indgredient vector
                    for (uint32_t i = 0; i < state.floating_ingredients.size(); i++) {
                        if (state.floating_ingredients[i].id == ingredient_to_remove) {
                            std::cout <<"DELETING" << state.floating_ingredients[i].id << " FROM HIERARCHY" << std::endl;
                            scene.delete_object(state.floating_ingredients[i].object);
                            state.floating_ingredients.erase(state.floating_ingredients.begin() + i);
                            break;
                        }
                    }
                }
            }


            if(c->recv_buffer[0] == 'R') {
                if (c->recv_buffer.size() < 1 + sizeof(uint32_t)) {
					return;
                } else {
                    c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1);
                    uint32_t winner = -1U;
                    memcpy(&winner, c->recv_buffer.data(), sizeof(uint32_t));
                    c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + sizeof(uint32_t));
                    winner_received = true;
                }
            }


            if(c->recv_buffer[0] == 'P') {
                if (c->recv_buffer.size() < 1 + sizeof(uint32_t)) {
					return;
                } else {
                    uint32_t other_player_planet_name_length;
                    memcpy(&other_player_planet_name_length, c->recv_buffer.data() + 1, sizeof(uint32_t));

                    if (c->recv_buffer.size() < 1 + sizeof(uint32_t) + (1 * other_player_planet_name_length)) {
                        return;
                    } else {
                        //receive_other_player_planet(c, other_player_planet_name_length);
                        
                        c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1);
                        c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + sizeof(uint32_t));

                        char *planet_name = new char[other_player_planet_name_length + 1];
                        planet_name[other_player_planet_name_length] = '\0';
						memcpy(planet_name, c->recv_buffer.data(), other_player_planet_name_length);
                        other_player->planet = std::string(planet_name);
                        c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + other_player_planet_name_length);

                        std::cout << "Received other Player's Planet: " << other_player->planet << std::endl;
                        
                    }
                }
            }

            if (c->recv_buffer[0] == 'A') {
                if (c->recv_buffer.size() < 1 + sizeof(uint32_t)) {
                    return;
                } else {
                    uint32_t num_asteroids;
                    memcpy(&num_asteroids, c->recv_buffer.data() + 1, sizeof(uint32_t));
                    if (c->recv_buffer.size() < 1 + sizeof(uint32_t) + (2 * sizeof(float) * num_asteroids)) {
                        return;
                    } else {

                        auto attach_object = [this](Scene::Transform *transform, std::string const &name)
                        {
                            Scene::Object *object = scene.new_object(transform);
                            object->program = vertex_color_program->program;
                            object->program_mvp_mat4 = vertex_color_program->object_to_clip_mat4;
                            object->program_mv_mat4x3 = vertex_color_program->object_to_light_mat4x3;
                            object->program_itmv_mat3 = vertex_color_program->normal_to_light_mat3;
                            object->vao = *sss_meshes_for_vertex_color_program;
                            MeshBuffer::Mesh const &mesh = sss_meshes->lookup(name);
                            object->start = mesh.start;
                            object->count = mesh.count;
                            return object;
                        };

                        c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1);
                        c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + sizeof(uint32_t));

                        for (uint32_t i = 0; i < num_asteroids; i++) {
                            Asteroid ast;

                            memcpy(&ast.position.x, c->recv_buffer.data(), sizeof(float));
                            c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + sizeof(float));
                            memcpy(&ast.position.y, c->recv_buffer.data(), sizeof(float));
                            c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + sizeof(float));

                            Scene::Transform *ast_transform = scene.new_transform();
                            ast_transform->position.x = ast.position.x;
                            ast_transform->position.y = ast.position.y;
                            ast_transform->scale = glm::vec3(0.75f, 0.75f, 0.75f);

                            ast.object = attach_object(ast_transform, "Asteroid");
                            
                            state.asteroids.push_back(ast);
                        }
                    }
                }
            }

            if (c->recv_buffer[0] == 'I') {
                if (c->recv_buffer.size() < 1 + sizeof(uint32_t)) {
                    return;
                } else {
                    uint32_t num_ingredients;
                    memcpy(&num_ingredients, c->recv_buffer.data() + 1, sizeof(uint32_t));
                    if (c->recv_buffer.size() < 1 + sizeof(uint32_t) + (1 + sizeof(uint32_t) + sizeof(float) + sizeof(float)) * num_ingredients) {
                        return;
                    } else {
                        std::cout << "Spawning " << num_ingredients << " ingredients..." << std::endl;

                        auto attach_object = [this](Scene::Transform *transform, std::string const &name)
                        {
                            Scene::Object *object = scene.new_object(transform);
                            object->program = vertex_color_program->program;
                            object->program_mvp_mat4 = vertex_color_program->object_to_clip_mat4;
                            object->program_mv_mat4x3 = vertex_color_program->object_to_light_mat4x3;
                            object->program_itmv_mat3 = vertex_color_program->normal_to_light_mat3;
                            object->vao = *sss_meshes_for_vertex_color_program;
                            MeshBuffer::Mesh const &mesh = sss_meshes->lookup(name);
                            object->start = mesh.start;
                            object->count = mesh.count;
                            return object;
                        };

                        c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1);
                        c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + sizeof(uint32_t));

                        for (uint32_t i = 0; i < num_ingredients; i++) {
                            // Get the ingredient name
                            char ingredient_type = c->recv_buffer[0];
                            c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1);
                            uint32_t id;
                            memcpy(&id, c->recv_buffer.data(), sizeof(uint32_t));
                            c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + sizeof(uint32_t));
                            Ingredient ingredient(state.ingredient_map[ingredient_type], id);


                            memcpy(&ingredient.position.x, c->recv_buffer.data(), sizeof(float));
                            c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + sizeof(float));
                            memcpy(&ingredient.position.y, c->recv_buffer.data(), sizeof(float));
                            c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + sizeof(float));

                            Scene::Transform *ingredient_transform = scene.new_transform();
                            ingredient_transform->position.x = ingredient.position.x;
                            ingredient_transform->position.y = ingredient.position.y;
                            ingredient_transform->scale = glm::vec3(0.75f, 0.75f, 0.75f);

                            ingredient.object = attach_object(ingredient_transform, ingredient.name);
                            
                            state.floating_ingredients.push_back(ingredient);
                        }
                    }
                }
            }

            // Update the position of the other ship
            if (c->recv_buffer[0] == 's') {
                if (c->recv_buffer.size() < 1 + sizeof(float) + sizeof(float)) {
					return; //wait for more data
				} else {
                    if (other_ship != nullptr) {
                        c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1);
                        
                        memcpy(&other_ship->transform->position.x, c->recv_buffer.data(), sizeof(float));
                        c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + sizeof(float));
                        
                        memcpy(&other_ship->transform->position.y, c->recv_buffer.data(), sizeof(float));
                        c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + sizeof(float));
                    } else {
                        c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1 + sizeof(float) + sizeof(float));
                    }
                }
            }
            
            // Check if the other player is connected
            if(c->recv_buffer[0] == 'C') {
                c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1);

                auto attach_object = [this](Scene::Transform *transform, std::string const &name)
                {
                    Scene::Object *object = scene.new_object(transform);
                    object->program = vertex_color_program->program;
                    object->program_mvp_mat4 = vertex_color_program->object_to_clip_mat4;
                    object->program_mv_mat4x3 = vertex_color_program->object_to_light_mat4x3;
                    object->program_itmv_mat3 = vertex_color_program->normal_to_light_mat3;
                    object->vao = *sss_meshes_for_vertex_color_program;
                    MeshBuffer::Mesh const &mesh = sss_meshes->lookup(name);
                    object->start = mesh.start;
                    object->count = mesh.count;
                    return object;
                };

                std::cout << "Other player connected" << std::endl;
                if (player_number == 1) {
                    // Create a transform for player2
                    Scene::Transform *transform_2 = scene.new_transform();
                    transform_2->position = glm::vec3(1.0f, 5.0f, 0.0f);
                    transform_2->scale = glm::vec3(0.5f);
                    transform_2->rotation *= glm::angleAxis((float)M_PI / 2.0f, glm::vec3(0.0f, 1.0f, 0.0f));
                    transform_2->rotation *= glm::angleAxis((float)M_PI, glm::vec3(1.0f, 0.0f, 0.0f));
                    other_ship = attach_object(transform_2, "Player2");
                } else {
                    // Create a transform for player2
                    Scene::Transform *transform_2 = scene.new_transform();
                    transform_2->position = glm::vec3(1.0f, 7.0f, 0.0f);
                    transform_2->scale = glm::vec3(0.5f);
                    transform_2->rotation *= glm::angleAxis((float)M_PI / 2.0f, glm::vec3(0.0f, 1.0f, 0.0f));
                    transform_2->rotation *= glm::angleAxis((float)M_PI, glm::vec3(1.0f, 0.0f, 0.0f));
                    other_ship = attach_object(transform_2, "Player1");
                }
                game_full = true;
            }

            // Receive the order from the server
            if (c->recv_buffer[0] == 'O') {
                std::cout << "Received an order signal" << std::endl;
                uint32_t num_ingredients = 0;
                // An order is on the way, wait until we recieve the number of ingredients
                if (c->recv_buffer.size() < 1 + sizeof(num_ingredients)) { 
                    return; // We need to know
                } else {
                    
                    // Read the size and check to make sure their are enough things in the buffer
                    
                    memcpy(&num_ingredients, c->recv_buffer.data() + 1, sizeof(num_ingredients));
                    std::cout << "Received order size: " << num_ingredients << std::endl;                    


                    if (c->recv_buffer.size() < 1 + sizeof(num_ingredients) + (1 * num_ingredients)) {
                        return;
                    } else {
                        // Erase the message type and number of items in the order
                        c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1);
                        c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + sizeof(num_ingredients));

                        state.current_order.clear();
                        
                        // Loop and read off each one of the ingredients
                        for (uint32_t i = 0; i < num_ingredients; i++) {
                            char ingredient_char = c->recv_buffer[0];
                            state.current_order.push_back(state.ingredient_map[ingredient_char]);
                            c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1);
                        }

                        state.print_order();


                    }
                }
            }
                
			//std::cerr << "Ignoring " << c->recv_buffer.size() << " bytes from server." << std::endl;
		}
	});
}

void SolarSystemSlices::SolarSystemSlicesMode::draw( glm::uvec2 const &drawable_size ) {

    glClearColor( 0.1f, 0.0f, 0.1f, 0.0f);
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glEnable( GL_DEPTH_TEST );
    glEnable( GL_BLEND );
    glBlendEquation( GL_FUNC_ADD );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    if (1) {

        { // Draw the scene
            glUseProgram(vertex_color_program->program);

            glUniform3fv(vertex_color_program->sun_color_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 1.0f)));
            glUniform3fv(vertex_color_program->sun_direction_vec3, 1, glm::value_ptr(glm::normalize(glm::vec3(-0.2f, 0.2f, 1.0f))));
            glUniform3fv(vertex_color_program->sky_color_vec3, 1, glm::value_ptr(glm::vec3(0.2f, 0.2f, 0.3f)));
            glUniform3fv(vertex_color_program->sky_direction_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 1.0f, 0.0f)));

            scene.draw(player_camera);
        }

        if (Mode::current.get() == this && other_player->planet == "") {

            glDisable(GL_DEPTH_TEST);
            
            std::string message = "WAITING FOR OTHER PLAYER";
            float height = 0.1f;
            float width = text_width(message, height);
            draw_text(message, glm::vec2(-0.5f * width,-0.5f), height, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

            glUseProgram(0);
	    }
    
     
        GL_ERRORS();
    }
}

void SolarSystemSlices::SolarSystemSlicesMode::show_order_menu() {
    // Make shared pointer to the current game
    std::shared_ptr< Mode > game = shared_from_this();
    std::shared_ptr< SolarSystemSlices::SolarSystemSlicesMode > game_mode;
    game_mode = std::static_pointer_cast<SolarSystemSlices::SolarSystemSlicesMode>(game);

    // Create the waiting menu
    std::shared_ptr< MenuMode > menu = std::make_shared< ShowOrderMenu >(game_mode, state.current_order);

    Mode::set_current(menu);
}

void SolarSystemSlices::SolarSystemSlicesMode::show_waiting_menu() {
    // Make shared pointer to the current game
    std::shared_ptr< Mode > game = shared_from_this();
    std::shared_ptr< SolarSystemSlices::SolarSystemSlicesMode > game_mode;
    game_mode = std::static_pointer_cast<SolarSystemSlices::SolarSystemSlicesMode>(game);

    // Create the waiting menu
    std::shared_ptr< MenuMode > menu = std::make_shared< WaitingMenu >(game_mode);

    Mode::set_current(menu);
}

void SolarSystemSlices::SolarSystemSlicesMode::show_endscreen() {
    // Make shared pointer to the current game
    std::shared_ptr< Mode > game = shared_from_this();
    std::shared_ptr< SolarSystemSlices::SolarSystemSlicesMode > game_mode;
    game_mode = std::static_pointer_cast<SolarSystemSlices::SolarSystemSlicesMode>(game);

    // Create the waiting menu
    std::shared_ptr< MenuMode > menu = std::make_shared< EndingScreen >(game_mode, winner);

    Mode::set_current(menu);
}

void SolarSystemSlices::SolarSystemSlicesMode::show_planet_selection_menu() {
    std::shared_ptr< MenuMode > menu = std::make_shared< MenuMode >();

    std::shared_ptr< Mode > game = shared_from_this();
    menu->background = game;

    menu->choices.emplace_back("CHOOSE A PLANET");
    menu->choices.emplace_back("MERCURY", [game](){
        // Recast the game
        std::shared_ptr< SolarSystemSlices::SolarSystemSlicesMode > mode;
        mode = std::static_pointer_cast<SolarSystemSlices::SolarSystemSlicesMode>(game);

        // Set the player's planet
        mode->my_player->planet = "Mercury";
		Mode::set_current(game);
	});
    
    menu->choices.emplace_back("VENUS", [game](){
        // Recast the game
        std::shared_ptr< SolarSystemSlices::SolarSystemSlicesMode > mode;
        mode = std::static_pointer_cast<SolarSystemSlices::SolarSystemSlicesMode>(game);

        // Set the player's planet
        mode->my_player->planet = "Venus";
		Mode::set_current(mode);
	});
    menu->choices.emplace_back("EARTH", [game](){
        // Recast the game
        std::shared_ptr< SolarSystemSlices::SolarSystemSlicesMode > mode;
        mode = std::static_pointer_cast<SolarSystemSlices::SolarSystemSlicesMode>(game);

        // Set the player's planet
        mode->my_player->planet = "Earth";
		Mode::set_current(game);
	});
    menu->choices.emplace_back("MARS", [game](){
        // Recast the game
        std::shared_ptr< SolarSystemSlices::SolarSystemSlicesMode > mode;
        mode = std::static_pointer_cast<SolarSystemSlices::SolarSystemSlicesMode>(game);

        // Set the player's planet
        mode->my_player->planet = "Mars";
		Mode::set_current(game);
	});
    menu->choices.emplace_back("JUPITER", [game](){
        // Recast the game
        std::shared_ptr< SolarSystemSlices::SolarSystemSlicesMode > mode;
        mode = std::static_pointer_cast<SolarSystemSlices::SolarSystemSlicesMode>(game);

        // Set the player's planet
        mode->my_player->planet = "Jupiter";
		Mode::set_current(game);
	});
    menu->choices.emplace_back("SATURN", [game](){
        // Recast the game
        std::shared_ptr< SolarSystemSlices::SolarSystemSlicesMode > mode;
        mode = std::static_pointer_cast<SolarSystemSlices::SolarSystemSlicesMode>(game);

        // Set the player's planet
        mode->my_player->planet = "Saturn";
		Mode::set_current(game);
	});
    menu->choices.emplace_back("NEPTUNE", [game](){
        // Recast the game
        std::shared_ptr< SolarSystemSlices::SolarSystemSlicesMode > mode;
        mode = std::static_pointer_cast<SolarSystemSlices::SolarSystemSlicesMode>(game);

        // Set the player's planet
        mode->my_player->planet = "Neptune";
		Mode::set_current(game);
	});
    menu->choices.emplace_back("URANUS", [game](){
        // Recast the game
        std::shared_ptr< SolarSystemSlices::SolarSystemSlicesMode > mode;
        mode = std::static_pointer_cast<SolarSystemSlices::SolarSystemSlicesMode>(game);
        
        // Set the player's planet
        mode->my_player->planet = "Uranus";
		Mode::set_current(game);
	});
    menu->choices.emplace_back("PLUTO", [game](){
        // Recast the game
        std::shared_ptr< SolarSystemSlices::SolarSystemSlicesMode > mode;
        mode = std::static_pointer_cast<SolarSystemSlices::SolarSystemSlicesMode>(game);
        
        // Set the player's planet
        mode->my_player->planet = "Pluto";
		Mode::set_current(game);
	});

    menu->selected = 1;

	Mode::set_current(menu);

}