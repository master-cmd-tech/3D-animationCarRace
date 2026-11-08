#include "Hud.hpp"
#include <iomanip>
#include <map>
namespace apex
{
static const std::map<char, std::array<unsigned char, 7>> FONT = {
    {'A', {14, 17, 17, 31, 17, 17, 17}}, {'B', {30, 17, 17, 30, 17, 17, 30}},
    {'C', {14, 17, 16, 16, 16, 17, 14}}, {'D', {30, 17, 17, 17, 17, 17, 30}},
    {'E', {31, 16, 16, 30, 16, 16, 31}}, {'F', {31, 16, 16, 30, 16, 16, 16}},
    {'G', {14, 17, 16, 23, 17, 17, 15}}, {'H', {17, 17, 17, 31, 17, 17, 17}},
    {'I', {31, 4, 4, 4, 4, 4, 31}},      {'J', {7, 2, 2, 2, 18, 18, 12}},
    {'K', {17, 18, 20, 24, 20, 18, 17}}, {'L', {16, 16, 16, 16, 16, 16, 31}},
    {'M', {17, 27, 21, 21, 17, 17, 17}}, {'N', {17, 25, 21, 19, 17, 17, 17}},
    {'O', {14, 17, 17, 17, 17, 17, 14}}, {'P', {30, 17, 17, 30, 16, 16, 16}},
    {'Q', {14, 17, 17, 17, 21, 18, 13}}, {'R', {30, 17, 17, 30, 20, 18, 17}},
    {'S', {15, 16, 16, 14, 1, 1, 30}},   {'T', {31, 4, 4, 4, 4, 4, 4}},
    {'U', {17, 17, 17, 17, 17, 17, 14}}, {'V', {17, 17, 17, 17, 17, 10, 4}},
    {'W', {17, 17, 17, 21, 21, 21, 10}}, {'X', {17, 17, 10, 4, 10, 17, 17}},
    {'Y', {17, 17, 10, 4, 4, 4, 4}},     {'Z', {31, 1, 2, 4, 8, 16, 31}},
    {'0', {14, 17, 19, 21, 25, 17, 14}}, {'1', {4, 12, 4, 4, 4, 4, 14}},
    {'2', {14, 17, 1, 2, 4, 8, 31}},     {'3', {30, 1, 1, 14, 1, 1, 30}},
    {'4', {2, 6, 10, 18, 31, 2, 2}},     {'5', {31, 16, 16, 30, 1, 1, 30}},
    {'6', {14, 16, 16, 30, 17, 17, 14}}, {'7', {31, 1, 2, 4, 8, 8, 8}},
    {'8', {14, 17, 17, 14, 17, 17, 14}}, {'9', {14, 17, 17, 15, 1, 1, 14}},
    {':', {0, 4, 4, 0, 4, 4, 0}},        {'.', {0, 0, 0, 0, 0, 4, 4}},
    {'-', {0, 0, 0, 31, 0, 0, 0}},       {'+', {0, 4, 4, 31, 4, 4, 0}},
    {'/', {1, 2, 2, 4, 8, 8, 16}},       {'!', {4, 4, 4, 4, 4, 0, 4}},
    {'%', {17, 2, 4, 4, 8, 16, 17}},     {'(', {2, 4, 8, 8, 8, 4, 2}},
    {')', {8, 4, 2, 2, 2, 4, 8}},        {'[', {14, 8, 8, 8, 8, 8, 14}},
    {']', {14, 2, 2, 2, 2, 2, 14}},      {'=', {0, 0, 31, 0, 31, 0, 0}}};
Hud::Hud(const std::filesystem::path &root)
{
    program = createProgram(root / "Shaders", "ui.vert", "ui.frag");
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Point), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        1, 4, GL_FLOAT, GL_FALSE, sizeof(Point), reinterpret_cast<void *>(sizeof(vec2)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}
Hud::~Hud()
{
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);
}
void Hud::rect(float x, float y, float w, float h, vec4 c)
{
    vec2 a(x, y), b(x + w, y), d(x, y + h), e(x + w, y + h);
    for (auto p : {a, b, e, a, e, d})
    {
        vertices.push_back({p, c});
    }
}
void Hud::line(vec2 a, vec2 b, float width, vec4 c)
{
    vec2 d = b - a;
    float l = glm::length(d);
    if (l < .001f)
    {
        return;
    }
    vec2 r(-d.y / l * width * .5f, d.x / l * width * .5f);
    for (auto p : {a + r, b + r, b - r, a + r, b - r, a - r})
    {
        vertices.push_back({p, c});
    }
}
void Hud::circle(vec2 c, float r, vec4 color)
{
    for (int i = 0; i < 16; ++i)
    {
        float a = i * 2 * PI / 16, b = (i + 1) * 2 * PI / 16;
        vertices.push_back({c, color});
        vertices.push_back({c + vec2(std::cos(a), std::sin(a)) * r, color});
        vertices.push_back({c + vec2(std::cos(b), std::sin(b)) * r, color});
    }
}
void Hud::text(float x, float y, const std::string &s, float scale, vec4 color)
{
    float begin = x;
    for (unsigned char raw : s)
    {
        char c = char(std::toupper(raw));
        if (c == '\n')
        {
            y += 10 * scale;
            x = begin;
            continue;
        }
        auto it = FONT.find(c);
        if (it != FONT.end())
        {
            for (int r = 0; r < 7; ++r)
            {
                for (int col = 0; col < 5; ++col)
                {
                    if (it->second[r] & (1 << (4 - col)))
                    {
                        rect(x + col * scale, y + r * scale, scale * .94f, scale * .94f, color);
                    }
                }
            }
        }
        x += 6 * scale;
    }
}
static std::string fixed(float value, int precision = 1)
{
    std::ostringstream s;
    s << std::fixed << std::setprecision(precision) << value;
    return s.str();
}
void Hud::draw(const Circuit &track, const RaceFrame &f, const Race &race, double time,
               const CameraState &camera, const HudState &s, int width, int height)
{
    if (!s.visible)
    {
        return;
    }
    vertices.clear();
    vec4 dark(.025f, .045f, .065f, .84f), muted(.65f, .76f, .81f, 1), white(.94f, .97f, 1, 1),
        gold(1, .73f, .23f, 1);
    rect(16, 16, 290, 88, dark);
    rect(16, 16, 4, 88, gold);
    text(31, 28, "APEX", 3.2f, white);
    text(31, 60, "CIRCUIT / 3D RACE CAR", 1.25f, muted);
    text(31, 81, s.team.substr(0, 32), 1.0f, muted);
    rect(485, 16, 310, 70, dark);
    text(503, 27, "ONE LAP / 120 SECOND FILM", 1.1f, muted);
    int seconds = int(time);
    std::ostringstream clock;
    clock << std::setfill('0') << std::setw(2) << seconds / 60 << ":" << std::setw(2)
          << seconds % 60;
    std::string timer = clock.str() + " / 02:00";
    text(516, 48, timer, 2.05f, white);
    float pct = clamp(f.cars[f.order[0]].progress / track.length, 0, 1);
    rect(485, 82, 310, 4, {.12f, .19f, .22f, 1});
    rect(485, 82, 310 * pct, 4, gold);
    rect(1036, 16, 228, 184, dark);
    text(1050, 28, "RACE POSITIONS", 1.25f, muted);
    for (int row = 0; row < 4; ++row)
    {
        int i = f.order[row];
        float y = 53 + row * 33.f;
        vec4 color(CARS[i].paint, 1);
        rect(1047, y, 4, 25, color);
        text(1062, y + 6, std::to_string(row + 1), 1.5f, white);
        text(1081, y + 6, CARS[i].name, 1.5f, color);
        std::string gap =
            row == 0 ? "LEAD" : "+" + fixed(f.cars[f.order[0]].progress - f.cars[i].progress) + "M";
        text(1188, y + 8, gap, 1.0f, muted);
    }
    // Minimal map uses the same closed spline as the cars and road.
    rect(16, 493, 245, 194, dark);
    text(30, 506, "APEX CIRCUIT", 1.05f, muted);
    auto map = [](vec3 p) { return vec2(138 + p.x * .26f, 602 + p.z * .213f); };
    for (std::size_t i = 9; i < track.points.size(); i += 9)
    {
        line(map(track.points[i - 9].p), map(track.points[i].p), 2.0f, {.39f, .53f, .55f, 1});
    }
    for (int i = 3; i >= 0; --i)
    {
        vec2 p = map(f.cars[i].p);
        circle(p, 5.5f, {.015f, .025f, .04f, 1});
        circle(p, 3.8f, vec4(CARS[i].paint, 1));
    }
    vec2 start = map(track.sample(0).p);
    rect(start.x - 2, start.y - 2, 4, 4, white);
    text(30, 671, fixed(track.length / 1000, 2) + " KM / CLOSED CIRCUIT", .9f, muted);
    rect(1027, 538, 237, 149, dark);
    int selected = camera.mode ? camera.selected : f.order[0];
    auto &c = f.cars[selected];
    text(1044,
         551,
         CARS[selected].name + std::string(" / #") + std::to_string(CARS[selected].number),
         1.3f,
         vec4(CARS[selected].paint, 1));
    text(1043, 578, std::to_string(int(c.speed * 3.6f)), 3.8f, white);
    text(1166, 591, "KM/H", 1.15f, muted);
    text(1044, 623, camera.shading ? "PHONG" : "GOURAUD", 1.15f, white);
    text(1148, 623, camera.lowQuality ? "LOW" : "HIGH", 1.15f, camera.lowQuality ? muted : gold);
    text(1044,
         644,
         "ANIMATION " + std::string(s.animation ? "ON" : "OFF"),
         1.1f,
         s.animation ? vec4(.34f, .94f, .68f, 1) : gold);
    text(1044, 666, s.fps > 0 ? "FPS " + std::to_string(int(s.fps)) : "FPS --", 1, muted);
    rect(285, 641, 716, 46, dark);
    text(301, 652, f.event, 1.28f, white);
    text(301,
         672,
         "CAM " +
             std::string(camera.mode == 0   ? "DIRECTOR"
                         : camera.mode == 1 ? "CHASE"
                         : camera.mode == 2 ? "ORBIT"
                                            : "FREE") +
             " / F1 HELP / SPACE PAUSE / R REPLAY",
         .95f,
         muted);
    rect(0, 701, 1280, 19, {.015f, .025f, .035f, .87f});
    text(21,
         706,
         "C CAMERA  V CAR  T ANIMATION  B BODY MOTION  1/2 SHADING  F2 QUALITY  F3 SHADOWS  P "
         "PROJECTION  ESC EXIT",
         .95f,
         muted);
    if (!s.started)
    {
        rect(330, 250, 620, 198, {.02f, .04f, .065f, .93f});
        rect(330, 250, 620, 4, gold);
        text(380, 279, "FOUR RACERS. ONE CHANCE.", 2.6f, white);
        text(407, 322, "A CARTOON INSPIRED RACE FILM", 1.5f, muted);
        text(426, 365, "[ ENTER ] START THE RACE", 2, gold);
        text(423, 413, "SPACE PAUSE / R RESTART / F1 HELP", 1.2f, muted);
    }
    else if (time >= 3 && time < 7.2)
    {
        std::string number = time < 6 ? std::to_string(int(std::ceil(6 - time))) : "GO!";
        float scale = time < 6 ? 8.f : 6.f;
        float x = 640 - float(number.size()) * 6 * scale * .5f;
        rect(x - 18,
             208,
             float(number.size()) * 6 * scale + 30,
             7 * scale + 35,
             {.02f, .04f, .06f, .68f});
        text(x, 225, number, scale, time < 6 ? gold : vec4(.28f, 1, .56f, 1));
    }
    if (time >= 112)
    {
        rect(391, 513, 498, 96, {.025f, .045f, .06f, .91f});
        rect(391, 513, 4, 96, gold);
        text(422, 529, "NOVA WINS!", 3.8f, gold);
        text(425,
             567,
             "PHOTO FINISH  +" + fixed(float(race.finishTimes[1] - race.finishTimes[0]), 3) + " S",
             1.45f,
             white);
        text(425, 591, "R TO REPLAY THE FULL 120 SECOND RACE", 1.05f, muted);
    }
    if (s.paused || !s.animation)
    {
        rect(493, 117, 294, 40, {.02f, .04f, .06f, .88f});
        text(512,
             129,
             !s.animation ? "SPLINE ANIMATION OFF" : "PAUSED - SPACE TO RESUME",
             1.55f,
             gold);
    }
    if (s.help)
    {
        rect(264, 144, 754, 459, {.018f, .035f, .05f, .97f});
        rect(264, 144, 754, 4, gold);
        text(289, 167, "RACE CAR / CONTROLS", 2.3f, white);
        const char *left[] = {"ENTER   START / RESUME",
                              "SPACE   PAUSE TIMELINE",
                              "R       RESTART FROM THE GRID",
                              "T       SPLINE ANIMATION ON/OFF",
                              "B       DRIFT / WHEELS / BODY",
                              "F       FREE CAMERA",
                              "C       DIRECTOR/CHASE/ORBIT/FREE",
                              "V       SELECT FOLLOWED CAR",
                              "F2      HIGH / LOW QUALITY",
                              "F3      SHADOWS ON/OFF",
                              "F1/H    SHOW THIS HELP"};
        const char *right[] = {"1 / 2   GOURAUD / PHONG",
                               "3 / 4   GLOSS / MATTE PAINT",
                               "5       BRONZE MATERIAL",
                               "P       PERSPECTIVE / PARALLEL",
                               "A / D   ORBIT CAMERA",
                               "W / S   ORBIT HEIGHT",
                               "Q / E   ORBIT ZOOM",
                               "WASD    MOVE IN FREE CAMERA",
                               "RMB     LOOK IN FREE CAMERA",
                               "J/L I/K ROTATE THE LIGHT",
                               "F12     SAVE BMP SCREENSHOT"};
        for (int i = 0; i < 11; ++i)
        {
            text(290, 215.f + i * 28.f, left[i], 1.22f, muted);
            text(672, 215.f + i * 28.f, right[i], 1.22f, muted);
        }
        text(290, 555, "FREE CAMERA: Q/E UP/DOWN - SHIFT FAST - MOUSE WHEEL ZOOM", 1.12f, gold);
        text(290, 580, "F10 HUD ON/OFF  /  ESC EXIT", 1.12f, white);
    }
    glUseProgram(program);
    glUniform2f(glGetUniformLocation(program, "viewport"), 1280, 720);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Point), vertices.data(), GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, GLsizei(vertices.size()));
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}
} // namespace apex
