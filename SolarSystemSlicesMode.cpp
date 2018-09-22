#include "SolarSystemSlicesMode.hpp"
#include "SolarSystemSlicesGame.hpp"

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

Load< MeshBuffer > sss_meshes(LoadTagDefault, [](){
	return new MeshBuffer(data_path("sss_assets.pnc"));
});

Load< GLuint > sss_meshes_for_vertex_color_program(LoadTagDefault, [](){
	return new GLuint(sss_meshes->make_vao_for_program(vertex_color_program->program));
});

//Scene::Transform *player_transform = nullptr;
//Scene::Camera *camera = nullptr;    // Camera transform should be a child of the player

const float SolarSystemSlices::Star::MAX_DISTANCE = 6.0f;
std::vector<SolarSystemSlices::Star> SolarSystemSlices::Star::generate_stars(uint32_t num_stars){
    std::vector<Star> stars;

    std::srand((uint32_t)std::time(NULL));

    for (uint32_t i = 0; i < num_stars; i++) {
        // Choose a random position between [-1.0, 1.0f]
        glm::vec2 pos;
        pos.x = (float)(std::rand() % 200) / 100.0f - 1.0f;
        pos.y = (float)(std::rand() % 200) / 100.0f - 1.0f;
        // Choose a random distance between [1, 6]
        float dist = std::min(MAX_DISTANCE, std::max(1.0f, (float)(std::rand() % 7)));
        //printf("Star @ (%f, %f, dist =  %f)\n", pos.x, pos.y, dist);
        stars.push_back(Star(pos, dist));
    }

    return stars;
}

SolarSystemSlices::SolarSystemSlicesMode::SolarSystemSlicesMode( Client &client_ ) : client( client_ ) {
    client.connection.send_raw("h", 1); // Default "Hello" msg from GameMode.cpp

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
        // Create the object for player 1;
        Scene::Transform *transform_1 = scene.new_transform();
        transform_1->position = glm::vec3(-1.0f, 0.0f, 0.0f);
        transform_1->scale = glm::vec3(0.5f);
        //transform_1->scale = glm::vec3(0.05f, 0.08f, 1.0f);
        transform_1->rotation *= glm::angleAxis((float)M_PI / 2.0f, glm::vec3(0.0f, 1.0f, 0.0f));
        transform_1->rotation *= glm::angleAxis((float)M_PI, glm::vec3(1.0f, 0.0f, 0.0f));
        player_1 = attach_object(transform_1, "Player2");
        

        // Create Camera;
        Scene::Transform *camera_transform = scene.new_transform();
        camera_transform->rotation *= glm::angleAxis((float)M_PI, glm::vec3(0.0f, 0.0f, 1.0f));
        //camera_transform->set_parent(player_1->transform);
        camera_transform->position = glm::vec3(0.0f, 0.0f, 10.0f);
        
        player_camera = scene.new_camera(camera_transform);
    }

    { // Create and place the stars with quad meshes
        std::srand((uint32_t)std::time(NULL));
        Scene::Transform *star_trans = nullptr;
        
        uint32_t num_background_objects = 35;
       
        for (uint32_t i = 0; i < num_background_objects; i++) {
            // Create a new star
            star_trans = scene.new_transform();
            star_trans->position = player_1->transform->position;
            star_trans->position.x += ((float)(std::rand() % 100) / 100.0f) * 16.0f - 8.0f;
            star_trans->position.y += ((float)(std::rand() % 100) / 100.0f) * 16.0f - 8.0f;
            star_trans->rotation *= glm::angleAxis((float)M_PI / 2.0f, glm::vec3(0.0f, 0.0f, 1.0f));
            float dist = std::min(6.0f, std::max(1.0f, (float)(std::rand() % 7)));
            star_trans->position.z = -dist;
            star_trans->scale = glm::vec3(0.08f / dist, 0.1f / dist, 1.0f);
            Scene::Object * star_obj = attach_object(star_trans, "Star");
            background_objects.push_back(star_obj);
        }
    }

    


}

SolarSystemSlices::SolarSystemSlicesMode::~SolarSystemSlicesMode() {
    // Do Nothing
}

bool SolarSystemSlices::SolarSystemSlicesMode::handle_event( SDL_Event const &evt, glm::uvec2 const &window_size ) {
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

void SolarSystemSlices::SolarSystemSlicesMode::update( float elapsed ) {
    
    //glm::vec2 player_vel(0.1f, 0.0f);
    glm::vec3 player_vel(10.0f, 0.0f, 0.0f);
    float player_rot = 0.0f;
    // Adjust position of ship on the screen horizontally
    if (controls.forward) {
        player_vel += glm::vec3(15.0f, 0.0f, 0.0f);
        //player_1->transform->position += glm::vec3(-0.1f, 0.0f, 0.0f);
    }
    if (controls.backward) {
        player_vel += glm::vec3(-5.0f, 0.0f, 0.0f);
        //player_1->transform->position += glm::vec3(0.1f, 0.0f, 0.0f);
    }
    if (controls.up) {
        player_rot -= (float)M_PI / 4.0f;
        player_vel += glm::vec3(0.0f, 15.0f, 0.0f);
        //player_1->transform->position += glm::vec3(0.0f, -0.1f, 0.0f);
    }
    if (controls.down) {
        player_rot += (float)M_PI / 4.0f;
        player_vel += glm::vec3(0.0f, -15.0f, 0.0f);
        //player_1->transform->position += glm::vec3(0.0f, 0.1f, 0.0f);
    }
    if (controls.boost) {
        player_vel *= 2;
    }

     player_1->transform->rotation = glm::angleAxis((float)M_PI / 2.0f, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::angleAxis(player_rot, glm::vec3(1.0f, 0.0f, 0.0f));
        //glm::angleAxis((float)M_PI, glm::vec3(1.0f, 0.0f, 0.0f)) *
        



    for (uint32_t i = 0; i < background_objects.size(); i++) {
       background_objects[i]->transform->position -= (player_vel * elapsed) / std::abs(background_objects[i]->transform->position.z);

        if (std::abs(player_1->transform->position.x - background_objects[i]->transform->position.x) > 9) {
            
            background_objects[i]->transform->position.x = 7.0f;
            float dist = std::min(6.0f, std::max(1.0f, (float)(std::rand() % 7)));
            background_objects[i]->transform->position.z = -dist;
            background_objects[i]->transform->scale = glm::vec3(0.08f / dist, 0.1f / dist, 1.0f);
            background_objects[i]->transform->position.y = ((float)(std::rand() % 100) / 100.0f) * 16.0f - 8.0f;
            //background_objects[i]->transform->position.y *= -1.0f;
            
        }
    }

    for (uint32_t i = 0; i < stars.size(); i++) {
        stars[i].update(elapsed, player_vel);
    }
}

void SolarSystemSlices::SolarSystemSlicesMode::draw( glm::uvec2 const &drawable_size ) {
    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f); // Space is black
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glEnable( GL_DEPTH_TEST );
    glEnable( GL_BLEND );
    glBlendEquation( GL_FUNC_ADD );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    
    /*
    // Attempt to draw stars
    {
        // Vertex array object for the stars
        GLuint star_vao;

        glGenVertexArrays( 1, &star_vao );
        glBindVertexArray( star_vao );

       
        std::vector<float> positions;

        for (uint32_t i = 0; i < stars.size(); i++) {
            positions.push_back(stars[i].position.x);
            positions.push_back(stars[i].position.y);
            //printf("%f\n", stars[i].distance);
        }

        float* star_positions = &positions[0];
        // Create vertex buffer object for star vertices
        GLuint star_vbo;
        glGenBuffers (1, &star_vbo );

        // Allocate space on the GPU
        glBindBuffer( GL_ARRAY_BUFFER, star_vbo );
        glBufferData( GL_ARRAY_BUFFER, 4 * 50, star_positions, GL_STATIC_DRAW );

        // Created a separate program for the stars
        GLuint star_program = compile_program( star_vert_shader, star_frag_shader );

        GLint position_attr = glGetAttribLocation( star_program, "position" );

        glVertexAttribPointer( position_attr, 2, GL_FLOAT, GL_FALSE, 0, 0 );

        glEnableVertexAttribArray( position_attr );

        glEnable( GL_PROGRAM_POINT_SIZE );
        glBindVertexArray( star_vao );
        glDrawArrays( GL_POINTS, 0, 25 );
        glPointSize( 3.0f );

        glDisable( GL_PROGRAM_POINT_SIZE);
    }
    */
    
    { // Draw the scene
        glUseProgram(vertex_color_program->program);

        glUniform3fv(vertex_color_program->sun_color_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 1.0f)));
        glUniform3fv(vertex_color_program->sun_direction_vec3, 1, glm::value_ptr(glm::normalize(glm::vec3(-0.2f, 0.2f, 1.0f))));
        glUniform3fv(vertex_color_program->sky_color_vec3, 1, glm::value_ptr(glm::vec3(0.2f, 0.2f, 0.3f)));
        glUniform3fv(vertex_color_program->sky_direction_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 1.0f, 0.0f)));

        scene.draw(player_camera);
    }
    
     
    GL_ERRORS();
}