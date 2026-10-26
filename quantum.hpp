// KVANTUMSZÁMÍTÁS: kezdőállapot, kapumátrixok és Bloch-koordináták.
// Nincs benne rajzolás: ugyanaz a matematika használható a programban és a tesztekben.

// Ezt a fejlécet egy fordítási egységen belül csak egyszer dolgozza fel a fordító.
#pragma once
// Fix méretű tömbök: két amplitúdó, illetve négy mátrixelem tárolásához.
#include <array>
// Matematikai függvények, például szinusz, koszinusz és négyzetgyök.
#include <cmath>
// Komplex számok kezelése: valós és képzetes rész egy változóban.
#include <complex>
// Hibák jelzésére használható kivételtípusok.
#include <stdexcept>
// Szövegek tárolása és összefűzése.
#include <string>
// Változó hosszúságú lista, például a kapusorozat tárolásához.
#include <vector>

// A matematikai részt közös névtérbe fogjuk, hogy ne keveredjen más nevekkel.
namespace quantum {
// A pí értéke állandó; a fok és a radián közötti átváltáshoz is ezt használjuk.
constexpr double pi = 3.14159265358979323846;
// Rövidebb nevet adunk a double pontosságú komplex számnak.
using Complex = std::complex<double>;
// Az allapot ket komplex amplitudo: alpha es beta.
// Egy állapot két komplex amplitúdó: a 0. elem alpha, az 1. elem beta.
using State = std::array<Complex, 2>;
// A 2×2-es mátrixot soronként tároljuk: [m00, m01, m10, m11].
using Matrix = std::array<Complex, 4>; // 2x2-es matrix: m00, m01, m10, m11.
// Egy térbeli pont vagy vektor három koordinátája.
struct Vec3 { double x, y, z; };
// Egy kapu neve és szöge; ha nem adunk szöget, az alapérték 90 fok.
struct Gate { std::string name; double degrees = 90; };

// A ket szogbol a |psi> = cos(theta/2)|0> + exp(i*phi)sin(theta/2)|1> allapot.
// Kezdőállapot készítése két szögből. Az inline miatt fejlécben is definiálható.
inline State fromAngles(double theta, double phi) {
    // Mindkét szöget fokról radiánra váltjuk, mert a trigonometrikus függvények azt várják.
    theta *= pi / 180; phi *= pi / 180;
    // alpha = cos(theta/2); beta hossza sin(theta/2), fázisa phi.
    return {std::cos(theta / 2), std::polar(std::sin(theta / 2), phi)};
}
// Az amplitudokbol a gombon megjelenitett x, y, z koordinatak.
// A komplex amplitúdókat átalakítjuk a Bloch-gömb három valós koordinátájává.
inline Vec3 bloch(const State& s) {
    // A conj megfordítja alpha képzetes részének előjelét, majd megszorozzuk betával.
    const auto ab = std::conj(s[0]) * s[1];
    // x = 2 Re(alpha* beta), y = 2 Im(alpha* beta), z = |alpha|² - |beta|². Itt * konjugálást jelöl.
    return {2 * ab.real(), 2 * ab.imag(), std::norm(s[0]) - std::norm(s[1])};
}
// Minden kapu egy 2x2-es matrix. A forgatasi szoget fokban kapjuk.
// A kapu neve alapján visszaadjuk a hozzá tartozó mátrixot.
inline Matrix matrix(const Gate& g) {
    // A képzetes egység: valós része 0, képzetes része 1, négyzete -1.
    const Complex i(0, 1);
    // A kapuszög radiánban, valamint a fél szög koszinusza és szinusza.
    const double a = g.degrees * pi / 180, c = std::cos(a / 2), s = std::sin(a / 2);
    // Rövid hivatkozás a kapu nevére; nem másoljuk le a szöveget.
    const auto& n = g.name;
    // X kapu: felcseréli alpha és beta értékét.
    if (n == "X") return {0, 1, 1, 0};
    // Y kapu: az új alpha = -i*beta, az új beta = i*alpha.
    if (n == "Y") return {0, -i, i, 0};
    // Z kapu: beta előjelét fordítja meg, alphát változatlanul hagyja.
    if (n == "Z") return {1, 0, 0, -1};
    // Hadamard-kapu: összegből és különbségből képez új amplitúdókat, 1/gyök(2) szorzóval.
    if (n == "H") { const double h = 1 / std::sqrt(2.0); return {h, h, h, -h}; }
    // S kapu: beta fázisát 90 fokkal növeli, vagyis i-vel szorozza.
    if (n == "S") return {1, 0, 0, i};
    // Az S kapu inverze: beta fázisát 90 fokkal csökkenti.
    if (n == "Sdg") return {1, 0, 0, -i};
    // T kapu: beta fázisát 45 fokkal növeli. A polar(1, szög) egységnyi hosszú komplex szám.
    if (n == "T") return {1, 0, 0, std::polar(1.0, pi / 4)};
    // A T kapu inverze: beta fázisát 45 fokkal csökkenti.
    if (n == "Tdg") return {1, 0, 0, std::polar(1.0, -pi / 4)};
    // Forgatás a Bloch-gömb X tengelye körül a megadott szöggel.
    if (n == "Rx") return {c, -i*s, -i*s, c};
    // Forgatás az Y tengely körül; ebben a mátrixban minden elem valós.
    if (n == "Ry") return {c, -s, s, c};
    // Forgatás a Z tengely körül: alpha és beta ellentétes fél szögnyi fázist kap.
    if (n == "Rz") return {std::polar(1.0, -a/2), 0, 0, std::polar(1.0, a/2)};
    // Fáziskapu: csak beta fázisát növeli a megadott szöggel.
    if (n == "P") return {1, 0, 0, std::polar(1.0, a)};
    // Ismeretlen kapunév esetén hibát jelzünk, nem adunk vissza hibás mátrixot.
    throw std::invalid_argument("Unknown gate: " + n);
}
// Matrix-vektor szorzas: ket szorzas es egy osszeadas soronkent.
// A kapu mátrixával megszorozzuk az állapot oszlopvektorát.
inline State applyGate(const Matrix& m, const State& s) {
    // Az első sorból lesz az új alpha, a második sorból az új beta.
    return {m[0]*s[0] + m[1]*s[1], m[2]*s[0] + m[3]*s[1]};
}
// Sorban alkalmazzuk a kapukat az eredeti kezdoallapotra.
// Az állapotot másolatként kapjuk, így az eredeti kezdőállapot nem változik meg.
inline State evolve(State s, const std::vector<Gate>& gates, size_t count) {
    // Nem kérhetünk több lépést, mint ahány kapu szerepel a listában.
    if (count > gates.size()) throw std::out_of_range("Gate count");
    // Az első count kaput balról jobbra, egymás után alkalmazzuk.
    for (size_t k=0; k<count; ++k) s = applyGate(matrix(gates[k]), s);
    // Visszaadjuk a kapusorozat után kapott állapotot.
    return s;
}
}
