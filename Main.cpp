#include "src/Hud.hpp"
#include <chrono>
#include <iomanip>
#ifdef _WIN32
extern "C" __declspec(dllexport) unsigned long NvOptimusEnablement = 1;
extern "C" __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
#endif
using namespace apex;
namespace
{
    struct Options
    {
        bool validate = false, hidden = false, help = false, overlay = false;
        int benchmark = 0, width = 1280, height = 720;
        double time = -1;
        std::string screenshot;
        CameraState camera;
        bool hud = true;
    };
    Options options(int argc, char** argv)
    {
        Options o;
        for (int i = 1; i < argc; ++i)
        {
            std::string a = argv[i];
            auto next = [&]() {
                if (i + 1 >= argc)
                {
                    throw std::runtime_error("Missing argument after " + a);
                }
                return std::string(argv[++i]);
                };
            if (a == "--validate")
            {
                o.validate = true;
            }
            else if (a == "--screenshot")
            {
                o.screenshot = next();
                o.hidden = true;
            }
            else if (a == "--time")
            {
                o.time = std::stod(next());
            }
            else if (a == "--benchmark")
            {
                o.benchmark = std::stoi(next());
                o.hidden = true;
            }
            else if (a == "--width")
            {
                o.width = std::stoi(next());
            }
            else if (a == "--height")
            {
                o.height = std::stoi(next());
            }
            else if (a == "--low-quality")
            {
                o.camera.lowQuality = true;
            }
            else if (a == "--no-shadows")
            {
                o.camera.shadows = false;
            }
            else if (a == "--gouraud")
            {
                o.camera.shading = 0;
            }
            else if (a == "--no-hud")
            {
                o.hud = false;
            }
            else if (a == "--help-overlay")
            {
                o.overlay = true;
            }
            else if (a == "--no-accents")
            {
                o.camera.accents = false;
            }
            else if (a == "--parallel")
            {
                o.camera.perspective = false;
            }
            else if (a == "--camera")
            {
                o.camera.mode = std::stoi(next());
            }
            else if (a == "--car")
            {
                o.camera.selected = std::stoi(next());
            }
            else if (a == "--show")
            {
                o.hidden = false;
            }
            else if (a == "--help")
            {
                o.help = true;
            }
            else
            {
                throw std::runtime_error("Unknown argument: " + a);
            }
        }
        if (o.width < 320 || o.width > 7680 || o.height < 240 || o.height > 4320 || o.camera.mode < 0 ||
            o.camera.mode > 3 || o.camera.selected < 0 || o.camera.selected > 3 || o.benchmark < 0 ||
            o.benchmark > 100000)
        {
            throw std::runtime_error("Invalid command-line range");
        }
        return o;
    }
    std::filesystem::path assetRoot(const char* exe)
    {
        std::vector<std::filesystem::path> candidates = { std::filesystem::absolute(exe).parent_path(),
                                                         std::filesystem::current_path() };
#ifdef APEX_SOURCE_DIR
        candidates.emplace_back(APEX_SOURCE_DIR);
#endif
        for (auto p : candidates)
        {
            if (std::filesystem::exists(p / "Shaders/scene.vert") &&
                std::filesystem::exists(p / "Assets/Models/Parts/high/parts.txt"))
            {
                return p;
            }
        }
        throw std::runtime_error("Cannot find Assets and Shaders. Keep them next to RaceCar3D.exe");
    }
} // namespace

struct InputState
{
    std::array<bool, GLFW_KEY_LAST + 1> previous{};
    bool dragging = false;
    double oldX = 0, oldY = 0;
};

bool processInput(GLFWwindow* window, Renderer& renderer, const Race& race, CameraState& state,
    HudState& ui, InputState& input, double& timeline, float delta, int width,
    int height)
{
    auto& previous = input.previous;
    bool& dragging = input.dragging;
    double& oldX = input.oldX;
    double& oldY = input.oldY;
    auto pressed = [&](int key) {
        bool down = glfwGetKey(window, key) == GLFW_PRESS;
        bool edge = down && !previous[key];
        previous[key] = down;
        return edge;
        };
    auto down = [&](int key) { return glfwGetKey(window, key) == GLFW_PRESS; };
    if (pressed(GLFW_KEY_ESCAPE))
    {
        glfwSetWindowShouldClose(window, true);
    }
    if (pressed(GLFW_KEY_ENTER))
    {
        ui.started = true;
        ui.paused = false;
        if (timeline >= 120)
        {
            timeline = 0;
        }
    }
    if (pressed(GLFW_KEY_SPACE))
    {
        ui.started = true;
        ui.paused = !ui.paused;
    }
    if (pressed(GLFW_KEY_R))
    {
        timeline = 0;
        ui.started = true;
        ui.paused = false;
        ui.animation = true;
    }
    if (pressed(GLFW_KEY_T))
    {
        ui.animation = !ui.animation;
    }
    if (pressed(GLFW_KEY_B))
    {
        state.accents = !state.accents;
    }

    if (pressed(GLFW_KEY_F1) || pressed(GLFW_KEY_H))
    {
        ui.help = !ui.help;
    }
    if (pressed(GLFW_KEY_F10))
    {
        ui.visible = !ui.visible;
    }
    if (pressed(GLFW_KEY_F2))
    {
        state.lowQuality = !state.lowQuality;
    }
    if (pressed(GLFW_KEY_F3))
    {
        state.shadows = !state.shadows;
    }
    if (pressed(GLFW_KEY_1))
    {
        state.shading = 0;
    }
    if (pressed(GLFW_KEY_2))
    {
        state.shading = 1;
    }
    if (pressed(GLFW_KEY_3))
    {
        state.material = 0;
    }
    if (pressed(GLFW_KEY_4))
    {
        state.material = 1;
    }
    if (pressed(GLFW_KEY_5))
    {
        state.material = 2;
    }
    if (pressed(GLFW_KEY_P))
    {
        state.perspective = !state.perspective;
    }
    if (pressed(GLFW_KEY_V))
    {
        state.selected = (state.selected + 1) % 4;
    }
    bool cycleCamera = pressed(GLFW_KEY_C), freeCamera = pressed(GLFW_KEY_F);
    if (cycleCamera || freeCamera)
    {
        int mode = freeCamera ? 3 : (state.mode + 1) % 4;
        if (mode == 3)
        {
            auto old = renderer.camera(
                race.evaluate(timeline, state.accents), timeline, state, width, height);
            state.freePosition = old.eye;
            vec3 d = normalized(old.target - old.eye);
            state.freeYaw = std::atan2(d.x, -d.z);
            state.freePitch = std::asin(clamp(d.y, -1, 1));
        }
        state.mode = mode;
    }

    if (state.mode == 3)
    {
        float speed = state.freeSpeed * delta * (down(GLFW_KEY_LEFT_SHIFT) ? 3.5f : 1.f);
        vec3 f(std::sin(state.freeYaw), 0, -std::cos(state.freeYaw)),
            r(std::cos(state.freeYaw), 0, std::sin(state.freeYaw));
        if (down(GLFW_KEY_W))
        {
            state.freePosition += f * speed;
        }
        if (down(GLFW_KEY_S))
        {
            state.freePosition -= f * speed;
        }
        if (down(GLFW_KEY_D))
        {
            state.freePosition += r * speed;
        }
        if (down(GLFW_KEY_A))
        {
            state.freePosition -= r * speed;
        }
        if (down(GLFW_KEY_Q))
        {
            state.freePosition.y += speed;
        }
        if (down(GLFW_KEY_E))
        {
            state.freePosition.y -= speed;
        }
        bool drag = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        if (drag && dragging)
        {
            state.freeYaw += float(x - oldX) * .0027f;
            state.freePitch = clamp(state.freePitch - float(y - oldY) * .0027f, -1.42f, 1.42f);
        }
        oldX = x;
        oldY = y;
        dragging = drag;
    }
    else
    {
        if (down(GLFW_KEY_A) || down(GLFW_KEY_D) || down(GLFW_KEY_W) || down(GLFW_KEY_S) ||
            down(GLFW_KEY_Q) || down(GLFW_KEY_E))
        {
            state.mode = 2;
        }
        state.orbitYaw += ((down(GLFW_KEY_D) ? 1.f : 0) - (down(GLFW_KEY_A) ? 1.f : 0)) * delta;
        state.orbitPitch =
            clamp(state.orbitPitch +
                ((down(GLFW_KEY_W) ? 1.f : 0) - (down(GLFW_KEY_S) ? 1.f : 0)) * .65f * delta,
                .055f,
                1.4f);
        state.orbitDistance =
            clamp(state.orbitDistance +
                ((down(GLFW_KEY_E) ? 1.f : 0) - (down(GLFW_KEY_Q) ? 1.f : 0)) * 18 * delta,
                5,
                110);
    }
    state.lightAzimuth +=
        ((down(GLFW_KEY_L) ? 1.f : 0) - (down(GLFW_KEY_J) ? 1.f : 0)) * .7f * delta;
    state.lightElevation =
        clamp(state.lightElevation +
            ((down(GLFW_KEY_I) ? 1.f : 0) - (down(GLFW_KEY_K) ? 1.f : 0)) * .5f * delta,
            .22f,
            1.4f);
    state.lightRadius =
        clamp(state.lightRadius +
            ((down(GLFW_KEY_O) ? 1.f : 0) - (down(GLFW_KEY_U) ? 1.f : 0)) * 35 * delta,
            20,
            250);

    return pressed(GLFW_KEY_F12);
}

int main(int argc, char** argv)
{
    GLFWwindow* window = nullptr;
    bool initialized = false;
    try
    {
        Options opt = options(argc, argv);
        Circuit track;
        Race race(track);
        if (opt.help)
        {
            std::cout
                << "RaceCar3D [--validate] [--time SECONDS --screenshot FILE.bmp] [--benchmark "
                "FRAMES]\n[--low-quality] [--no-shadows] [--gouraud] [--no-accents] "
                "[--no-hud]\n[--camera 0..3] [--car 0..3] [--parallel] [--width W --height "
                "H]\nSee README_RU.md and press F1 in the application.\n";
            return 0;
        }
        if (opt.validate)
        {
            race.validate();
            auto root = assetRoot(argv[0]);
            validateCarParts(root / "Assets/Models/Parts/high");
            validateCarParts(root / "Assets/Models/Parts/low");
            auto mesh = loadSMF(root / "Assets/Models/sports_car_detailed.smf");
            std::cout << "Original SMF valid: " << mesh.vertices.size() << " vertices, "
                << mesh.indices.size() / 3 << " triangles\n";
            return 0;
        }
        const auto root = assetRoot(argv[0]);
        glfwSetErrorCallback(
            [](int code, const char* msg) { std::cerr << "GLFW " << code << ": " << msg << '\n'; });
        if (!glfwInit())
        {
            throw std::runtime_error("GLFW initialization failed");
        }
        initialized = true;
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SAMPLES, 4);
        glfwWindowHint(GLFW_STENCIL_BITS, 8);
        glfwWindowHint(GLFW_VISIBLE, opt.hidden ? GLFW_FALSE : GLFW_TRUE);
        window = glfwCreateWindow(
            opt.width, opt.height, "3D Animation Race Car | APEX CIRCUIT", nullptr, nullptr);
        if (!window)
        {
            throw std::runtime_error("OpenGL 3.3 core window creation failed");
        }
        glfwMakeContextCurrent(window);
        glfwSwapInterval(opt.hidden ? 0 : 1);
        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
        {
            throw std::runtime_error("GLAD initialization failed");
        }
        {
            Renderer renderer(root, track);
            Hud hud(root);
            CameraState state = opt.camera;
            HudState ui;
            ui.visible = opt.hud;
            ui.help = opt.overlay;
            std::ifstream team(root / "Assets/team.txt");
            if (team)
            {
                std::string name;
                std::getline(team, name);
                if (!name.empty())
                {
                    ui.team = name;
                }
            }
            glfwSetWindowUserPointer(window, &state);
            glfwSetScrollCallback(window, [](GLFWwindow* w, double, double y) {
                auto* s = static_cast<CameraState*>(glfwGetWindowUserPointer(w));
                s->orbitDistance = clamp(s->orbitDistance - float(y) * 1.8f, 5, 110);
                });
            double timeline = opt.time >= 0 ? std::min(120., std::max(0., opt.time)) : 0;
            ui.started = opt.time >= 0 || opt.benchmark > 0;
            ui.paused = opt.time >= 0 && opt.screenshot.empty();
            InputState input;
            double last = glfwGetTime(), fpsAt = last;
            int fpsFrames = 0, frameNumber = 0;
            double renderSeconds = 0;
            while (!glfwWindowShouldClose(window))
            {
                double now = glfwGetTime(), dt = std::min(.25, std::max(0., now - last));
                last = now;
                glfwPollEvents();
                bool capture = processInput(window,
                    renderer,
                    race,
                    state,
                    ui,
                    input,
                    timeline,
                    float(dt),
                    opt.width,
                    opt.height);
                if (opt.benchmark > 0)
                {
                    timeline = opt.time >= 0 ? std::fmod(opt.time + frameNumber / 60., 120.)
                        : std::fmod(frameNumber * 120. / opt.benchmark, 120.);
                    ui.started = true;
                    ui.paused = false;
                }
                else if (ui.started && !ui.paused && ui.animation && opt.screenshot.empty())
                {
                    timeline = std::min(FILM_SECONDS, timeline + dt);
                }
                int w, h;
                glfwGetFramebufferSize(window, &w, &h);
                if (w <= 0 || h <= 0)
                {
                    continue;
                }
                auto start = std::chrono::steady_clock::now();
                RaceFrame frame = race.evaluate(timeline, state.accents);
                renderer.draw(frame, timeline, state, w, h);
                hud.draw(track, frame, race, timeline, state, ui, w, h);
                if (opt.benchmark > 0)
                {
                    glFinish();
                }
                renderSeconds +=
                    std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

                if (!opt.screenshot.empty())
                {
                    glFinish();
                    renderer.saveScreenshot(opt.screenshot, w, h);
                    break;
                }
                if (capture)
                {
                    std::ostringstream name;
                    name << "race_" << std::fixed << std::setprecision(2) << timeline << ".bmp";
                    renderer.saveScreenshot(root / "Screenshots" / name.str(), w, h);
                }
                glfwSwapBuffers(window);
                ++frameNumber;
                ++fpsFrames;
                if (now - fpsAt >= .5)
                {
                    ui.fps = float(fpsFrames / (now - fpsAt));
                    fpsFrames = 0;
                    fpsAt = now;
                }
                if (opt.benchmark > 0 && frameNumber >= opt.benchmark)
                {
                    std::cout << "BENCHMARK " << (state.lowQuality ? "LOW" : "HIGH") << " "
                        << (state.shading ? "PHONG" : "GOURAUD") << ": " << frameNumber
                        << " frames; " << frameNumber / renderSeconds << " FPS; "
                        << renderSeconds * 1000 / frameNumber << " ms/frame at " << w << "x"
                        << h << "\n";
                    break;
                }
            }
            GLenum error = glGetError();
            if (error != GL_NO_ERROR)
            {
                throw std::runtime_error("OpenGL error at shutdown: " + std::to_string(error));
            }
        }
        glfwDestroyWindow(window);
        glfwTerminate();
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "ERROR: " << e.what() << '\n';
        if (window)
        {
            glfwDestroyWindow(window);
        }
        if (initialized)
        {
            glfwTerminate();
        }
        return 1;
    }
}
