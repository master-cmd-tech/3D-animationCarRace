#include "Renderer.hpp"
namespace apex
{
GLuint compileShader(GLenum type, const char *source, const std::string &name)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        GLint size = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &size);
        std::string log(std::max(1, size), ' ');
        glGetShaderInfoLog(shader, size, nullptr, log.data());
        glDeleteShader(shader);
        throw std::runtime_error("Shader " + name + " failed:\n" + log);
    }
    return shader;
}
GLuint createProgram(const std::filesystem::path &folder, const std::string &vert,
                     const std::string &frag)
{
    // Read the two complete shader files, then compile and link as in the lab example.
    std::string vertexSource = readText(folder / vert);
    std::string fragmentSource = readText(folder / frag);
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexSource.c_str(), vert);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentSource.c_str(), frag);
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    glDeleteShader(vs);
    glDeleteShader(fs);
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        GLint size = 0;
        glGetProgramiv(p, GL_INFO_LOG_LENGTH, &size);
        std::string log(std::max(1, size), ' ');
        glGetProgramInfoLog(p, size, nullptr, log.data());
        glDeleteProgram(p);
        throw std::runtime_error("Program link failed:\n" + log);
    }
    return p;
}

CpuMesh processMesh(const std::vector<vec3> &positions, const std::vector<Triangle> &triangles)
{
    CpuMesh mesh;
    mesh.vertices.resize(positions.size());
    std::vector<vec3> normalSums(positions.size(), vec3(0));

    // Add each face normal to its three vertices, then normalize the sums.
    for (const Triangle &triangle : triangles)
    {
        for (int index : {triangle.a, triangle.b, triangle.c})
        {
            if (index < 0 || index >= int(positions.size()))
            {
                throw std::runtime_error("SMF index out of bounds");
            }
        }

        const vec3 &a = positions[triangle.a];
        const vec3 &b = positions[triangle.b];
        const vec3 &c = positions[triangle.c];
        vec3 normal = glm::cross(b - a, c - a);
        for (int index : {triangle.a, triangle.b, triangle.c})
        {
            normalSums[index] += normal;
            mesh.indices.push_back(index);
        }
    }
    for (std::size_t i = 0; i < positions.size(); ++i)
    {
        mesh.vertices[i] = {positions[i], normalized(normalSums[i]), vec3(1)};
    }
    return mesh;
}

CpuMesh loadSMF(const std::filesystem::path &p)
{
    std::ifstream f(p);
    if (!f)
    {
        throw std::runtime_error("Missing SMF: " + p.string());
    }
    std::vector<vec3> positions;
    std::vector<Triangle> triangles;
    std::string line;
    bool zero = false;
    while (std::getline(f, line))
    {
        std::istringstream s(line);
        std::string type;
        s >> type;
        if (type.empty() || type[0] == '#')
        {
            continue;
        }
        if (type == "v")
        {
            vec3 a;
            if (!(s >> a.x >> a.y >> a.z) || !std::isfinite(a.x + a.y + a.z))
            {
                throw std::runtime_error("Bad SMF vertex");
            }
            positions.push_back(a);
        }
        else if (type == "f")
        {
            Triangle triangle;
            if (!(s >> triangle.a >> triangle.b >> triangle.c))
            {
                throw std::runtime_error("Bad SMF triangle");
            }
            zero |= triangle.a == 0 || triangle.b == 0 || triangle.c == 0;
            triangles.push_back(triangle);
        }
    }
    if (positions.empty() || triangles.empty())
    {
        throw std::runtime_error("Empty SMF");
    }
    if (!zero)
    {
        for (Triangle &triangle : triangles)
        {
            --triangle.a;
            --triangle.b;
            --triangle.c;
        }
    }
    return processMesh(positions, triangles);
}

struct PartDescription
{
    std::string file;
    int transformGroup = 0, painted = 0;
    vec3 diffuse{1};
    float shininess = 80;
};
static std::vector<PartDescription> readPartList(const std::filesystem::path &folder)
{
    std::ifstream file(folder / "parts.txt");
    if (!file)
    {
        throw std::runtime_error("Missing parts.txt in " + folder.string());
    }
    std::vector<PartDescription> parts;
    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
        {
            continue;
        }
        std::istringstream input(line);
        PartDescription part;
        if (!(input >> part.file >> part.transformGroup >> part.painted >> part.diffuse.r >>
              part.diffuse.g >> part.diffuse.b >> part.shininess))
        {
            throw std::runtime_error("Invalid material/part description");
        }
        if (part.transformGroup < 0 || part.transformGroup > 8)
        {
            throw std::runtime_error("Invalid transform group");
        }
        parts.push_back(part);
    }
    if (parts.empty())
    {
        throw std::runtime_error("Empty car part list");
    }
    return parts;
}
std::size_t validateCarParts(const std::filesystem::path &folder)
{
    std::size_t triangles = 0;
    for (const auto &part : readPartList(folder))
    {
        triangles += loadSMF(folder / part.file).indices.size() / 3;
    }
    std::cout << folder.filename().string() << ": " << triangles
              << " triangles in ordinary SMF parts\n";
    return triangles;
}
static CarModel loadCarModel(const std::filesystem::path &folder)
{
    CarModel model;
    for (const auto &description : readPartList(folder))
    {
        CpuMesh cpu = loadSMF(folder / description.file);
        ModelPart part;
        part.mesh.upload(cpu);
        part.transformGroup = description.transformGroup;
        part.material.diffuse = description.diffuse;
        part.material.ambient = description.diffuse * .28f;
        part.material.shininess = description.shininess;
        part.material.painted = description.painted != 0;
        model.triangles += cpu.indices.size() / 3;
        model.parts.push_back(part);
    }
    return model;
}
void CarModel::destroy()
{
    for (auto &part : parts)
    {
        part.mesh.destroy();
    }
}
Renderer::Renderer(const std::filesystem::path &root, const Circuit &t) : track(t)
{
    gpu = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
    std::cout << "OpenGL " << glGetString(GL_VERSION) << " | " << gpu << '\n';
    program = createProgram(root / "Shaders", "scene.vert", "scene.frag");
    CpuMesh scenery = buildEnvironment(t);
    sceneryTriangles = scenery.indices.size() / 3;
    environment.upload(scenery);
    CpuMesh casters;
    casters.vertices = scenery.vertices;
    for (std::size_t i = 0; i < scenery.indices.size(); i += 3)
    {
        auto a = scenery.indices[i], b = scenery.indices[i + 1], c = scenery.indices[i + 2];
        float height =
            std::max({scenery.vertices[a].p.y, scenery.vertices[b].p.y, scenery.vertices[c].p.y});
        if (height > .2f && height < 25.f)
        {
            casters.indices.insert(casters.indices.end(), {a, b, c});
        }
    }
    staticShadowCasters.upload(casters);
    carMedium = loadCarModel(root / "Assets/Models/Parts/high");
    carLow = loadCarModel(root / "Assets/Models/Parts/low");
    mediumTriangles = carMedium.triangles;
    lowTriangles = carLow.triangles;
    for (int i = 0; i < 4; ++i)
    {
        numbers[i].upload(buildNumber(CARS[i].number));
    }
    lightsRed.upload(buildStartLights({.95f, .045f, .02f}));
    lightsGreen.upload(buildStartLights({.05f, .98f, .27f}));
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    std::cout << "SMF car: " << mediumTriangles << " triangles; scenery: " << sceneryTriangles
              << " triangles\n";
}
Renderer::~Renderer()
{
    environment.destroy();
    staticShadowCasters.destroy();
    carMedium.destroy();
    carLow.destroy();
    for (auto &number : numbers)
    {
        number.destroy();
    }
    lightsRed.destroy();
    lightsGreen.destroy();
    particles.destroy();
    if (program)
    {
        glDeleteProgram(program);
    }
}
void Renderer::setModel(const mat4 &model, bool flat)
{
    setM(program, "model", model);
    // Lighting is calculated in view coordinates, as in Main (2).cpp.
    // The planar shadow matrix is singular; flat shadows do not need normals.
    glm::mat3 normal = flat ? glm::mat3(1) : glm::transpose(glm::inverse(glm::mat3(view * model)));
    glUniformMatrix3fv(
        glGetUniformLocation(program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normal));
}
void Renderer::setMaterial(const Material &m, bool colors)
{
    set3(program, "materialAmbient", m.ambient);
    set3(program, "materialDiffuse", m.diffuse);
    set3(program, "materialSpecular", m.specular);
    setF(program, "materialShininess", m.shininess);
    setI(program, "vertexColors", colors);
}
void Renderer::drawCars(const RaceFrame &frame, const CameraState &state, const mat4 &projection,
                        bool shadow)
{
    CarModel &model = state.lowQuality ? carLow : carMedium;
    for (int i = 0; i < 4; ++i)
    {
        const CarPose &car = frame.cars[i];
        for (const ModelPart &part : model.parts)
        {
            mat4 child(1);
            if (part.transformGroup > 0)
            {
                int wheel = (part.transformGroup - 1) % 4;
                vec3 pivot(wheel % 2 == 0 ? -.875f : .875f, .355f, wheel < 2 ? -1.31f : 1.31f);
                // Parent body -> wheel pivot -> steering -> spin -> local part.
                child = glm::translate(child, pivot);
                if (wheel < 2)
                {
                    child = glm::rotate(child, car.steer, vec3(0, 1, 0));
                }
                if (part.transformGroup <= 4)
                {
                    child = glm::rotate(child, car.spin, vec3(1, 0, 0));
                }
                child = glm::translate(child, -pivot);
            }
            setModel(projection * car.model * child, shadow);
            if (!shadow)
            {
                Material m = part.material;
                if (m.painted)
                {
                    m.diffuse *= CARS[i].paint;
                    if (state.material == 1)
                    {
                        m.specular = vec3(.13f);
                        m.shininess = 18;
                    }
                    if (state.material == 2)
                    {
                        m.diffuse = vec3(.67f, .36f, .12f);
                        m.shininess = 48;
                    }
                    m.ambient = m.diffuse * .28f;
                }
                setMaterial(m, false);
            }
            part.mesh.draw();
        }
        if (!shadow)
        {
            setModel(car.model);
            Material m;
            m.diffuse = vec3(.95f);
            setMaterial(m, false);
            numbers[i].draw();
        }
    }
}
Camera Renderer::camera(const RaceFrame &f, double time, const CameraState &s, int w, int h) const
{
    Camera c;
    float t = float(time);
    vec3 anchor(0), forward(0), right(0);
    for (auto &a : f.cars)
    {
        anchor += a.p * .25f;
        forward += a.forward;
    }
    forward = normalized(forward);
    right = normalized(glm::cross(forward, vec3(0, 1, 0)));
    float side = 9, height = 5.6f, ahead = 20, look = 1;
    if (t < 6)
    {
        side = 13 - t * .5f;
        height = 4.3f;
        ahead = 19;
        look = -1;
    }
    else if (t < 18)
    {
        side = -8;
        height = 6;
        ahead = -18;
        look = 3;
    }
    else if (t < 32)
    {
        side = 11;
        height = 3.2f;
        ahead = 18;
    }
    else if (t < 43)
    {
        side = -13;
        height = 23;
        ahead = -11;
    }
    else if (t < 54)
    {
        auto &a = f.cars[0];
        anchor = a.p;
        forward = a.forward;
        right = a.right;
        side = -10;
        height = 3.7f;
        ahead = 14;
        look = 0;
    }
    else if (t < 68)
    {
        side = 10;
        height = 4.0f;
        ahead = 17;
    }
    else if (t < 83)
    {
        side = -8;
        height = 5.0f;
        ahead = -19;
        look = 4;
    }
    else if (t < 96)
    {
        side = 13;
        height = 16;
        ahead = 12;
    }
    else if (t < 103)
    {
        side = -8;
        height = 3.5f;
        ahead = 20;
    }
    else if (t < 112)
    {
        anchor = (f.cars[0].p + f.cars[1].p) * .5f;
        side = 16;
        height = 3.1f;
        ahead = 10;
        look = 0;
    }
    else
    {
        anchor = f.cars[0].p;
        forward = f.cars[0].forward;
        right = f.cars[0].right;
        side = -10;
        height = 3.7f;
        ahead = 15;
        look = -2;
    }
    side += std::sin(t * .23f) * .65f;
    c.eye = anchor + right * side + forward * ahead + vec3(0, height, 0);
    c.target = anchor + forward * look + vec3(0, .75f, 0);
    if (s.mode == 1)
    {
        const auto &a = f.cars[s.selected];
        c.eye = a.p - a.forward * 13.f + a.right * 4.f + vec3(0, 4.2f, 0);
        c.target = a.p + a.forward * 6.f + vec3(0, .65f, 0);
    }
    if (s.mode == 2)
    {
        const auto &a = f.cars[s.selected];
        vec3 d(std::cos(s.orbitYaw) * std::cos(s.orbitPitch),
               std::sin(s.orbitPitch),
               std::sin(s.orbitYaw) * std::cos(s.orbitPitch));
        c.eye = a.p + d * s.orbitDistance;
        c.target = a.p + vec3(0, .6f, 0);
    }
    if (s.mode == 3)
    {
        c.eye = s.freePosition;
        c.target = c.eye + vec3(std::cos(s.freePitch) * std::sin(s.freeYaw),
                                std::sin(s.freePitch),
                                -std::cos(s.freePitch) * std::cos(s.freeYaw));
    }
    c.view = glm::lookAt(c.eye, c.target, vec3(0, 1, 0));
    float aspect = float(w) / std::max(1, h);
    if (s.perspective)
    {
        c.projection = glm::perspective(glm::radians(52.f), aspect, .09f, 2400.f);
    }
    else
    {
        float size = s.mode == 2 ? s.orbitDistance * .44f : 14.f;
        c.projection = glm::ortho(-size * aspect, size * aspect, -size, size, .1f, 2400.f);
    }
    return c;
}

void Renderer::draw(const RaceFrame &f, double time, const CameraState &s, int width, int height)
{
    Camera cam = camera(f, time, s, width, height);
    view = cam.view;
    vec3 anchor(0);
    for (const CarPose &car : f.cars)
    {
        anchor += car.p * .25f;
    }
    vec3 sun(std::cos(s.lightAzimuth) * std::cos(s.lightElevation),
             std::sin(s.lightElevation),
             std::sin(s.lightAzimuth) * std::cos(s.lightElevation));
    glViewport(0, 0, width, height);
    glStencilMask(0xFF);
    glClearColor(.60f, .74f, .82f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glUseProgram(program);
    setM(program, "view", cam.view);
    setM(program, "projection", cam.projection);
    // A directional sun keeps the large circuit evenly lit. The second light
    // is a point light, matching the two light types covered in week five.
    set3(program, "light1DirectionView", vec3(view * vec4(sun, 0)));
    vec3 fill = anchor + vec3(-sun.z, .45f, sun.x) * s.lightRadius + vec3(0, 15, 0);
    set3(program, "light2PositionView", vec3(view * vec4(fill, 1)));
    set3(program, "lightAmbient", vec3(.40f));
    set3(program, "lightDiffuse", vec3(.57f));
    set3(program, "lightSpecular", vec3(.8f));
    setI(program, "shadingMode", s.shading);
    setI(program, "flatPass", false);
    setF(program, "objectAlpha", 1);
    setModel(mat4(1));
    setMaterial(Material{}, true);
    environment.draw();
    if (s.shadows && !s.lowQuality)
    {
        // Matrix projection onto a plane + depth, stencil and alpha blending.
        // Stencil ensures a shadow pixel is blended once, including overlaps.
        const float floor = .060f;
        mat4 plane(1);
        plane[1] = vec4(-sun.x / sun.y, 0, -sun.z / sun.y, 0);
        plane[3] = vec4(sun.x / sun.y * floor, floor + .004f, sun.z / sun.y * floor, 1);
        setI(program, "flatPass", true);
        setMaterial(Material{}, false);
        glUniform4f(glGetUniformLocation(program, "flatColor"), .025f, .035f, .045f, .34f);
        glEnable(GL_STENCIL_TEST);
        glStencilFunc(GL_EQUAL, 0, 0xFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        glDisable(GL_CULL_FACE);
        setModel(plane, true);
        staticShadowCasters.draw();
        drawCars(f, s, plane, true);
        glDepthMask(GL_TRUE);
        glEnable(GL_CULL_FACE);
        glDisable(GL_BLEND);
        glDisable(GL_STENCIL_TEST);
        setI(program, "flatPass", false);
    }
    drawCars(f, s, mat4(1), false);
    setModel(mat4(1));
    setMaterial(Material{}, true);
    if (time < 9)
    {
        setI(program, "flatPass", true);
        glUniform4f(glGetUniformLocation(program, "flatColor"), 1, 1, 1, 1);
        if (time < 6)
        {
            lightsRed.draw();
        }
        else
        {
            lightsGreen.draw();
        }
        setI(program, "flatPass", false);
    }
    // Bounded transparent batch: contact shade, drifting smoke and a short boost.
    CpuMesh fx;
    vec3 up = normalized(vec3(cam.view[0][1], cam.view[1][1], cam.view[2][1])),
         right = normalized(glm::cross(cam.target - cam.eye, vec3(0, 1, 0)));
    for (auto &a : f.cars)
    {
        vec3 c = a.p + vec3(0, .009f, 0);
        for (int j = 0; j < 16; ++j)
        {
            float a0 = j * 2 * PI / 16, a1 = (j + 1) * 2 * PI / 16;
            triangle(fx,
                     c,
                     c + a.right * (1.05f * std::cos(a0)) + a.forward * (2.1f * std::sin(a0)),
                     c + a.right * (1.05f * std::cos(a1)) + a.forward * (2.1f * std::sin(a1)),
                     {.015f, .02f, .025f});
        }
    }
    if (s.accents && time > 43 && time < 56)
    {
        Race history(track);
        for (int i = 0; i < 34; ++i)
        {
            float age = i * .055f;
            float then = float(time) - age;
            if (then < 43 || then > 54)
            {
                continue;
            }
            auto old = history.evaluate(then).cars[0];
            if (std::abs(old.drift) < .02f)
            {
                continue;
            }
            vec3 c = vec3(old.model * vec4((i & 1) ? .9f : -.9f, .18f, 1.47f, 1));
            c.y += age * .5f;
            c += old.right * (std::sin(i * 2.2f) * age * .17f);
            float r = .12f + age * .35f;
            for (int j = 0; j < 9; ++j)
            {
                float a = j * 2 * PI / 9, b = (j + 1) * 2 * PI / 9;
                triangle(fx,
                         c,
                         c + right * (r * std::cos(a)) + up * (r * std::sin(a)),
                         c + right * (r * std::cos(b)) + up * (r * std::sin(b)),
                         {.65f, .69f, .70f});
            }
        }
    }
    if (s.accents && time > 71 && time < 84)
    {
        const auto &a = f.cars[2];
        for (float side : {-.65f, .65f})
        {
            vec3 c = vec3(a.model * vec4(side, .33f, 2.23f, 1));
            float len = .22f + .13f * std::sin(float(time) * 38);
            vec3 tip = c - a.forward * len;
            for (int j = 0; j < 6; ++j)
            {
                float v = j * 2 * PI / 6, w = (j + 1) * 2 * PI / 6;
                triangle(fx,
                         tip,
                         c + a.right * (.045f * std::cos(v)) + vec3(0, .045f * std::sin(v), 0),
                         c + a.right * (.045f * std::cos(w)) + vec3(0, .045f * std::sin(w), 0),
                         {.10f, .55f, 1.f});
            }
        }
    }

    particles.upload(fx);
    setModel(mat4(1));
    setMaterial(Material{}, true);
    setI(program, "flatPass", true);
    glUniform4f(glGetUniformLocation(program, "flatColor"), 1, 1, 1, .23f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    particles.draw();
    glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    setI(program, "flatPass", false);
}
void Renderer::saveScreenshot(const std::filesystem::path &p, int w, int h) const
{
    std::vector<unsigned char> pixels(std::size_t(w) * h * 4);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, w, h, GL_BGRA, GL_UNSIGNED_BYTE, pixels.data());
    if (!p.parent_path().empty())
    {
        std::filesystem::create_directories(p.parent_path());
    }
    std::ofstream f(p, std::ios::binary);
    if (!f)
    {
        throw std::runtime_error("Cannot save screenshot: " + p.string());
    }
    auto u16 = [&](std::uint16_t v) {
        f.put(char(v));
        f.put(char(v >> 8));
    };
    auto u32 = [&](std::uint32_t v) {
        for (int i = 0; i < 4; ++i)
        {
            f.put(char(v >> (8 * i)));
        }
    };
    f.put('B');
    f.put('M');
    u32(54 + w * h * 4);
    u32(0);
    u32(54);
    u32(40);
    u32(w);
    u32(h);
    u16(1);
    u16(32);
    u32(0);
    u32(w * h * 4);
    u32(2835);
    u32(2835);
    u32(0);
    u32(0);
    f.write(reinterpret_cast<char *>(pixels.data()), pixels.size());
    std::cout << "Screenshot: " << p.string() << '\n';
}
} // namespace apex
