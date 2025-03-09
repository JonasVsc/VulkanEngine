#include "pch.h"
#include "vk_camera.h"

void Camera::init(uint32_t window_width, uint32_t window_height, glm::vec3 initial_position)
{
	position = initial_position;
	projection = glm::perspective(glm::radians(fov), 16.0f / 9.0f, 0.1f, 100000.0f);
	projection[1][1] *= -1;

	front = { 0.0f, 0.0f, -1.0f };
	up = { 0.0f, 1.0f, 0.0f };

	lastX = window_width;
	lastY = window_height;
}

void Camera::update(SDL_Window* window, float delta_time)
{
	float camera_speed = 50.5f * delta_time;
	view = glm::translate(glm::mat4(1.0f), position);

	const Uint8* keys = SDL_GetKeyboardState(NULL);

	if (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT))
	{
		if (keys[SDL_SCANCODE_W])
			position += camera_speed * front;
		if (keys[SDL_SCANCODE_A])
			position -= glm::normalize(glm::cross(front, up)) * camera_speed;
		if (keys[SDL_SCANCODE_S])
			position -= camera_speed * front;
		if (keys[SDL_SCANCODE_D])
			position += glm::normalize(glm::cross(front, up)) * camera_speed;
		if (keys[SDL_SCANCODE_SPACE])
			position.y += camera_speed;
		if (keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL])
			position.y -= camera_speed;
	}
}

void Camera::update_vectors()
{
	glm::vec3 front;
	front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	this->front = glm::normalize(front);

	// Also re-calculate the Right and Up vector
	this->right = glm::normalize(glm::cross(this->front, glm::vec3(0.0f, 1.0f, 0.0f)));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
	this->up = glm::normalize(glm::cross(this->right, this->front));
}
