#pragma once
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace apex
{
constexpr float PI = 3.14159265358979323846f;
constexpr double FILM_SECONDS = 120.0;
using glm::mat4;
using glm::vec2;
using glm::vec3;
using glm::vec4;
inline float clamp(float x, float a, float b)
{
    return std::max(a, std::min(b, x));
}
inline float smooth(float x)
{
    x = clamp(x, 0, 1);
    return x * x * (3 - 2 * x);
}
inline float smoother(float x)
{
    x = clamp(x, 0, 1);
    return x * x * x * (x * (x * 6 - 15) + 10);
}
inline float pulse(float t, float a, float b)
{
    if (t <= a || t >= b)
    {
        return 0;
    }
    return std::sin(PI * (t - a) / (b - a));
}
inline vec3 normalized(vec3 v)
{
    float n = glm::length(v);
    return n > 1e-7f ? v / n : vec3(0, 0, -1);
}
inline std::string readText(const std::filesystem::path &p)
{
    std::ifstream f(p);
    if (!f)
    {
        throw std::runtime_error("Cannot open " + p.string());
    }
    return {std::istreambuf_iterator<char>(f), {}};
}

// The course sample stores a position and normal. A color is added for scenery.
struct Vertex
{
    vec3 p, n, color;
};
struct Triangle
{
    int a, b, c;
};
struct Material
{
    vec3 ambient{.28f};
    vec3 diffuse{1.f};
    vec3 specular{.6f};
    float shininess = 80.f;
    bool painted = false;
};
struct CpuMesh
{
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
};
struct GpuMesh
{
    GLuint vao = 0, vbo = 0, ebo = 0;
    GLsizei count = 0;
    void upload(const CpuMesh &m)
    {
        if (!vao)
        {
            glGenVertexArrays(1, &vao);
            glGenBuffers(1, &vbo);
            glGenBuffers(1, &ebo);
        }
        count = static_cast<GLsizei>(m.indices.size());
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(
            GL_ARRAY_BUFFER, m.vertices.size() * sizeof(Vertex), m.vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER, m.indices.size() * 4, m.indices.data(), GL_STATIC_DRAW);
        for (int i = 0; i < 3; ++i)
        {
            glEnableVertexAttribArray(i);
            glVertexAttribPointer(i,
                                  3,
                                  GL_FLOAT,
                                  GL_FALSE,
                                  sizeof(Vertex),
                                  reinterpret_cast<void *>(std::uintptr_t(i * 12)));
        }
        glBindVertexArray(0);
    }
    void draw() const
    {
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
    }
    void destroy()
    {
        if (vao)
        {
            glDeleteVertexArrays(1, &vao);
        }
        if (vbo)
        {
            glDeleteBuffers(1, &vbo);
        }
        if (ebo)
        {
            glDeleteBuffers(1, &ebo);
        }
        vao = vbo = ebo = 0;
    }
};
inline void triangle(CpuMesh &m, vec3 a, vec3 b, vec3 c, vec3 color)
{
    vec3 n = normalized(glm::cross(b - a, c - a));
    auto base = std::uint32_t(m.vertices.size());
    for (auto p : {a, b, c})
    {
        m.vertices.push_back({p, n, color});
    }
    m.indices.insert(m.indices.end(), {base, base + 1, base + 2});
}
inline void quad(CpuMesh &m, vec3 a, vec3 b, vec3 c, vec3 d, vec3 color)
{
    triangle(m, a, b, c, color);
    triangle(m, a, c, d, color);
}
inline void box(CpuMesh &m, vec3 center, vec3 size, vec3 color)
{
    vec3 r = size * .5f;
    vec3 p[8];
    for (int i = 0; i < 8; ++i)
    {
        p[i] = center + vec3((i & 1) ? r.x : -r.x, (i & 2) ? r.y : -r.y, (i & 4) ? r.z : -r.z);
    }
    const int f[6][4] = {
        {0, 4, 6, 2}, {1, 3, 7, 5}, {0, 1, 5, 4}, {2, 6, 7, 3}, {0, 2, 3, 1}, {4, 5, 7, 6}};
    for (auto &q : f)
    {
        quad(m, p[q[0]], p[q[1]], p[q[2]], p[q[3]], color);
    }
}
inline void cone(CpuMesh &m, vec3 c, float r, float h, vec3 color, int sides = 9)
{
    for (int i = 0; i < sides; ++i)
    {
        float a = i * 2 * PI / sides, b = (i + 1) * 2 * PI / sides;
        vec3 p = c + vec3(r * std::cos(a), 0, r * std::sin(a)),
             q = c + vec3(r * std::cos(b), 0, r * std::sin(b));
        triangle(m, p, c + vec3(0, h, 0), q, color);
        triangle(m, c, p, q, color * .8f);
    }
}
inline void set3(GLuint p, const char *n, vec3 v)
{
    glUniform3fv(glGetUniformLocation(p, n), 1, glm::value_ptr(v));
}
inline void setM(GLuint p, const char *n, const mat4 &v)
{
    glUniformMatrix4fv(glGetUniformLocation(p, n), 1, GL_FALSE, glm::value_ptr(v));
}
inline void setF(GLuint p, const char *n, float v)
{
    glUniform1f(glGetUniformLocation(p, n), v);
}
inline void setI(GLuint p, const char *n, int v)
{
    glUniform1i(glGetUniformLocation(p, n), v);
}
} // namespace apex
