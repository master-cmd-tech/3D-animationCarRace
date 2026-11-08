#pragma once
#include "Race.hpp"
namespace apex
{
struct CameraState
{
    int mode = 0, selected = 0;
    float orbitYaw = -.6f, orbitPitch = .3f, orbitDistance = 22;
    vec3 freePosition{-195, 9, -325};
    float freeYaw = .2f, freePitch = -.2f, freeSpeed = 30;
    float lightAzimuth = -.7f, lightElevation = 1.05f, lightRadius = 75;
    int shading = 1, material = 0;
    bool perspective = true, lowQuality = false, shadows = true, accents = true;
};
struct Camera
{
    vec3 eye, target;
    mat4 view, projection;
};
GLuint compileShader(GLenum type, const char *source, const std::string &name);
GLuint createProgram(const std::filesystem::path &folder, const std::string &vert,
                     const std::string &frag);
CpuMesh processMesh(const std::vector<vec3> &positions, const std::vector<Triangle> &triangles);
CpuMesh loadSMF(const std::filesystem::path &path);
std::size_t validateCarParts(const std::filesystem::path &folder);
struct ModelPart
{
    GpuMesh mesh;
    Material material;
    int transformGroup = 0;
};
struct CarModel
{
    std::vector<ModelPart> parts;
    std::size_t triangles = 0;
    void destroy();
};
class Renderer
{
  public:
    Renderer(const std::filesystem::path &root, const Circuit &track);
    ~Renderer();
    void draw(const RaceFrame &frame, double time, const CameraState &state, int width, int height);
    void saveScreenshot(const std::filesystem::path &file, int width, int height) const;
    std::size_t sceneryTriangles = 0, mediumTriangles = 0, lowTriangles = 0;
    std::string gpu;
    Camera camera(const RaceFrame &f, double t, const CameraState &s, int w, int h) const;

  private:
    const Circuit &track;
    GLuint program = 0;
    GpuMesh environment, staticShadowCasters, numbers[4], lightsRed, lightsGreen, particles;
    CarModel carMedium, carLow;
    mat4 view{1};
    void setModel(const mat4 &model, bool flat = false);
    void setMaterial(const Material &material, bool vertexColors);
    void drawCars(const RaceFrame &frame, const CameraState &state, const mat4 &projection,
                  bool shadow);
};
} // namespace apex
