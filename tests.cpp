#include "quantum.hpp"
#include <iostream>
#include <random>
using namespace quantum;
int checks = 0;
void require(bool ok, const std::string& name) {
    ++checks; if (!ok) throw std::runtime_error(name);
}
bool near(Complex a, Complex b) { return std::abs(a-b)<1e-10; }
void same(const State& a, const State& b, const std::string& name) {
    require(near(a[0],b[0]) && near(a[1],b[1]), name);
}
int main() {
    try {
        State zero{1,0}, one{0,1};
        same(applyGate(matrix({"X"}),zero),one,"X|0> = |1>");
        same(applyGate(matrix({"Y"}),zero),State{0,Complex(0,1)},"Y sign convention");
        same(applyGate(matrix({"H"}),zero),fromAngles(90,0),"H|0> = |+>");
        same(applyGate(matrix({"S"}),fromAngles(90,0)),fromAngles(90,90),"S|+> = |+i>");
        auto rx=bloch(applyGate(matrix({"Rx",90}),zero));
        require(std::abs(rx.y+1)<1e-10,"Rx positive angle points to -y");
        auto ry=bloch(applyGate(matrix({"Ry",90}),zero));
        require(std::abs(ry.x-1)<1e-10,"Ry positive angle points to +x");
        std::mt19937 rng(84); std::uniform_real_distribution<double> u(0,1);
        std::vector<std::string> names={"X","Y","Z","H","S","Sdg","T","Tdg","Rx","Ry","Rz","P"};
        for(int k=0;k<100;++k) {
            auto s=fromAngles(180*u(rng),360*u(rng));
            for(const auto& n:names) {
                Gate g{n,720*u(rng)-360}; auto out=applyGate(matrix(g),s);
                require(std::abs(std::norm(out[0])+std::norm(out[1])-1)<1e-10,"Unitarity: "+n);
                auto v=bloch(out), p=pathPoint(bloch(s),g,1);
                require(std::abs(v.x-p.x)+std::abs(v.y-p.y)+std::abs(v.z-p.z)<1e-9,"Path endpoint: "+n);
                auto m=matrix(g); Matrix adj{std::conj(m[0]),std::conj(m[2]),std::conj(m[1]),std::conj(m[3])};
                same(applyGate(adj,out),s,"Inverse: "+n);
            }
            same(evolve(s,{{"H"},{"Z"},{"H"}},3),applyGate(matrix({"X"}),s),"HZH = X; sequence order");
            same(evolve(s,{{"S"},{"Sdg"}},2),s,"S inverse");
            same(evolve(s,{{"T"},{"Tdg"}},2),s,"T inverse");
        }
        require(std::abs(bloch(fromAngles(90,90)).y-1)<1e-10,"Bloch y sign");
        std::cout << "PASS: " << checks << " quantum checks\n";
    } catch(const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
