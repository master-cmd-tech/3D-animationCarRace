#include "Race.hpp"
#include <iomanip>
namespace apex
{
// Keyframe times and longitudinal offsets define the same overtakes on every run.
static constexpr float TIMES[] = {0, 10, 25, 35, 48, 60, 76, 88, 99, 104, 106};
static constexpr float OFFSETS[4][11] = {{0, -1, 3, 2, -8, -7, -3, 2, 6, 8.5f, 10},
                                         {0, 2, .5f, 4, 5, 3, 2, 3, 6.2f, 8.8f, 9.45f},
                                         {-8.5f, -6, .6f, 5, 1, -1, 5, 1, -1, 2, 3},
                                         {-8.5f, -7, -3, -5, -2, 4, -3, -5, -6, -3, -2}};
Race::Race(const Circuit &t) : track(t)
{
    for (int i = 0; i < 4; ++i)
    {
        double a = 6, b = 120;
        for (int j = 0; j < 40; ++j)
        {
            double mid = (a + b) * .5;
            if (progress(i, float(mid - 6)) < track.length)
            {
                a = mid;
            }
            else
            {
                b = mid;
            }
        }
        finishTimes[i] = (a + b) * .5;
    }
}
float Race::offset(int car, float q) const
{
    if (q <= 0)
    {
        return OFFSETS[car][0];
    }
    if (q >= 106)
    {
        return OFFSETS[car][10];
    }
    int i = 0;
    while (TIMES[i + 1] < q)
    {
        ++i;
    }
    return glm::mix(
        OFFSETS[car][i], OFFSETS[car][i + 1], smoother((q - TIMES[i]) / (TIMES[i + 1] - TIMES[i])));
}
float Race::base(float q) const
{
    if (q <= 0)
    {
        return -3;
    }
    if (q > 106)
    {
        float v = (base(106) - base(105.98f)) / .02f;
        return base(106) + v * 3.3f * (1 - std::exp(-(q - 106) / 3.3f));
    }
    auto phase = [](float x) { return x - 2.6f * (1 - std::exp(-x / 2.6f)); };
    float fraction = phase(q) / phase(106);
    return -3 + track.distanceAtTimeFraction(fraction) * (track.length - 7) / track.length;
}
float Race::progress(int i, float q) const
{
    return base(q) + offset(i, q);
}
float Race::lane(int i, float q) const
{
    const float start[4] = {-2.4f, 2.4f, -2.4f, 2.4f}, running[4] = {-3.9f, 1.3f, -1.3f, 3.9f};
    float x = glm::mix(start[i], running[i], smoother(q / 8));
    if (i == 0)
    {
        x -= .85f * pulse(q, 37, 48);
    }
    if (i == 0)
    {
        x += 2.6f * smoother((q - 99) / 5);
    }
    if (i == 2)
    {
        x -= 2.6f * smoother((q - 99) / 5);
    }
    x += .055f * std::sin(q * .6f + i * 1.9f) * smooth(q / 8);
    return x;
}
RaceFrame Race::evaluate(double seconds, bool accents) const
{
    RaceFrame frame;
    float t = float(clamp(float(seconds), 0, 120)), q = std::max(0.f, t - 6);
    for (int i = 0; i < 4; ++i)
    {
        CarPose &c = frame.cars[i];
        c.progress = progress(i, q);
        c.lane = lane(i, q);
        TrackPoint p = track.sample(c.progress);
        c.forward = p.forward;
        c.right = p.right;
        c.p = p.p + p.right * c.lane;
        c.speed = q > 0 ? (progress(i, q + .012f) - progress(i, std::max(0.f, q - .012f))) /
                              (q > .012f ? .024f : q + .012f)
                        : 0;
        c.drift = accents && i == 0 ? .25f * pulse(q, 37, 48) : 0;
        c.heading = std::atan2(-p.forward.x, -p.forward.z) + c.drift;
        c.steer = clamp(std::atan(2.62f * p.curvature) - c.drift * .7f, -.39f, .39f);
        c.roll = accents ? clamp(-p.curvature * c.speed * c.speed * .0008f, -.065f, .065f) : 0;
        c.spin = accents ? -c.progress / .354f : 0;
        float bob = accents ? .005f * std::sin(q * 14 + i) +
                                  .008f * std::sin(q * 3 + i) * smooth(c.speed / 6)
                            : 0;
        // Model matrix: local body roll, heading rotation, then world translation.
        c.model = glm::translate(mat4(1), c.p + vec3(0, bob, 0));
        c.model = glm::rotate(c.model, c.heading, vec3(0, 1, 0));
        c.model = glm::rotate(c.model, c.roll, vec3(0, 0, 1));
    }
    std::stable_sort(frame.order.begin(),
                     frame.order.end(),
                     [&](int a, int b) { return frame.cars[a].progress > frame.cars[b].progress; });
    if (t < 3)
    {
        frame.event = "FOUR RACERS. ONE CIRCUIT. ONE CHANCE.";
    }
    else if (t < 6)
    {
        frame.event = "ON THE GRID - TWO BY TWO";
    }
    else if (t < 17)
    {
        frame.event = "LIGHTS OUT! VOLT GETS A FLYING START";
    }
    else if (t < 32)
    {
        frame.event = "BLAZE TAKES THE GAP BETWEEN THE LEADERS";
    }
    else if (t < 43)
    {
        frame.event = "THREE CARS FIGHT FOR THE LEAD";
    }
    else if (t < 54)
    {
        frame.event = "NOVA SLIDES WIDE - CAN SHE RECOVER?";
    }
    else if (t < 68)
    {
        frame.event = "PIXEL SNEAKS PAST ON THE OUTSIDE";
    }
    else if (t < 85)
    {
        frame.event = "BLAZE ATTACKS. NOVA STARTS THE COMEBACK";
    }
    else if (t < 103)
    {
        frame.event = "THE FINAL SECTOR - EVERY METRE COUNTS";
    }
    else if (t < 112)
    {
        frame.event = "NOVA VS VOLT - SIDE BY SIDE TO THE LINE";
    }
    else
    {
        frame.event = "PHOTO FINISH! NOVA WINS BY A NOSE";
    }
    return frame;
}
void Race::validate() const
{
    float minGap = 1e9f, maxEdge = 0, minSpeed = 1e9f;
    // Separation is checked with a conservative planar rectangle for every car.
    for (int step = 0; step <= 7200; ++step)
    {
        auto frame = evaluate(step / 60.0);
        for (int i = 0; i < 4; ++i)
        {
            auto &a = frame.cars[i];
            minSpeed = std::min(minSpeed, a.speed);
            float edge = std::abs(a.lane) + 1.067f * std::abs(std::cos(a.drift)) +
                         2.263f * std::abs(std::sin(a.drift));
            maxEdge = std::max(maxEdge, edge);
            if (edge > Circuit::halfWidth - .05f)
            {
                throw std::runtime_error("A car leaves the racing surface");
            }
            for (int j = i + 1; j < 4; ++j)
            {
                auto &b = frame.cars[j];
                vec3 d = b.p - a.p;
                float separation = -1e9f;
                vec3 af(-std::sin(a.heading), 0, -std::cos(a.heading)),
                    ar = glm::cross(af, vec3(0, 1, 0));
                vec3 bf(-std::sin(b.heading), 0, -std::cos(b.heading)),
                    br = glm::cross(bf, vec3(0, 1, 0));
                for (vec3 axis : {af, ar, bf, br})
                {
                    float ra = 2.263f * std::abs(glm::dot(af, axis)) +
                               1.067f * std::abs(glm::dot(ar, axis));
                    float rb = 2.263f * std::abs(glm::dot(bf, axis)) +
                               1.067f * std::abs(glm::dot(br, axis));
                    separation = std::max(separation, std::abs(glm::dot(d, axis)) - ra - rb);
                }
                minGap = std::min(minGap, separation);
                if (separation < -.025f)
                {
                    throw std::runtime_error("Choreography collision at " +
                                             std::to_string(step / 60.0) + " s, cars " +
                                             std::to_string(i) + " and " + std::to_string(j));
                }
            }
        }
    }
    if (minSpeed < -.03f)
    {
        throw std::runtime_error("A racer reverses unexpectedly");
    }
    auto a = track.sample(0), b = track.sample(track.length - .001f);
    if (glm::distance(a.p, b.p) > .02f)
    {
        throw std::runtime_error("Circuit seam is not closed");
    }
    std::cout << "VALIDATION OK: 120 s, 4 cars, closed circuit, 7201 samples\nTrack length: "
              << track.length << " m; minimum OBB gap: " << minGap
              << " m; maximum road edge: " << maxEdge << " / " << Circuit::halfWidth << " m\n";
    for (int i = 0; i < 4; ++i)
    {
        std::cout << CARS[i].name << " finish: " << std::fixed << std::setprecision(3)
                  << finishTimes[i] << " s\n";
    }
}
} // namespace apex
