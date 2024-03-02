//    Author: Nabira Ahmad
//    Assignment: Pong Clone
//    Date due: 2024-03-02, 11:59pm
//    I pledge that I have completed this assignment without
//    collaborating with anyone else, in conformance with the
//    NYU School of Engineering Policies and Procedures on
//    Academic Misconduct.

#define GL_GLEXT_PROTOTYPES 1
#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"

#include <ctime>
#include "cmath"


const int WINDOW_WIDTH  = 640*1.7,
          WINDOW_HEIGHT = 480*1.7;

const float BG_RED     = 0.1922f,
            BG_BLUE    = 0.549f,
            BG_GREEN   = 0.9059f,
            BG_OPACITY = 1.0f;

const int VIEWPORT_X      = 0,
          VIEWPORT_Y      = 0,
          VIEWPORT_WIDTH  = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const char  V_SHADER_PATH[]          = "shaders/vertex_textured.glsl",
            F_SHADER_PATH[]          = "shaders/fragment_textured.glsl",
            PLAYER_SPRITE_FILEPATH[] = "dog1.png",
            PLAYER_SPRITE2_FILEPATH[] = "player1.png",
            YARN_TEXTURE_FILEPATH[] = "yarn.png",
            FONT_TEXTURE_FILEPATH[] = "font1.png";

const float MILLISECONDS_IN_SECOND     = 1000.0;
const float MINIMUM_COLLISION_DISTANCE = 1.0f;

const int   NUMBER_OF_TEXTURES = 1;
const GLint LEVEL_OF_DETAIL    = 0;
const GLint TEXTURE_BORDER     = 0;

SDL_Window* g_display_window;
bool  g_game_is_running = true;
float g_previous_ticks  = 0.0f;

// constants for window bounding
const float maxY = 3.3f;
const float minY = -3.3f;

ShaderProgram g_shader_program;
glm::mat4     g_view_matrix,
              g_model_matrix,
              g_projection_matrix,
              g_other_model_matrix,
              g_yarn_matrix;

GLuint g_player_texture_id,
       g_other_texture_id,
       g_yarn_texture_id,
       font_texture_id;

// positions, movements, and velocities
glm::vec3 g_player1_position = glm::vec3(-4.0f, 0.0f, 0.0f);
glm::vec3 g_player1_movement = glm::vec3(0.0f, 0.0f, 0.0f);

glm::vec3 g_player2_position  = glm::vec3(4.0f, 0.0f, 0.0f);
glm::vec3 g_player2_movement  = glm::vec3(0.0f, 0.0f, 0.0f);

glm::vec3 g_yarn_position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_yarn_velocity = glm::vec3(1.5f, 1.5f, 0.0f);

// measurements for collision detection
float yarn_width = 0.5f, yarn_height = 0.5f,
      g_player_width = 0.5f, g_other_width = 1.0f,
      g_player_height = 1.0f, g_other_height = 1.5f;

float g_player_speed = 2.0f;

// to check if t is toggled or not
bool two_players = true;
// from animation + text lecture
const int FONTBANK_SIZE = 16;

GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);
    
    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }
    
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    stbi_image_free(image);
    
    return textureID;
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Hello, Collisions!",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);
    
    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);
    
#ifdef _WINDOWS
    glewInit();
#endif
    
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    
    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);
    
    g_model_matrix       = glm::mat4(1.0f);
    
    g_other_model_matrix = glm::mat4(1.0f);
    
    g_yarn_matrix = glm::mat4(1.0f);
    
    g_view_matrix       = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
    
    g_player_texture_id = load_texture(PLAYER_SPRITE_FILEPATH);
    g_other_texture_id  = load_texture(PLAYER_SPRITE2_FILEPATH);
    g_yarn_texture_id = load_texture(YARN_TEXTURE_FILEPATH);
    font_texture_id = load_texture(FONT_TEXTURE_FILEPATH);
    
    glUseProgram(g_shader_program.get_program_id());

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);
    
    glUseProgram(g_shader_program.get_program_id());
    
    // changed the color :)
    glClearColor(0.8f, 0.95f, 0.8f, 1.0f);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


void process_input()
{
    // if no button is pressed we don't want movement
    // added an if statement so that one paddle can still move when game is on one player mode
    g_player1_movement = glm::vec3(0.0f);
    if (two_players){
        g_player2_movement = glm::vec3(0.0f);
    }
    
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                g_game_is_running = false;
                break;
                
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym)
                {
                        
                    case SDLK_t:
                        // switch between two player to one player
                        two_players = !two_players;
                        break;
                        
                    default:
                        break;
                }
            default:
                break;
        }
    }
    
    const Uint8 *key_state = SDL_GetKeyboardState(NULL);
    
    // key states for player's movement
    if (key_state[SDL_SCANCODE_UP]) {
        g_player1_movement.y = 1.0f;
    } else if (key_state[SDL_SCANCODE_DOWN]) {
        g_player1_movement.y = -1.0f;
    }
    
    // second sprite movement controls
    if (two_players) {
        if (key_state[SDL_SCANCODE_W]) {
            g_player2_movement.y = 1.0f;
        } else if (key_state[SDL_SCANCODE_S]) {
            g_player2_movement.y = -1.0f;
        }
        // normalizing so that it isn't faster
        if (glm::length(g_player2_movement) > 1.0f)
        {
            g_player2_movement = glm::normalize(g_player2_movement);
        }
    }

    if (glm::length(g_player1_movement) > 1.0f)
        {
            g_player1_movement = glm::normalize(g_player1_movement);
        }
}

// check collision function: distance formula with widths and heights of each element
bool check_collision(const glm::vec3& position_a, const glm::vec3& position_b, float width_a, float height_a, float width_b, float height_b) {
    return (position_a.x - width_a / 2.0f < position_b.x + width_b / 2.0f) &&
           (position_a.x + width_a / 2.0f > position_b.x - width_b / 2.0f) &&
           (position_a.y - height_a / 2.0f < position_b.y + height_b / 2.0f) &&
           (position_a.y + height_a / 2.0f > position_b.y - height_b / 2.0f);
}

// from animation + text lecture material
void DrawText(ShaderProgram *program, GLuint font_texture_id, std::string text, float screen_size, float spacing, glm::vec3 position)
{
    // Scale the size of the fontbank in the UV-plane
    // We will use this for spacing and positioning
    float width = 1.0f / FONTBANK_SIZE;
    float height = 1.0f / FONTBANK_SIZE;

    // Instead of having a single pair of arrays, we'll have a series of pairs—one for each character
    // Don't forget to include <vector>!
    std::vector<float> vertices;
    std::vector<float> texture_coordinates;

    // For every character...
    for (int i = 0; i < text.size(); i++) {
        // 1. Get their index in the spritesheet, as well as their offset (i.e. their position
        //    relative to the whole sentence)
        int spritesheet_index = (int) text[i];  // ascii value of character
        float offset = (screen_size + spacing) * i;
        
        // 2. Using the spritesheet index, we can calculate our U- and V-coordinates
        float u_coordinate = (float) (spritesheet_index % FONTBANK_SIZE) / FONTBANK_SIZE;
        float v_coordinate = (float) (spritesheet_index / FONTBANK_SIZE) / FONTBANK_SIZE;

        // 3. Inset the current pair in both vectors
        vertices.insert(vertices.end(), {
            offset + (-0.5f * screen_size), 0.5f * screen_size,
            offset + (-0.5f * screen_size), -0.5f * screen_size,
            offset + (0.5f * screen_size), 0.5f * screen_size,
            offset + (0.5f * screen_size), -0.5f * screen_size,
            offset + (0.5f * screen_size), 0.5f * screen_size,
            offset + (-0.5f * screen_size), -0.5f * screen_size,
        });

        texture_coordinates.insert(texture_coordinates.end(), {
            u_coordinate, v_coordinate,
            u_coordinate, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate + width, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate, v_coordinate + height,
        });
    }

    // 4. And render all of them using the pairs
    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(model_matrix, position);
    
    program->set_model_matrix(model_matrix);
    glUseProgram(program->get_program_id());
    
    glVertexAttribPointer(program->get_position_attribute(), 2, GL_FLOAT, false, 0, vertices.data());
    glEnableVertexAttribArray(program->get_position_attribute());
    glVertexAttribPointer(program->get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates.data());
    glEnableVertexAttribArray(program->get_tex_coordinate_attribute());
    
    glBindTexture(GL_TEXTURE_2D, font_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, (int) (text.size() * 6));
    
    glDisableVertexAttribArray(program->get_position_attribute());
    glDisableVertexAttribArray(program->get_tex_coordinate_attribute());
}

void update()
{
    // delta time
    float ticks = (float) SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;
    
    // player 1 transitions
    g_model_matrix = glm::mat4(1.0f);
    // position = movement * speed * delta time
    g_player1_position += g_player1_movement * g_player_speed * delta_time;
    
    // bounding the player to the screen
    if (g_player1_position.y < minY) {
        g_player1_position.y = minY;
    } else if (g_player1_position.y > maxY) {
        g_player1_position.y = maxY;
    }
    
    g_model_matrix = glm::translate(g_model_matrix, g_player1_position);
    
    // player 2 transitions
    if (two_players) {
        g_other_model_matrix = glm::mat4(1.0f);
        g_player2_position += g_player2_movement * g_player_speed * delta_time;
        
        if (g_player2_position.y < minY) {
            g_player2_position.y = minY   ;
        } else if (g_player2_position.y > maxY) {
            g_player2_position.y = maxY;
        }
        
        g_other_model_matrix = glm::translate(g_other_model_matrix, g_player2_position);
    }
    else{
        // if we are in one player mode, simple up and down movement
        g_other_model_matrix = glm::mat4(1.0f);
        
        // since it automatically is set to 0
        if (g_player2_movement.y == 0.0f){
            g_player2_movement.y = 1.0f;
        }
        if (g_player2_position.y >= maxY) {
            g_player2_movement.y = -1.0f;
        } else if (g_player2_position.y <= minY) {
            g_player2_movement.y = 1.0f;
        }
        // update position after movement is updated
        g_player2_position += g_player2_movement * g_player_speed * delta_time;
        
        g_other_model_matrix = glm::translate(g_other_model_matrix, g_player2_position);
    }

    // for a smoother ball movement use fixed time step
    float fixed_time_step = 0.016f;
    g_yarn_matrix = glm::mat4(1.0f);
    g_yarn_position += g_yarn_velocity * fixed_time_step;
        
    // negate the velocity when it reaches top and bottom walls so that it can bounce
    if (g_yarn_position.y + yarn_height / 2 >= maxY || g_yarn_position.y - yarn_height / 2 <= minY) {
        g_yarn_velocity.y = -1.01 * g_yarn_velocity.y;
    }
    
    // check for collision between ball and players, negate horizontal velocity when collision occurs
    if (check_collision(g_player1_position, g_yarn_position, g_player_width, g_player_height, yarn_width, yarn_height) ||
        check_collision(g_player2_position, g_yarn_position, g_other_width, g_other_height, yarn_width, yarn_height)) {
        g_yarn_velocity.x = -g_yarn_velocity.x;
    }
    g_yarn_matrix = glm::translate(glm::mat4(1.0f), g_yarn_position);
}


void draw_object(glm::mat4 &object_model_matrix, GLuint &object_texture_id)
{
    g_shader_program.set_model_matrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);
    
    float vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f
    };

    float texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    };
    
    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());
    
    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
    
    draw_object(g_model_matrix, g_player_texture_id);
    draw_object(g_other_model_matrix, g_other_texture_id);
    draw_object(g_yarn_matrix, g_yarn_texture_id);
    
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
    
    // for an endgame UI message (for fun), when the yarn goes past the dog's side the cat wins
    if (g_yarn_position.x + yarn_width / 2 < -5.0f){
           DrawText(&g_shader_program, font_texture_id, "CAT WINS!", 0.5f, 0.1f, glm::vec3(-2.0f, 0.0f, 0.0f));
       }
    // when the yarn goes past the cat's side the dog wins
    if (g_yarn_position.x - yarn_width / 2 > 5.0f){
        DrawText(&g_shader_program, font_texture_id, "DOG WINS!", 0.5f, 0.1f, glm::vec3(-2.0f, 0.0f, 0.0f));
    }
    SDL_GL_SwapWindow(g_display_window);
}


void shutdown() { SDL_Quit(); }


int main(int argc, char* argv[])
{
    initialise();
    
    while (g_game_is_running)
    {
        process_input();
        update();
        render();
    }
    
    shutdown();
    return 0;
}

