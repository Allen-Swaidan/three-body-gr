#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

class Shader {
public:
    GLuint id = 0;

    Shader() = default;
    Shader(const std::string& vertPath, const std::string& fragPath);

    void use() const;
    void setMat4(const std::string& name, const glm::mat4& m) const;
    void setVec3(const std::string& name, const glm::vec3& v) const;
    void setVec4(const std::string& name, const glm::vec4& v) const;
    void setFloat(const std::string& name, float f) const;
    void setInt(const std::string& name, int i) const;

private:
    GLuint compile(const std::string& path, GLenum type);
};
