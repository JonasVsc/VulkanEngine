#pragma once

#include <SDL2/SDL.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

class Camera
{

public:
    void init(uint32_t window_width, uint32_t window_height, glm::vec3 initial_position = glm::vec3(0.0f));

    void update(SDL_Window* window, float delta_time);

    void update_vectors();

    inline glm::mat4 get_view_matrix() { return glm::lookAt(position, position + front, up); };

    glm::mat4 projection;
    glm::vec3 position;
    glm::mat4 view;

    glm::vec3 direction;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;

    bool allowMovement = false;
    bool firstMouse = true;

    float yaw = -90.0f;
    float pitch = 0.0f;

    float lastX = 1280.0f / 2.0f;
    float lastY = 720.0f / 2.0f;
    float fov = 90.0f;
};