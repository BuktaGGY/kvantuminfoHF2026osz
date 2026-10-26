// TESZTEK: külön futtatható program, amely ellenőrzi a kvantumszámítást.
// Nem nyit ablakot. A kis kerekítési eltéréseket megengedjük a lebegőpontos számítások miatt.

// A kvantumállapotok és kvantumkapuk számításai.
#include "quantum.hpp"
// Eredmények és hibaüzenetek kiírása a terminálba.
#include <iostream>
// Véletlenszerű tesztadatok előállításához kell.
#include <random>
// Így nem kell minden saját matematikai típus elé kiírni, hogy quantum::.
using namespace quantum;
// Az eddig lefuttatott ellenőrzések száma.
int checks = 0;
// Ha egy feltétel nem teljesül, hibát dobunk a teszt megnevezésével.
void require(bool ok, const std::string& name) {
    // Minden ellenőrzést megszámolunk; hamis feltételnél kivétellel jelezzük a hibát.
    ++checks; if (!ok) throw std::runtime_error(name);
}
// Két komplex számot akkor tekintünk egyenlőnek, ha az eltérésük kisebb 10^-10-nél.
bool near(Complex a, Complex b) { return std::abs(a-b)<1e-10; }
// Két állapot mindkét amplitúdóját összehasonlítjuk.
void same(const State& a, const State& b, const std::string& name) {
    // A teszt akkor sikeres, ha alpha és beta is megegyezik a megengedett hibán belül.
    require(near(a[0],b[0]) && near(a[1],b[1]), name);
}
// A program indulásakor ez a függvény fut le.
int main() {
    // Az itt keletkező kivételt a hozzá tartozó catch ág fogja kezelni.
    try {
        // A |0> és |1> alapállapotok: rendre (alpha,beta) = (1,0) és (0,1).
        State zero{1,0}, one{0,1};
        // Ellenőrizzük, hogy X a |0> állapotot |1> állapottá alakítja.
        same(applyGate(matrix({"X"}),zero),one,"X|0> = |1>");
        // Az Y kapu képzetes előjelét is ellenőrizzük: Y|0> = i|1>.
        same(applyGate(matrix({"Y"}),zero),State{0,Complex(0,1)},"Y sign convention");
        // A H kapu |0>-ból az egyenlő szuperpozíciót, |+>-t készíti.
        same(applyGate(matrix({"H"}),zero),fromAngles(90,0),"H|0> = |+>");
        // Az S kapu |+>-ból |+i>-t készít.
        same(applyGate(matrix({"S"}),fromAngles(90,0)),fromAngles(90,90),"S|+> = |+i>");
        // A |0> állapotot 90 fokkal elforgatjuk az X tengely körül.
        auto rx=bloch(applyGate(matrix({"Rx",90}),zero));
        // A pozitív X körüli forgatás a +Z irányt a -Y irányba viszi.
        require(std::abs(rx.y+1)<1e-10,"Rx positive angle points to -y");
        // A |0> állapotot 90 fokkal elforgatjuk az Y tengely körül.
        auto ry=bloch(applyGate(matrix({"Ry",90}),zero));
        // A pozitív Y körüli forgatás a +Z irányt a +X irányba viszi.
        require(std::abs(ry.x-1)<1e-10,"Ry positive angle points to +x");
        // Rögzített kezdőértékű véletlengenerátor: a teszt minden futáskor megismételhető.
        std::mt19937 rng(84); std::uniform_real_distribution<double> u(0,1);
        // Az összes matematikailag támogatott kaput teszteljük, az inverzeket is.
        std::vector<std::string> names={"X","Y","Z","H","S","Sdg","T","Tdg","Rx","Ry","Rz","P"};
        // Száz különböző véletlen kezdőállapottal ismételjük meg a vizsgálatot.
        for(int k=0;k<100;++k) {
            // Theta 0..180, phi 0..360 fokból készül egy véletlen tesztállapot.
            auto s=fromAngles(180*u(rng),360*u(rng));
            // Az adott kezdőállapoton minden kaput külön kipróbálunk.
            for(const auto& n:names) {
                // Véletlen szöget választunk -360..360 fok között, és alkalmazzuk a kaput.
                Gate g{n,720*u(rng)-360}; auto out=applyGate(matrix(g),s);
                // Az összes mérési valószínűség kapuzás után is 1 maradjon.
                require(std::abs(std::norm(out[0])+std::norm(out[1])-1)<1e-10,"Unitarity: "+n);
                // A konjugált transzponált mátrixot képezzük: felcseréljük a sorokat és oszlopokat, és konjugálunk.
                auto m=matrix(g); Matrix adj{std::conj(m[0]),std::conj(m[2]),std::conj(m[1]),std::conj(m[3])};
                // Unitér kapu után az inverzének vissza kell adnia az eredeti amplitúdókat.
                same(applyGate(adj,out),s,"Inverse: "+n);
            }
            // Ellenőrizzük a H-Z-H sorozatot: ugyanazt az eredményt kell adnia, mint az X kapunak.
            same(evolve(s,{{"H"},{"Z"},{"H"}},3),applyGate(matrix({"X"}),s),"HZH = X; sequence order");
            // S és az inverze egymás után nem változtathatja meg az állapotot.
            same(evolve(s,{{"S"},{"Sdg"}},2),s,"S inverse");
            // T és az inverze egymás hatását kioltja.
            same(evolve(s,{{"T"},{"Tdg"}},2),s,"T inverse");
        }
        // A |+i> állapot Bloch-vektora a pozitív Y tengely felé mutasson.
        require(std::abs(bloch(fromAngles(90,90)).y-1)<1e-10,"Bloch y sign");
        // Siker esetén kiírjuk, hány ellenőrzés futott le.
        std::cout << "PASS: " << checks << " quantum checks\n";
    // Az első hibánál kiírjuk a hiba nevét, és 1-es kilépési kóddal jelezzük a sikertelenséget.
    } catch(const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
