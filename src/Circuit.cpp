#include "Circuit.hpp"
namespace apex
{
Circuit::Circuit()
{
    const vec2 xz[] = {{-200, -300}, {40, -300},  {245, -295}, {335, -230}, {350, -110},
                       {235, -60},   {180, 40},   {260, 125},  {335, 220},  {280, 330},
                       {140, 355},   {70, 255},   {-20, 205},  {-100, 295}, {-245, 320},
                       {-340, 220},  {-330, 75},  {-230, 45},  {-195, -55}, {-310, -105},
                       {-350, -205}, {-300, -280}};
    for (auto p : xz)
    {
        controls.emplace_back(p.x, 0, p.y);
    }
    const int count = int(controls.size()) * 84;
    for (int i = 0; i <= count; ++i)
    {
        float u = float(i) * controls.size() / count;
        TrackPoint p;
        p.p = spline(u);
        p.p.y = .055f;
        p.forward = normalized(spline(u + .003f) - spline(u - .003f));
        p.right = normalized(glm::cross(p.forward, vec3(0, 1, 0)));
        vec3 before = normalized(spline(u) - spline(u - .004f)),
             after = normalized(spline(u + .004f) - spline(u));
        float ds = glm::distance(spline(u + .004f), spline(u - .004f)) * .5f;
        p.curvature =
            std::atan2(glm::cross(before, after).y, glm::dot(before, after)) / std::max(ds, .001f);
        if (i)
        {
            length += glm::distance(points.back().p, p.p);
        }
        p.s = length;
        points.push_back(p);
    }
    referenceTime.push_back(0);
    for (std::size_t i = 1; i < points.size(); ++i)
    {
        float k = std::abs(points[i].curvature), speed = clamp(39.f / (1.f + 35.f * k), 17.f, 39.f);
        idealLap += (points[i].s - points[i - 1].s) / speed;
        referenceTime.push_back(idealLap);
    }
}
vec3 Circuit::spline(float u) const
{
    int n = int(controls.size());
    u = std::fmod(u, float(n));
    if (u < 0)
    {
        u += n;
    }
    int i = int(u);
    float t = u - i;
    const vec3 &a = controls[(i + n - 1) % n], &b = controls[i % n], &c = controls[(i + 1) % n],
               &d = controls[(i + 2) % n];
    // Closed cubic Bezier spans use shared endpoint tangents for a smooth join.
    const vec3 b0 = b;
    const vec3 b1 = b + (c - a) / 6.f;
    const vec3 b2 = c - (d - b) / 6.f;
    const vec3 b3 = c;

    // De Casteljau: three levels of linear interpolation evaluate one cubic span.
    const vec3 p01 = glm::mix(b0, b1, t);
    const vec3 p12 = glm::mix(b1, b2, t);
    const vec3 p23 = glm::mix(b2, b3, t);
    const vec3 q01 = glm::mix(p01, p12, t);
    const vec3 q12 = glm::mix(p12, p23, t);
    return glm::mix(q01, q12, t);
}
TrackPoint Circuit::sample(float s) const
{
    s = std::fmod(s, length);
    if (s < 0)
    {
        s += length;
    }
    auto it =
        std::upper_bound(points.begin(), points.end(), s, [](float value, const TrackPoint &p) {
            return value < p.s;
        });
    std::size_t i = std::max<std::size_t>(1, std::size_t(it - points.begin()));
    i = std::min(i, points.size() - 1);
    const auto &a = points[i - 1];
    const auto &b = points[i];
    float t = (s - a.s) / std::max(.0001f, b.s - a.s);
    TrackPoint r;
    r.s = s;
    r.p = glm::mix(a.p, b.p, t);
    r.forward = normalized(glm::mix(a.forward, b.forward, t));
    r.right = normalized(glm::cross(r.forward, vec3(0, 1, 0)));
    r.curvature = glm::mix(a.curvature, b.curvature, t);
    return r;
}
float Circuit::distanceAtTimeFraction(float f) const
{
    float wanted = clamp(f, 0, 1) * idealLap;
    auto it = std::lower_bound(referenceTime.begin(), referenceTime.end(), wanted);
    std::size_t i = std::max<std::size_t>(1, std::size_t(it - referenceTime.begin()));
    i = std::min(i, points.size() - 1);
    float t = (wanted - referenceTime[i - 1]) /
              std::max(.00001f, referenceTime[i] - referenceTime[i - 1]);
    return glm::mix(points[i - 1].s, points[i].s, t);
}

static void ribbon(CpuMesh &m, const Circuit &t, float s0, float s1, float a, float b, float y,
                   vec3 color)
{
    if (a > b)
    {
        std::swap(a, b);
    }
    auto p = t.sample(s0), q = t.sample(s1);
    vec3 lift(0, y, 0);
    quad(m,
         p.p + p.right * a + lift,
         p.p + p.right * b + lift,
         q.p + q.right * b + lift,
         q.p + q.right * a + lift,
         color);
}
CpuMesh buildEnvironment(const Circuit &t)
{
    CpuMesh m;
    quad(m,
         {-1800, 0, -1800},
         {-1800, 0, 1800},
         {1800, 0, 1800},
         {1800, 0, -1800},
         {.21f, .37f, .22f});
    const int n = int(std::ceil(t.length / 3));
    for (int i = 0; i < n; ++i)
    {
        float a = t.length * i / n, b = t.length * (i + 1) / n;
        ribbon(m, t, a, b, -10.7f, 10.7f, -.010f, {.29f, .39f, .31f});
        ribbon(m, t, a, b, -7.4f, 7.4f, 0, {.105f, .12f, .14f});
        for (int s : {-1, 1})
        {
            ribbon(m, t, a, b, s * 7.21f, s * 7.34f, .012f, {.88f, .90f, .85f});
            float lo = s > 0 ? 7.4f : -8.35f, hi = s > 0 ? 8.35f : -7.4f;
            vec3 curb = (i % 4 < 2) ? vec3(.82f, .13f, .12f) : vec3(.9f, .91f, .85f);
            ribbon(m, t, a, b, lo, hi, .025f, curb);
            auto p = t.sample(a), q = t.sample(b);
            float edge = s * 10.4f;
            vec3 pa = p.p + p.right * edge, pb = q.p + q.right * edge;
            vec3 col = (i / 9) % 2 ? vec3(.69f, .73f, .74f) : vec3(.84f, .86f, .84f);
            // One continuous batched barrier, never one draw call per block.
            vec3 normal = -float(s) * p.right;
            if (s > 0)
            {
                quad(m, pa, pb, pb + vec3(0, .72f, 0), pa + vec3(0, .72f, 0), col);
            }
            else
            {
                quad(m, pb, pa, pa + vec3(0, .72f, 0), pb + vec3(0, .72f, 0), col);
            }
            ribbon(m, t, a, b, s * 10.25f, s * 10.65f, .72f, col * .94f);
        }
    }
    for (int i = 0; i < 28; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            float lo = -7.4f + 14.8f * i / 28, hi = -7.4f + 14.8f * (i + 1) / 28;
            ribbon(m,
                   t,
                   -.65f + j * .325f,
                   -.65f + (j + 1) * .325f,
                   lo,
                   hi,
                   .031f,
                   ((i + j) & 1) ? vec3(.95f) : vec3(.035f));
        }
    }
    // Four starting boxes: two in front, two behind.
    for (float s : {-3.f, -11.5f})
    {
        for (float lane : {-2.4f, 2.4f})
        {
            ribbon(m, t, s - 2.3f, s - 2.15f, lane - 1.3f, lane + 1.3f, .027f, vec3(.85f));
            ribbon(m, t, s - 2.3f, s + 2.5f, lane - 1.3f, lane - 1.18f, .027f, vec3(.85f));
            ribbon(m, t, s - 2.3f, s + 2.5f, lane + 1.18f, lane + 1.3f, .027f, vec3(.85f));
        }
    }
    // Start gantry and lightweight pit buildings sit beside the main straight.
    auto start = t.sample(0);
    for (float side : {-1.f, 1.f})
    {
        box(m,
            start.p + start.right * (side * 9.2f) + vec3(0, 3.4f, 0),
            {.55f, 6.8f, .65f},
            {.20f, .24f, .28f});
    }
    box(m, start.p + vec3(0, 7.0f, 0), {1.0f, 1.2f, 19.2f}, {.11f, .14f, .18f});
    for (int i = 0; i < 6; ++i)
    {
        float x = -172.f + i * 27;
        box(m, {x, 2.5f, -278}, {25, 5, 12}, {.58f, .64f, .66f});
        box(m, {x, 5.2f, -278}, {26, .4f, 13}, {.85f, .87f, .85f});
        for (int j = 0; j < 3; ++j)
        {
            box(m, {x - 8.f + j * 8, 1.8f, -284.1f}, {5.9f, 3.2f, .1f}, {.13f, .21f, .26f});
        }
        box(m,
            {x, 4.7f, -284.3f},
            {19, .5f, .3f},
            i % 2 ? vec3(.05f, .58f, .69f) : vec3(.94f, .58f, .13f));
    }
    for (int stand = 0; stand < 3; ++stand)
    {
        float x = -190.f + stand * 68;
        for (int tier = 0; tier < 6; ++tier)
        {
            box(m,
                {x, 1.0f + tier * .75f, -325.f - tier * 2.1f},
                {58, 1.0f, 2.2f},
                {.46f, .50f, .54f});
            for (int j = 0; j < 30; ++j)
            {
                vec3 c = (j + tier) % 3 == 0 ? vec3(.86f, .35f, .22f)
                                             : ((j + tier) % 3 == 1 ? vec3(.16f, .45f, .61f)
                                                                    : vec3(.88f, .78f, .39f));
                box(m,
                    {x - 27 + j * 1.85f, 1.9f + tier * .75f, -325.f - tier * 2.1f},
                    {.6f, .65f, .48f},
                    c);
            }
        }
        box(m, {x, 7.1f, -331}, {62, .4f, 19}, {.20f, .27f, .31f});
    }
    // Deterministic trees and broad low-poly hills, avoiding the racing surface.
    std::uint32_t rng = 33017;
    auto random = [&]() {
        rng = 1664525 * rng + 1013904223;
        return float((rng >> 8) & 0xffffff) / 16777216.f;
    };
    for (int i = 0; i < 125; ++i)
    {
        vec3 p((random() - .5f) * 1000, 0, (random() - .5f) * 1000);
        float closest = 1e9f;
        for (std::size_t k = 0; k < t.points.size(); k += 9)
        {
            closest = std::min(closest, glm::distance(p, t.points[k].p));
        }
        if (closest < 22 || ((p.z < -270 && p.z > -355) && (p.x > -245 && p.x < 10)))
        {
            continue;
        }
        float h = 5 + random() * 7;
        box(m, p + vec3(0, h * .24f, 0), {.6f, h * .48f, .6f}, {.32f, .24f, .16f});
        cone(m,
             p + vec3(0, h * .3f, 0),
             h * .29f,
             h * .75f,
             {.12f + random() * .06f, .27f + random() * .10f, .19f},
             8);
    }
    for (int i = 0; i < 28; ++i)
    {
        float a = i * 2 * PI / 28, r = 700 + random() * 220;
        vec3 p(std::cos(a) * r, -1, std::sin(a) * r);
        cone(m, p, 120 + random() * 100, 35 + random() * 85, {.27f, .40f, .31f}, 10);
    }
    return m;
}

CpuMesh buildNumber(int number)
{
    CpuMesh m;
    const unsigned masks[10] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f};
    for (int side : {-1, 1})
    {
        for (int digit = 0; digit < 2; ++digit)
        {
            int value = digit == 0 ? number / 10 : number % 10;
            float z = .09f - side * (digit - .5f) * .20f;
            const vec2 seg[7][2] = {{{-.065f, .12f}, {.065f, .12f}},
                                    {{.065f, .12f}, {.065f, 0}},
                                    {{.065f, 0}, {.065f, -.12f}},
                                    {{-.065f, -.12f}, {.065f, -.12f}},
                                    {{-.065f, 0}, {-.065f, -.12f}},
                                    {{-.065f, .12f}, {-.065f, 0}},
                                    {{-.065f, 0}, {.065f, 0}}};
            for (int k = 0; k < 7; ++k)
            {
                if (masks[value] & (1 << k))
                {
                    auto a = seg[k][0], b = seg[k][1];
                    float width = .014f;
                    vec2 d = glm::normalize(b - a), r(-d.y, d.x);
                    a -= d * width * .4f;
                    b += d * width * .4f;
                    vec3 p[4];
                    vec2 q[4] = {a + r * width, b + r * width, b - r * width, a - r * width};
                    for (int j = 0; j < 4; ++j)
                    {
                        p[j] = {side * .926f, .523f + q[j].y, z - side * q[j].x};
                    }
                    quad(m, p[3], p[2], p[1], p[0], vec3(.95f));
                }
            }
        }
    }
    return m;
}
CpuMesh buildStartLights(vec3 color)
{
    CpuMesh m;
    for (int i = 0; i < 5; ++i)
    {
        vec3 c(-200.56f, 7, -304.f + i * 2);
        box(m, c, {.07f, .66f, .66f}, color);
    }
    return m;
}
} // namespace apex
