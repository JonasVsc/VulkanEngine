#pragma once

#include "jvsc_mesh.hpp"

// lib
#include <glm/gtc/type_ptr.hpp>

namespace jvsc {

    struct Transform {
        glm::vec3 translate;
        glm::vec3 scale;
        glm::vec3 rotate;
        glm::mat4 mat4() {
            glm::mat4 matrix{ 1.f };
            matrix = glm::translate(matrix, translate);
            matrix = glm::rotate(matrix, rotate.x, { 0.0f, 1.0f, 0.0f });
            matrix = glm::rotate(matrix, rotate.y, { 1.0f, 0.0f, 0.0f });
            matrix = glm::rotate(matrix, rotate.z, { 0.0f, 0.0f, 1.0f });
            matrix = glm::scale(matrix, scale);
            return matrix;
        }
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
        Transform transform{};

    private:
        JvscGameObject(id_t obj_id) : id{ obj_id } {}

        id_t id;
    };

}