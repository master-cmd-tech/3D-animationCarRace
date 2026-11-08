#pragma once
#include "Renderer.hpp"
namespace apex
{
struct HudState
{
    bool started = false, paused = false, animation = true, help = false, visible = true;
    float fps = 0;
    std::string team = "TEAM RACE CAR";
};
class Hud
{
  public:
    explicit Hud(const std::filesystem::path &root);
    ~Hud();
    void draw(const Circuit &track, const RaceFrame &frame, const Race &race, double time,
              const CameraState &camera, const HudState &state, int width, int height);

  private:
    struct Point
    {
        vec2 p;
        vec4 color;
    };
    GLuint program = 0, vao = 0, vbo = 0;
    std::vector<Point> vertices;
    void rect(float x, float y, float w, float h, vec4 c);
    void line(vec2 a, vec2 b, float width, vec4 c);
    void circle(vec2 center, float radius, vec4 color);
    void text(float x, float y, const std::string &s, float size, vec4 color = vec4(1));
};
} // namespace apex
