/**
* Author:  Jack Ma
* Assignment: Simple 2D Scene
* Date due: 2023-06-11, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/


#define GL_SILENCE_DEPRECATION
#define GL_GLEXT_PROTOTYPES 1
#define LOG(argument) std::cout << argument << '\n'
#define STB_IMAGE_IMPLEMENTATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"                // 4x4 Matrix
#include "glm/gtc/matrix_transform.hpp"  // Matrix transformation methods
#include "ShaderProgram.h"               // We'll talk about these later in the course
#include "stb_image.h"

enum Coordinate
{
    x_coordinate,
    y_coordinate
};


// Our window dimensions
const int WINDOW_WIDTH  = 640,
          WINDOW_HEIGHT = 480;

// Heartbeat stuff
const float GROWTH_FACTOR = 1.01f;  // grow by 1.0% / frame
const float SHRINK_FACTOR = 0.99f;  // grow by -1.0% / frame
const int MAX_FRAMES = 40;

// Rotation stuff
const float ROT_ANGLE = glm::radians(1.0f);

// Translation stuff
const float TRAN_VALUE = 0.025f;

int g_frame_counter = 0;
bool g_is_growing = true;

// Background color components
const float BG_RED     = 0.1922f,
            BG_BLUE    = 0.549f,
            BG_GREEN   = 0.9059f,
            BG_OPACITY = 1.0f;

// Our viewport—or our "camera"'s—position and dimensions
const int VIEWPORT_X      = 0,
          VIEWPORT_Y      = 0,
          VIEWPORT_WIDTH  = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

// Our shader filepaths; these are necessary for a number of things
// Not least, to actually draw our shapes
// We'll have a whole lecture on these later
const char V_SHADER_PATH[] = "shaders/vertex.glsl",
           F_SHADER_PATH[] = "shaders/fragment.glsl";

const float MILLISECONDS_IN_SECOND = 1000.0;
const float DEGREES_PER_SECOND = 90.0f;

const int NUMBER_OF_TEXTURES = 1; // to be generated, that is
const GLint LEVEL_OF_DETAIL = 0;  // base image level; Level n is the nth mipmap reduction image
const GLint TEXTURE_BORDER = 0;   // this value MUST be zero

const char PLAYER_SPRITE_FILEPATH[] = "assets/steve.png";
const char PLAYER_SPRITE_FILEPATH2[] = "assets/creeper.png";

// Our object's fill colour
const float TRIANGLE_RED     = 1.0,
            TRIANGLE_BLUE    = 0.4,
            TRIANGLE_GREEN   = 0.4,
            TRIANGLE_OPACITY = 1.0;

bool g_game_is_running = true;
SDL_Window* g_display_window;

ShaderProgram g_shader_program;
glm::mat4 view_matrix, m_model_matrix, m_projection_matrix, m_trans_matrix;
float m_previous_ticks = 0.0f;

GLuint g_player_texture_id;
SDL_Joystick *g_player_one_controller;

// overall position
glm::vec3 g_player_position = glm::vec3(0.0f, 0.0f, 0.0f);

// movement tracker
glm::vec3 g_player_movement = glm::vec3(0.0f, 0.0f, 0.0f);

float get_screen_to_ortho(float coordinate, Coordinate axis)
{
    switch (axis) {
        case x_coordinate:
            return ((coordinate / WINDOW_WIDTH) * 10.0f ) - (10.0f / 2.0f);
        case y_coordinate:
            return (((WINDOW_HEIGHT - coordinate) / WINDOW_HEIGHT) * 7.5f) - (7.5f / 2.0);
        default:
            return 0.0f;
    }
}

glm::mat4 g_view_matrix,        // Defines the position (location and orientation) of the camera
          g_model_matrix, g_model_matrix2,      // Defines every translation, rotation, and/or scaling applied to an object; we'll look at these next week
          g_projection_matrix;  // Defines the characteristics of your camera, such as clip panes, field of view, projection method, etc.

const float INIT_TRIANGLE_ANGLE = glm::radians(45.0);

GLuint load_texture(const char* filepath)
{
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);
    
    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        LOG(filepath);
        assert(false);
    }
    
    // STEP 2: Generating and binding a texture ID to our image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);
    
    // STEP 3: Setting our texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    // STEP 4: Releasing our file from memory and returning our texture id
    stbi_image_free(image);
    
    return textureID;
}


void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Hello, Triangle!",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);
    
    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);
    
    g_model_matrix = glm::mat4(1.0f);
    g_model_matrix2 = glm::mat4(1.0f);
    g_model_matrix = glm::rotate(g_model_matrix, INIT_TRIANGLE_ANGLE, glm::vec3(1.0f, 0.0f, 0.0f));
    g_model_matrix2 = glm::rotate(g_model_matrix2, INIT_TRIANGLE_ANGLE, glm::vec3(0.0f, 1.0f, 1.0f));
    
    
    
    
#ifdef _WINDOWS
    glewInit();
#endif
    
    // Initialise our camera
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    
    // Load up our shaders
    g_shader_program.Load(V_SHADER_PATH, F_SHADER_PATH);
    
    // Initialise our view, model, and projection matrices
    g_view_matrix       = glm::mat4(1.0f);  // Defines the position (location and orientation) of the camera
    g_model_matrix      = glm::mat4(1.0f);  // Defines every translation, rotations, or scaling applied to an object
    g_model_matrix2 = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);  // Defines the characteristics of your camera, such as clip planes, field of view, projection method etc.
    
//    g_model_matrix = glm::translate(g_model_matrix, glm::vec3(5.0f, 0.0f, 0.0f));
    
    g_shader_program.SetProjectionMatrix(g_projection_matrix);
    g_shader_program.SetViewMatrix(g_view_matrix);
    // Notice we haven't set our model matrix yet!
    
    g_shader_program.SetColor(TRIANGLE_RED, TRIANGLE_BLUE, TRIANGLE_GREEN, TRIANGLE_OPACITY);
    
    // Each object has its own unique ID
    glUseProgram(g_shader_program.programID);
    
    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    
    // enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    g_player_movement = glm::vec3(0.0f);
    
    SDL_Event event;
    
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
            case SDL_WINDOWEVENT_CLOSE:
            case SDL_QUIT:
                g_game_is_running = false;
                break;
                
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_RIGHT:
                        g_player_movement.x = 1.0f;
                        break;
                    case SDLK_LEFT:
                        g_player_movement.x = -1.0f;
                        break;
                    case SDLK_q:
                        g_game_is_running = false;
                        break;
                    default:
                        break;
                }
            default:
                break;
        }
    }
    
    const Uint8 *key_states = SDL_GetKeyboardState(NULL); // array of key states [0, 0, 1, 0, 0, ...]
    
    if (key_states[SDL_SCANCODE_LEFT])
    {
        g_player_movement.x = -1.0f;
    } else if (key_states[SDL_SCANCODE_RIGHT])
    {
        g_player_movement.x = 1.0f;
    }
    
    if (key_states[SDL_SCANCODE_UP])
    {
        g_player_movement.y = 1.0f;
    } else if (key_states[SDL_SCANCODE_DOWN])
    {
        g_player_movement.y = -1.0f;
    }
    
    if (glm::length(g_player_movement) > 1.0f)
    {
        g_player_movement = glm::normalize(g_player_movement);
    }
}



void update()
{
    float ticks = (float) SDL_GetTicks() / MILLISECONDS_IN_SECOND; // get the current number of ticks
    float delta_time = ticks - m_previous_ticks; // the delta time is the difference from the last frame
    m_previous_ticks = ticks;

    // Add             direction       * elapsed time * units per second
    g_player_position += g_player_movement * delta_time * 1.0f;

    m_model_matrix = glm::mat4(1.0f);
    m_model_matrix = glm::translate(m_model_matrix, g_player_position);
    
    
    glm::vec3 scale_vector;
    g_frame_counter += 1;
    
    // Every frame, rotate my model by an angle of ROT_ANGLE on the z-axis
    g_model_matrix = glm::rotate(g_model_matrix, ROT_ANGLE, glm::vec3(0.0f, 0.0f, 1.0f));
    g_model_matrix2 = glm::rotate(g_model_matrix2, ROT_ANGLE, glm::vec3(0.0f, 0.0f, 1.0f));
    
    if (g_frame_counter >= MAX_FRAMES)
    {
        g_is_growing = !g_is_growing;
        g_frame_counter = 0;
    }
    
    scale_vector = glm::vec3(g_is_growing ? GROWTH_FACTOR : SHRINK_FACTOR,
                             g_is_growing ? GROWTH_FACTOR : SHRINK_FACTOR,
                             1.0f);
    
    // translates the shape
    g_model_matrix = glm::translate(g_model_matrix, glm::vec3(TRAN_VALUE, 0.0f, 0.0f));
    
    g_model_matrix2 = glm::translate(g_model_matrix2, glm::vec3(TRAN_VALUE, 0.0f, 0.0f));

    // rotates the shape
    g_model_matrix = glm::rotate(g_model_matrix, ROT_ANGLE, glm::vec3(0.0f, 1.0f, 0.0f));
    
    g_model_matrix2 = glm::rotate(g_model_matrix2, ROT_ANGLE, glm::vec3(0.0f, 1.0f, 0.0f));
    
    // scales the shape
    g_model_matrix = glm::scale(g_model_matrix, scale_vector);
    g_model_matrix2 = glm::scale(g_model_matrix2, scale_vector);
}

void draw_object(glm::mat4 &object_model_matrix, GLuint &object_texture_id)
{
    g_shader_program.SetModelMatrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so we use 6 instead of 3
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);
    
    g_shader_program.SetModelMatrix(g_model_matrix);
    
    // Vertices
     float vertices[] = {
         -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
         -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
     };

     // Textures
     float texture_coordinates[] = {
         0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
         0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
     };
    
    glVertexAttribPointer(g_shader_program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.positionAttribute);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    
    
    glDisableVertexAttribArray(g_shader_program.positionAttribute);
    
    draw_object(g_model_matrix, g_player_texture_id);
    
    draw_object(g_model_matrix2, g_player_texture_id);
    
    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() { SDL_Quit(); }

/**
 Start here—we can see the general structure of a game loop without worrying too much about the details yet.
 */
int main(int argc, char* argv[])
{
    // Initialise our program—whatever that means
    initialise();
    
    while (g_game_is_running)
    {
        process_input();  // If the player did anything—press a button, move the joystick—process it
        update();         // Using the game's previous state, and whatever new input we have, update the game's state
        render();         // Once updated, render those changes onto the screen
    }
    
    shutdown();  // The game is over, so let's perform any shutdown protocols
    return 0;
}
