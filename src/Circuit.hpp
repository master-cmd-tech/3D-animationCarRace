#pragma once
#include "Common.hpp"
namespace apex
{
struct TrackPoint
{
    float s = 0;
    vec3 p{0}, forward{1, 0, 0}, right{0, 0, 1};
    float curvature = 0;
};
class Circuit
{
  public:
    static constexpr float halfWidth = 7.4f;
    std::vector<TrackPoint> points;
    std::vector<float> referenceTime;
    float length = 0, idealLap = 0;
    Circuit();
    TrackPoint sample(float distance) const;
    float distanceAtTimeFraction(float fraction) const;

  private:
    std::vector<vec3> controls;
    vec3 spline(float u) const;
};
CpuMesh buildEnvironment(const Circuit &track);
CpuMesh buildNumber(int number);
CpuMesh buildStartLights(vec3 color);
} // namespace apex
