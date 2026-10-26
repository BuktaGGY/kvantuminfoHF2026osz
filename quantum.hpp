#pragma once
#include <array>
#include <cmath>
#include <complex>
#include <stdexcept>
#include <string>
#include <vector>

namespace quantum {
constexpr double pi = 3.14159265358979323846;
using Complex = std::complex<double>;
using State = std::array<Complex, 2>;
using Matrix = std::array<Complex, 4>; // Row-major, column state vectors.
struct Vec3 { double x, y, z; };
struct Gate { std::string name; double degrees = 90; };
inline State fromAngles(double theta, double phi) {
    theta *= pi / 180; phi *= pi / 180;
    return {std::cos(theta / 2), std::polar(std::sin(theta / 2), phi)};
}
inline Vec3 bloch(const State& s) {
    const auto ab = std::conj(s[0]) * s[1];
    return {2 * ab.real(), 2 * ab.imag(), std::norm(s[0]) - std::norm(s[1])};
}
inline Matrix matrix(const Gate& g) {
    const Complex i(0, 1);
    const double a = g.degrees * pi / 180, c = std::cos(a / 2), s = std::sin(a / 2);
    const auto& n = g.name;
    if (n == "X") return {0, 1, 1, 0};
    if (n == "Y") return {0, -i, i, 0};
    if (n == "Z") return {1, 0, 0, -1};
    if (n == "H") { const double h = 1 / std::sqrt(2.0); return {h, h, h, -h}; }
    if (n == "S") return {1, 0, 0, i};
    if (n == "Sdg") return {1, 0, 0, -i};
    if (n == "T") return {1, 0, 0, std::polar(1.0, pi / 4)};
    if (n == "Tdg") return {1, 0, 0, std::polar(1.0, -pi / 4)};
    if (n == "Rx") return {c, -i*s, -i*s, c};
    if (n == "Ry") return {c, -s, s, c};
    if (n == "Rz") return {std::polar(1.0, -a/2), 0, 0, std::polar(1.0, a/2)};
    if (n == "P") return {1, 0, 0, std::polar(1.0, a)};
    throw std::invalid_argument("Unknown gate: " + n);
}
inline State applyGate(const Matrix& m, const State& s) {
    return {m[0]*s[0] + m[1]*s[1], m[2]*s[0] + m[3]*s[1]};
}
inline State evolve(State s, const std::vector<Gate>& gates, size_t count) {
    if (count > gates.size()) throw std::out_of_range("Gate count");
    for (size_t k=0; k<count; ++k) s = applyGate(matrix(gates[k]), s);
    return s;
}
// Continuous path on the Bloch sphere. Global phase has no Bloch representation.
inline Vec3 pathPoint(Vec3 v, const Gate& g, double t) {
    Vec3 axis{0,0,1}; double a = g.degrees * pi / 180;
    if (g.name == "X" || g.name == "Rx") axis = {1,0,0};
    if (g.name == "Y" || g.name == "Ry") axis = {0,1,0};
    if (g.name == "X" || g.name == "Y" || g.name == "Z" || g.name == "H") a = pi;
    if (g.name == "H") axis = {1/std::sqrt(2.0),0,1/std::sqrt(2.0)};
    if (g.name == "S" || g.name == "Sdg") a = (g.name == "S" ? 1 : -1)*pi/2;
    if (g.name == "T" || g.name == "Tdg") a = (g.name == "T" ? 1 : -1)*pi/4;
    const double c=std::cos(a*t), s=std::sin(a*t), d=axis.x*v.x+axis.y*v.y+axis.z*v.z;
    return {v.x*c+(axis.y*v.z-axis.z*v.y)*s+axis.x*d*(1-c),
            v.y*c+(axis.z*v.x-axis.x*v.z)*s+axis.y*d*(1-c),
            v.z*c+(axis.x*v.y-axis.y*v.x)*s+axis.z*d*(1-c)};
}
}
