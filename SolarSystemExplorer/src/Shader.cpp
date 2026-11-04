#include "Shader.h"
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>
namespace sse {
namespace {
GLuint compile(GLenum type, const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in)
        throw std::runtime_error("Cannot open shader: " + path.u8string());
    std::ostringstream content;
    content << in.rdbuf();
    const auto str = content.str();
    const char* src = str.c_str();
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint n = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &n);
        std::vector<char> log(static_cast<size_t>(n) + 1);
        glGetShaderInfoLog(shader, n, nullptr, log.data());
        glDeleteShader(shader);
        throw std::runtime_error("Shader compilation failed: " + path.u8string() + "\n" + log.data());
    }
    return shader;
}
} // namespace
Shader::Shader(const std::filesystem::path& vertex, const std::filesystem::path& fragment) {
    GLuint vs = 0, fs = 0;
    try {
        vs = compile(GL_VERTEX_SHADER, vertex);
        fs = compile(GL_FRAGMENT_SHADER, fragment);
        id_ = glCreateProgram();
        glAttachShader(id_, vs);
        glAttachShader(id_, fs);
        glLinkProgram(id_);
        GLint ok = 0;
        glGetProgramiv(id_, GL_LINK_STATUS, &ok);
        if (!ok) {
            GLint n = 0;
            glGetProgramiv(id_, GL_INFO_LOG_LENGTH, &n);
            std::vector<char> log(static_cast<size_t>(n) + 1);
            glGetProgramInfoLog(id_, n, nullptr, log.data());
            throw std::runtime_error("Shader link failed: " + fragment.u8string() + "\n" + log.data());
        }
        glDetachShader(id_, vs);
        glDetachShader(id_, fs);
        glDeleteShader(vs);
        glDeleteShader(fs);
    } catch (...) {
        if (vs)
            glDeleteShader(vs);
        if (fs)
            glDeleteShader(fs);
        if (id_)
            glDeleteProgram(id_);
        throw;
    }
}
Shader::~Shader() {
    if (id_)
        glDeleteProgram(id_);
}
GLint Shader::location(const std::string& name) {
    auto it = uniforms_.find(name);
    if (it != uniforms_.end())
        return it->second;
    const auto loc = glGetUniformLocation(id_, name.c_str());
    uniforms_.emplace(name, loc);
    return loc;
}
void Shader::set(const std::string& n, int v) {
    glUniform1i(location(n), v);
}
void Shader::set(const std::string& n, float v) {
    glUniform1f(location(n), v);
}
void Shader::set(const std::string& n, const glm::vec3& v) {
    glUniform3fv(location(n), 1, glm::value_ptr(v));
}
void Shader::set(const std::string& n, const glm::mat4& v) {
    glUniformMatrix4fv(location(n), 1, GL_FALSE, glm::value_ptr(v));
}
} // namespace sse
