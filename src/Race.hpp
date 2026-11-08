#pragma once
#include "Circuit.hpp"
namespace apex
{
struct CarInfo
{
    const char *name;
    vec3 paint;
    int number;
};
inline const std::array<CarInfo, 4> CARS = {{{"NOVA", {.93f, .64f, .16f}, 7},
                                             {"VOLT", {.04f, .64f, .78f}, 22},
                                             {"BLAZE", {.85f, .075f, .08f}, 11},
                                             {"PIXEL", {.49f, .19f, .76f}, 63}}};
struct CarPose
{
    vec3 p{0};
    vec3 forward{0, 0, -1};
    vec3 right{1, 0, 0};
    float progress = 0;
    float lane = 0;
    float speed = 0;
    float heading = 0;
    float roll = 0;
    float drift = 0;
    float steer = 0;
    float spin = 0;
    mat4 model{1};
};
struct RaceFrame
{
    std::array<CarPose, 4> cars;
    std::array<int, 4> order{0, 1, 2, 3};
    std::string event;
};
class Race
{
  public:
    const Circuit &track;
    std::array<double, 4> finishTimes{};
    explicit Race(const Circuit &t);
    RaceFrame evaluate(double seconds, bool accents = true) const;
    void validate() const;

  private:
    float progress(int car, float q) const;
    float base(float q) const;
    float offset(int car, float q) const;
    float lane(int car, float q) const;
};
} // namespace apex
