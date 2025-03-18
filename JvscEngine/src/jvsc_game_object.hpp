#pragma once

// lib
#include "jvsc_mesh.hpp"

namespace jvsc {

    struct Transform2D 
    {
        glm::vec2 translation{};  // (position offset)
        glm::vec2 scale{ 1.f, 1.f };
        float rotation;

        glm::mat2 mat2() {
            const float s = glm::sin(rotation);
            const float c = glm::cos(rotation);
            glm::mat2 rotMatrix{ {c, s}, {-s, c} };

            glm::mat2 scaleMat{ {scale.x, .0f}, {.0f, scale.y} };
            return rotMatrix * scaleMat;
        }
    };

    struct RigidBody2D
    {
        glm::vec2 velocity;
        float mass{ 1.0f };
    };

    class JvscGameObject
    {
    public:
        using id_t = unsigned int;

        static JvscGameObject create_game_object()
        {
            static id_t current_Id = 0;
            return JvscGameObject{ current_Id++ };
        }

        JvscGameObject(const JvscGameObject&) = delete;
        JvscGameObject& operator=(const JvscGameObject&) = delete;
        JvscGameObject(JvscGameObject&&) = default;
        JvscGameObject& operator=(JvscGameObject&&) = default;
        
        void destroy()
        {
            mesh->destroy();
        }

        id_t get_Id() { return id; }

        // components
        JvscMesh* mesh{};
        glm::vec3 color{};
        Transform2D transform{};
        RigidBody2D rigidbody2d{};

    private:
        JvscGameObject(id_t obj_id) : id{ obj_id } {}

        id_t id;
    };

}