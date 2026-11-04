#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <filesystem>
#include <string>
#include <unordered_map>
namespace sse {
class Shader {
  public:
    Shader(const std::filesystem::path& vertex, const std::filesystem::path& fragment);
    ~Shader();
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    void use() const { glUseProgram(id_); }
    void set(const std::string& name, int value);
    void set(const std::string& name, float value);
    void set(const std::string& name, const glm::vec3& value);
    void set(const std::string& name, const glm::mat4& value);

  private:
    GLint location(const std::string& name);
    GLuint id_ = 0;
    std::unordered_map<std::string, GLint> uniforms_;
};
} // namespace sse
