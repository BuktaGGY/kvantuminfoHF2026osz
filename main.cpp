// FŐPROGRAM: a felület, a gömb rajzolása és a felhasználói műveletek.
// A matematikai számításokat a quantum.hpp, az ablakkezelést a window.cpp végzi.

// A kvantumállapotok és kvantumkapuk számításai.
#include "quantum.hpp"
// Az ablakkezelés és az OpenGL eléréséhez szükséges deklarációk.
#include "window.hpp"
// A min, max és clamp segít az értékeket megfelelő határok között tartani.
#include <algorithm>
// A kiírt számok tizedesjegyeinek beállítása.
#include <iomanip>
// Számokból formázott szöveget készítünk vele.
#include <sstream>

// Így nem kell minden saját matematikai típus elé kiírni, hogy quantum::.
using namespace quantum;

// A bekapcsolt kezdoallapotokra ugyanazt a kapusorozatot alkalmazzuk.
// A három kezdőállapot polárszöge fokban: a +Z tengelytől mérjük.
double theta[3] = {0, 90, 90};
// A három kezdőállapot körbefordulási szöge az XY síkban, fokban.
double phi[3] = {0, 0, 90};
// A true azt jelenti, hogy az adott állapot részt vesz a közös futtatásban.
bool enabled[3] = {true, true, true};
// Az állapotok RGB-színei; az összetevők 0 és 1 közötti számok.
float colors[3][3] = {{0.2f, 0.9f, 0.7f}, {1, 0.65f, 0.3f}, {0.6f, 0.6f, 1}};
// Annak az állapotnak az indexe, amelyet éppen szerkesztünk. Az indexelés 0-tól indul.
int selected = 0;
// Az ezután hozzáadott forgatási és fáziskapuk szöge.
double gateAngle = 90;
// A kapuk listája a végrehajtás sorrendjében.
std::vector<Gate> gates;
// Ennyi kaput hajtottunk végre; 0 esetén még a kezdőállapotot mutatjuk.
int step = 0;

// A felulet csak egyszeru teglalap gombokat hasznal, muveletazonositokkal.
// Egy gomb helye, mérete, azonosítója és felirata.
struct Button {
    // Bal felső sarok, szélesség és műveletazonosító; a gombmagasság mindenhol 28.
    int x, y, width, id;
    // A gombon látható szöveg.
    std::string label;
};
// Az aktuálisan kirajzolt gombok; kattintáskor ebben a listában keresünk.
std::vector<Button> buttons;
// A felületen választható tíz kapu neve, a gombok sorrendjében.
const char* gateNames[] = {"X", "Y", "Z", "H", "S", "T", "Rx", "Ry", "Rz", "P"};
// A szerkesztett számmező azonosítója; -1 azt jelenti, hogy nincs aktív bevitel.
int editing = -1;
// A felhasználó által éppen begépelt szám szöveges alakja.
std::string input;
// A jobb alsó sarokban megjelenő rövid visszajelzés.
std::string message = "A gombot az egerrel forgathatod.";
// Akkor igaz, ha a felhasználó az egér húzásával forgatja a nézetet.
bool dragging = false;
// Az előző egérpozíció, amelyhez képest az elmozdulást számoljuk.
int lastX, lastY;
// A kamera vízszintes körbefordulása fokban.
double cameraAngle = 40;
// A kamera emelkedési szöge fokban, az XY síkhoz képest.
double cameraHeight = 25;

// Egy számot jól olvasható szöveggé alakítunk.
std::string number(double value) {
    // A kerekítve nullának számító értékeket 0-ra állítjuk, így nem jelenik meg -0.000.
    if (std::abs(value) < 0.0005) value = 0;
    // Ebbe a szöveges kimenetbe írjuk a formázott számot.
    std::ostringstream out;
    // Mindig három tizedesjegyet jelenítünk meg, például 90.000.
    out << std::fixed << std::setprecision(3) << value;
    // A létrejött szöveget adjuk vissza.
    return out.str();
}

// Az aktualis allapot: kezdoallapot + az elso 'step' darab kapu.
// Kiszámítjuk az egyik állapotot az aktuális közös lépésnél.
State currentState(int index) {
    // A tárolt szögekből előállítjuk a kezdőállapot amplitúdóit.
    State state = fromAngles(theta[index], phi[index]);
    // A bekapcsolt állapot step kaput kap; a kihagyott állapotnál nulla kaput alkalmazunk.
    return evolve(state, gates, enabled[index] ? step : 0);
}

// Kétdimenziós felirat rajzolása a megadott helyre.
void text(int x, int y, const std::string& value) {
    // Világosszürke színt állítunk be a szöveghez.
    glColor3f(0.9f, 0.9f, 0.9f);
    // Megadjuk, honnan kezdődjön a szöveg kirajzolása.
    glRasterPos2i(x, y);
    // Az operációs rendszerhez tartozó szövegrajzolót hívjuk meg.
    drawText(value);
}

// Kirajzolunk egy gombot, és eltároljuk az adatait a kattintások felismeréséhez.
void addButton(int x, int y, int width, int id, const std::string& label) {
    // A gomb adatait a lista végére tesszük.
    buttons.push_back({x, y, width, id, label});
    // A gomb hátterének színe.
    glColor3f(0.20f, 0.25f, 0.30f);
    // Négyszöget rajzolunk: utána a négy sarkát adjuk meg.
    glBegin(GL_QUADS);
    // A gomb bal felső sarka.
    glVertex2i(x, y);
    // A gomb jobb felső sarka.
    glVertex2i(x + width, y);
    // A gomb jobb alsó sarka.
    glVertex2i(x + width, y + 28);
    // A gomb bal alsó sarka.
    glVertex2i(x, y + 28);
    // Befejezzük az előző glBegin hívással indított alakzatot.
    glEnd();
    // A feliratot kis belső margóval a gombba írjuk.
    text(x + 8, y + 19, label);
}

// A térbeli gömb, a tengelyek és az állapotvektorok kirajzolása.
void drawSphere() {
    // Az ablak bal oldali 7/11 részét foglalja el a gömb; legalább 1 pixel legyen.
    int viewWidth = std::max(1, windowWidth * 7 / 11);
    // A térbeli rajz csak a bal oldali területre kerüljön.
    glViewport(0, 0, viewWidth, windowHeight);
    // A vetítési mátrixot állítjuk: ez szabja meg, hogyan lesz a 3D képből képernyőkép.
    glMatrixMode(GL_PROJECTION);
    // A kiválasztott mátrixot alaphelyzetbe állítjuk, törölve az előző átalakításokat.
    glLoadIdentity();
    // Perspektivikus nézet: 40 fokos függőleges látószög, ablakarány, közeli és távoli határ.
    gluPerspective(40, double(viewWidth) / windowHeight, 0.1, 20);
    // A kamera és a tárgyak helyzetét leíró mátrixot választjuk ki.
    glMatrixMode(GL_MODELVIEW);
    // A kiválasztott mátrixot alaphelyzetbe állítjuk, törölve az előző átalakításokat.
    glLoadIdentity();
    // A vízszintes kameraszöget radiánra váltjuk.
    double a = cameraAngle * pi / 180;
    // A kamera emelkedési szögét radiánra váltjuk.
    double h = cameraHeight * pi / 180;
    // A kamera 4 egységre van a középponttól, az origóra néz, és a +Z irány van felfelé.
    gluLookAt(4*cos(h)*cos(a), 4*cos(h)*sin(a), 4*sin(h), 0,0,0, 0,0,1);
    // A közelebbi térbeli elemek eltakarhatják a távolabbiakat.
    glEnable(GL_DEPTH_TEST);

    // Szelessegi es hosszusagi korok szakaszokbol.
    // A gömb rácsvonalainak színe.
    glColor3f(0.25f, 0.32f, 0.38f);
    // 30 fokonként rajzolunk szélességi köröket -60 és +60 fok között.
    for (int latitude = -60; latitude <= 60; latitude += 30) {
        // Az adott szélességi kör szöge radiánban.
        double t = latitude * pi / 180;
        // Zárt vonalláncot kezdünk: az utolsó pontot is összeköti az elsővel.
        glBegin(GL_LINE_LOOP);
        // Egy kört száz rövid szakasszal közelítünk.
        for (int i = 0; i < 100; ++i) {
            // A kör aktuális pontjának szöge: egy teljes kör 2*pi radián.
            double p = i * 2 * pi / 100;
            // Gömbi koordinátákból kiszámítjuk és megadjuk a térbeli x, y, z pontot.
            glVertex3d(cos(t)*cos(p), cos(t)*sin(p), sin(t));
        }
        // Befejezzük az előző glBegin hívással indított alakzatot.
        glEnd();
    }
    // 30 fokonként választunk egy új meridiánsíkot; minden ciklus egy teljes kört rajzol.
    for (int longitude = 0; longitude < 180; longitude += 30) {
        // A meridiánsík iránya radiánban.
        double p = longitude * pi / 180;
        // Zárt vonalláncot kezdünk: az utolsó pontot is összeköti az elsővel.
        glBegin(GL_LINE_LOOP);
        // Egy kört száz rövid szakasszal közelítünk.
        for (int i = 0; i < 100; ++i) {
            // A meridián körének következő pontjához tartozó szög.
            double t = i * 2 * pi / 100;
            // Gömbi koordinátákból kiszámítjuk és megadjuk a térbeli x, y, z pontot.
            glVertex3d(cos(t)*cos(p), cos(t)*sin(p), sin(t));
        }
        // Befejezzük az előző glBegin hívással indított alakzatot.
        glEnd();
    }

    // Koordinatatengelyek es a pozitiv iranyok feliratai.
    // A tengelyek és a tengelyfeliratok színe.
    glColor3f(0.6f, 0.6f, 0.6f);
    // Szakaszokat rajzolunk; két egymást követő csúcspont ad egy szakaszt.
    glBegin(GL_LINES);
    // Az X tengely két végpontja.
    glVertex3d(-1.2,0,0); glVertex3d(1.2,0,0);
    // Az Y tengely két végpontja.
    glVertex3d(0,-1.2,0); glVertex3d(0,1.2,0);
    // A Z tengely két végpontja.
    glVertex3d(0,0,-1.2); glVertex3d(0,0,1.2);
    // Befejezzük az előző glBegin hívással indított alakzatot.
    glEnd();
    // Felirat a pozitív X tengely végén; ebben az irányban van a |+> állapot.
    glRasterPos3d(1.25,0,0); drawText("X / |+>");
    // Felirat a pozitív Y tengelynél; ez a |+i> állapot iránya.
    glRasterPos3d(0,1.25,0); drawText("Y / |+i>");
    // A gömb északi pólusa a |0> állapot.
    glRasterPos3d(0,0,1.25); drawText("Z / |0>");
    // A gömb déli pólusa a |1> állapot.
    glRasterPos3d(0,0,-1.25); drawText("|1>");

    // Minden allapot egy szines szakasz, a vegen egy ponttal.
    // Végigmegyünk a három állapoton, 0-s, 1-es és 2-es indexszel.
    for (int i = 0; i < 3; ++i) {
        // A kikapcsolt állapotot nem rajzoljuk ki, továbblépünk a következőre.
        if (!enabled[i]) continue;
        // Kiszámítjuk az aktuális állapot Bloch-vektorát.
        Vec3 v = bloch(currentState(i));
        // Az adott állapothoz tartozó színt használjuk.
        glColor3fv(colors[i]);
        // A szerkesztett állapot vonala vastagabb, így könnyebb felismerni.
        glLineWidth(i == selected ? 4 : 2);
        // Szakaszokat rajzolunk; két egymást követő csúcspont ad egy szakaszt.
        glBegin(GL_LINES);
        // A vektor kezdőpontja az origó, a gömb középpontja.
        glVertex3d(0,0,0);
        // Az állapotvektor végpontját adjuk meg.
        glVertex3d(v.x, v.y, v.z);
        // Befejezzük az előző glBegin hívással indított alakzatot.
        glEnd();
        // A szerkesztett állapot végpontja nagyobb pontként jelenik meg.
        glPointSize(i == selected ? 11 : 7);
        // Most különálló pontokat rajzolunk, nem vonalakat.
        glBegin(GL_POINTS);
        // Az állapotvektor végpontját adjuk meg.
        glVertex3d(v.x, v.y, v.z);
        // Befejezzük az előző glBegin hívással indított alakzatot.
        glEnd();
    }
    // Visszaállítjuk az alap vonalvastagságot a további rajzoláshoz.
    glLineWidth(1);
    // A 2D feliratokat már ne takarhassák el a térbeli elemek.
    glDisable(GL_DEPTH_TEST);
}

// A gombok, számmezők, kapulista és eredmények kirajzolása.
void drawPanel() {
    // A felület az egész ablakot használhatja.
    glViewport(0, 0, windowWidth, windowHeight);
    // A vetítési mátrixot állítjuk: ez szabja meg, hogyan lesz a 3D képből képernyőkép.
    glMatrixMode(GL_PROJECTION);
    // A kiválasztott mátrixot alaphelyzetbe állítjuk, törölve az előző átalakításokat.
    glLoadIdentity();
    // Síkbeli koordináták: bal felül (0,0), jobb alul (1100,720), perspektíva nélkül.
    glOrtho(0, 1100, 720, 0, -1, 1);
    // A kamera és a tárgyak helyzetét leíró mátrixot választjuk ki.
    glMatrixMode(GL_MODELVIEW);
    // A kiválasztott mátrixot alaphelyzetbe állítjuk, törölve az előző átalakításokat.
    glLoadIdentity();
    // Újrarajzolás előtt töröljük a korábbi gomblistát, hogy ne legyenek benne ismétlések.
    buttons.clear();

    // Az alkalmazás címe a bal felső sarokban.
    text(24, 35, "BLOCH-GOMB SZIMULATOR");
    // Színmagyarázat, hogy felismerhetők legyenek az állapotok.
    text(24, 59, "Zold: 1. / Narancs: 2. / Lila: 3. allapot");
    // Kiírjuk, melyik állapot szögeit szerkesztjük; a kijelzés 1-től számoz.
    text(720, 35, "SZERKESZTETT ALLAPOT: " + std::to_string(selected + 1));
    // Végigmegyünk a három állapoton, 0-s, 1-es és 2-es indexszel.
    for (int i = 0; i < 3; ++i) {
        // Három gombbal választható ki, melyik állapotot szerkesszük.
        addButton(720 + i*120, 50, 110, i, "Allapot " + std::to_string(i+1));
        // Külön kapcsoló mutatja, részt vesz-e ez az állapot a futtatásban.
        addButton(720 + i*120, 84, 110, 60+i, enabled[i] ? "[x] Futtat" : "[ ] Kihagy");
    }

    // A kezdőállapot szögmezőinek felirata.
    text(720, 134, "Kezdoallapot (fok):");
    // Theta számmező: kattintásra a 10-es művelet indítja a bevitelt.
    addButton(720, 145, 350, 10, "theta = " + number(theta[selected]));
    // Phi számmező: a 11-es művelethez tartozik.
    addButton(720, 180, 350, 11, "phi   = " + number(phi[selected]));
    // Gyorsgomb a |0> kezdőállapothoz.
    addButton(720, 215, 110, 50, "|0>");
    // Gyorsgomb a |1> kezdőállapothoz.
    addButton(840, 215, 110, 51, "|1>");
    // Gyorsgomb a |+> kezdőállapothoz.
    addButton(960, 215, 110, 52, "|+>");
    // Rövid segítség a számok módosításához.
    text(720, 263, "Szam modositas: kattintas, ertek, Enter.");
    // A jövőben hozzáadott forgatási és fáziskapuk szögének mezője.
    addButton(720, 275, 350, 12, "Kapuszog = " + number(gateAngle));
    // Szerkesztés közben a begépelt, még el nem mentett számot mutatjuk.
    if (editing >= 0) text(720, 325, "Uj ertek: " + input + "_");
    // Ha nincs bevitel, a megengedett kezdőszögeket mutatjuk.
    else text(720, 325, "theta: 0..180 / phi: 0..360");

    // Jelezzük, hogy a kapugombok először csak összeállítják a sorozatot.
    text(720, 354, "1. KAPUK HOZZAADASA A SOROZATHOZ:");
    // Végigmegyünk a tíz választható kapun.
    for (int i = 0; i < 10; ++i)
        // Öt gomb fér egy sorba: i%5 az oszlop, i/5 a sor; az azonosító 20+i.
        addButton(720 + (i%5)*72, 365 + (i/5)*35, 64, 20+i, gateNames[i]);
    // Visszalépés egy kapuval az összes aktív állapoton.
    addButton(720, 440, 110, 40, "Elozo");
    // Egy következő kapu végrehajtása az összes aktív állapoton.
    addButton(840, 440, 110, 41, "Kovetkezo");
    // A teljes kapusorozat törlésének gombja.
    addButton(960, 440, 110, 42, "Torles");
    // A teljes összeállított sorozat végrehajtásának gombja.
    addButton(720, 478, 170, 43, "2. Futtatas");
    // Visszatérés a kezdőállapotokhoz a kapulista megtartásával.
    addButton(900, 478, 170, 44, "Kezdoallapotok");
    // A már alkalmazott kapuk száma és a sorozat teljes hossza.
    text(720, 533, "KOZOS LEPES: " + std::to_string(step) + " / " + std::to_string(gates.size()));

    // Ot kapu latszik az aktualis lepes kozeleben.
    // A kapulista az aktuális lépés közelében kezdődik, de az index nem lehet negatív.
    int first = std::max(0, step - 2);
    // Legfeljebb öt kaput írunk ki, és nem lépünk túl a lista végén.
    for (int i = first; i < std::min(first+5, int(gates.size())); ++i) {
        // A kapu sorszáma és neve kerül a sor elejére; a kiírt sorszám 1-től indul.
        std::string label = std::to_string(i+1) + ". " + gates[i].name;
        // Az Rx, Ry, Rz és P kapunál a szöget is megjelenítjük.
        if (gates[i].name.size() == 2 || gates[i].name == "P")
            // A kapunév mögé zárójelben odaírjuk a tárolt szöget.
            label += "(" + number(gates[i].degrees) + ")";
        // A már végrehajtott kapu kész, a többi még várakozik.
        label += i < step ? "  [kesz]" : "  [var]";
        // A kapu sorát kiírjuk; két sor között 24 egység távolság van.
        text(720, 557 + (i-first)*24, label);
    }
    // Üres lista esetén magyarázó szöveg jelenik meg a kapuk helyén.
    if (gates.empty()) text(720, 557, "Meg nincs kapu a sorozatban.");

    // A szerkesztett állapot aktuális amplitúdóit kérjük le az alsó kijelzéshez.
    State state = currentState(selected);
    // Alpha valós és képzetes részét külön írjuk ki.
    text(24, 635, "alpha: " + number(state[0].real()) + " + (" + number(state[0].imag()) + ")i");
    // Beta valós és képzetes részét külön írjuk ki.
    text(24, 658, "beta:  " + number(state[1].real()) + " + (" + number(state[1].imag()) + ")i");
    // A Z bázisban mért 0 és 1 valószínűsége az amplitúdók abszolútértékének négyzete.
    text(24, 686, "P(0) = " + number(std::norm(state[0])) + "   P(1) = " + number(std::norm(state[1])));
    // A legutóbbi művelet visszajelzése a jobb alsó sarokban.
    text(720, 690, message);
}

// Egy teljes képkocka megrajzolása: háttértörlés, majd gömb és felület.
void draw() {
    // A háttér sötét színe; a negyedik érték az átlátszatlanság.
    glClearColor(0.07f, 0.09f, 0.12f, 1);
    // Töröljük az előző képet és az előző képhez tartozó mélységadatokat.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // Először a térbeli részt rajzoljuk meg.
    drawSphere();
    // Utána a felületet rajzoljuk rá, hogy olvasható maradjon.
    drawPanel();
}

// A szambevitel csak a harom szog mezohoz kell.
// Billentyűleütés kezelése a számmező szerkesztése közben.
void keyPressed(unsigned char key) {
    // Ha nincs aktív számmező, nem kell feldolgozni a billentyűt.
    if (editing < 0) return;
    // Az Escape megszakítja a szerkesztést, az érték nem változik.
    if (key == 27) { editing = -1; return; } // Escape: megse
    // A törléshez tartozó billentyűk kezelése.
    if (key == 8 || key == 127) {
        // Ha van már begépelt szöveg, levesszük az utolsó karakterét.
        if (!input.empty()) input.pop_back();
    // Enter esetén megpróbáljuk elmenteni a beírt számot.
    } else if (key == 13) {
        // Az itt keletkező kivételt a hozzá tartozó catch ág fogja kezelni.
        try {
            // Ide kerül, hogy a számolvasás hány karaktert tudott feldolgozni.
            size_t used;
            // A begépelt szöveget double típusú számmá alakítjuk.
            double value = std::stod(input, &used);
            // Csak teljesen értelmezhető, véges számot fogadunk el.
            if (used != input.size() || !std::isfinite(value)) throw 0;
            // A theta szöget a megengedett 0 és 180 fok közé szorítjuk.
            if (editing == 10) theta[selected] = std::clamp(value, 0.0, 180.0);
            // A phi szöget 0 és 360 fok között tartjuk.
            if (editing == 11) phi[selected] = std::clamp(value, 0.0, 360.0);
            // A később hozzáadott kapuk szöge -360 és +360 fok közötti lehet.
            if (editing == 12) gateAngle = std::clamp(value, -360.0, 360.0);
            // Új kezdőállapot megadásakor minden résztvevő visszatér a nulladik lépéshez.
            if (editing == 10 || editing == 11) step = 0;
            // Visszajelezzük a sikeres számbevitelt.
            message = "Ertek beallitva.";
        // Ha a bevitel feldolgozása hibás volt, nem áll le a program, hanem üzenetet írunk.
        } catch (...) {
            // Jelezzük, hogy a beírt szöveg nem fogadható el számként.
            message = "Ervenytelen szam.";
        }
        // Befejezzük vagy megszakítjuk az aktuális számmező szerkesztését.
        editing = -1;
    // Csak számjegy, tizedespont és mínuszjel kerülhet a beviteli szövegbe.
    } else if ((key >= '0' && key <= '9') || key == '.' || key == '-') {
        // Legfeljebb 12 karaktert tárolunk, hogy a bevitel elférjen a felületen.
        if (input.size() < 12) input += key;
    }
}

// A gomb azonosítója alapján eldöntjük, milyen műveletet kell végrehajtani.
void buttonPressed(int id) {
    // Befejezzük vagy megszakítjuk az aktuális számmező szerkesztését.
    editing = -1;
    // A 0, 1, 2 azonosítójú gombok a szerkesztett állapotot választják ki.
    if (id < 3) selected = id;
    // A 10, 11, 12 számmező; új, üres bevitelt kezdünk.
    if (id >= 10 && id <= 12) { editing = id; input.clear(); }
    // A 20 és 29 közötti azonosítók a tíz kapugombhoz tartoznak.
    if (id >= 20 && id < 30) {
        // Száz kapunál megállítjuk a lista bővítését, és ezt jelezzük is.
        if (gates.size() >= 100) { message = "Legfeljebb 100 kapu lehet."; return; }
        // A lista végére mentjük a kapu nevét és az éppen beállított szöget.
        gates.push_back({gateNames[id-20], gateAngle});
        // A közös futtatást visszaállítjuk a kezdőállapotokra.
        step = 0; // A szerkesztett sorozatot kulon inditjuk el.
        // Csak a sorozat bővült; a végrehajtást a felhasználó külön indítja.
        message = "Kapu hozzaadva. Inditsd a futtatast.";
    }
    // Visszalépünk egy kapuval, de a lépésszám nem lehet negatív.
    if (id == 40 && step > 0) --step;
    // Előrelépünk egy kapuval, ha van még végrehajtatlan kapu.
    if (id == 41 && step < int(gates.size())) ++step;
    // A törlés kiüríti a kapulistát, és visszaállítja a kezdőállapotokat.
    if (id == 42) { gates.clear(); step = 0; message = "Kapusorozat torolve."; }
    // A Futtatás gomb a sorozat végére ugrik.
    if (id == 43) {
        // Az összes felvett kaput alkalmazzuk az aktív állapotokra.
        step = int(gates.size());
        // Üres lista esetén kapukat kérünk, különben jelezzük a kész futtatást.
        message = gates.empty() ? "Elobb adj hozza kapukat." : "A kijelolt allapotok futtatasa kesz.";
    }
    // A kezdőállapotokat mutatjuk újra, de a kapulistát megtartjuk.
    if (id == 44) { step = 0; message = "Vissza a kezdoallapotokhoz."; }
    // A három gyorsgomb a |0>, |1> vagy |+> kezdőállapotot állítja be.
    if (id >= 50 && id <= 52) {
        // A három állapothoz rendre 0, 180 és 90 fokos theta tartozik.
        theta[selected] = id == 50 ? 0 : (id == 51 ? 180 : 90);
        // Ezeknél az előre megadott állapotoknál nullára állítjuk a phi szöget.
        phi[selected] = 0;
        // A közös futtatást visszaállítjuk a kezdőállapotokra.
        step = 0;
        // Visszajelzés a gyorsgombbal végzett állapotbeállításról.
        message = "Kezdoallapot beallitva.";
    }
    // Az állapotok közös futtatásban való részvételét kapcsoljuk be vagy ki.
    if (id >= 60 && id <= 62) {
        // A gombazonosítóból visszakapjuk a 0, 1 vagy 2 állapotindexet.
        int index = id - 60;
        // Összeszámoljuk az aktív állapotokat: a true értéke 1, a false értéke 0.
        int count = int(enabled[0]) + int(enabled[1]) + int(enabled[2]);
        // Az egyetlen megmaradt aktív állapotot nem engedjük kikapcsolni.
        if (enabled[index] && count == 1) {
            // Elmagyarázzuk, miért maradt bekapcsolva az utolsó állapot.
            message = "Legalabb egy allapot maradjon aktiv.";
            // Kilépünk a függvényből; ezt az eseményt már kezeltük.
            return;
        }
        // A logikai tagadás felcseréli a bekapcsolt és a kikapcsolt értéket.
        enabled[index] = !enabled[index];
        // A közös futtatást visszaállítjuk a kezdőállapotokra.
        step = 0;
        // A résztvevők módosítása után újra lehet futtatni a sorozatot.
        message = "Kijeloles modositva. Ujraindithato.";
    }
}

// A bal egérgomb lenyomásának vagy felengedésének kezelése.
void mousePressed(int x, int y, bool down) {
    // Először leállítjuk az esetleges korábbi kameraforgatást.
    dragging = false;
    // Felengedéskor nincs új kattintási művelet; a húzás már leállt.
    if (!down) return;
    // Atvaltunk a felulet 1100 x 720-as koordinataira.
    // Az egér vízszintes helyét a felület saját, 1100 egység széles rendszerébe váltjuk.
    x = x * 1100 / windowWidth;
    // Az egér függőleges helyét a felület saját, 720 egység magas rendszerébe váltjuk.
    y = y * 720 / windowHeight;
    // Végignézzük a kirajzolt gombokat, hogy melyikre kattintottak.
    for (Button button : buttons) {
        // Megnézzük, hogy az egérpont a gomb téglalapján belül van-e.
        if (x >= button.x && x <= button.x+button.width && y >= button.y && y <= button.y+28) {
            // Végrehajtjuk a megtalált gombhoz tartozó műveletet.
            buttonPressed(button.id);
            // Kilépünk a függvényből; ezt az eseményt már kezeltük.
            return;
        }
    }
    // A gömb területén lenyomott egérgomb forgatást kezd; eltároljuk a kiinduló egérpozíciót.
    if (x < 700) { dragging = true; lastX = x; lastY = y; }
}

// Egérmozgáskor a kamera irányát változtatjuk, ha éppen húzzák a gömböt.
void mouseMoved(int x, int y) {
    // Lenyomott egérgomb nélkül nem forgatjuk a nézetet.
    if (!dragging) return;
    // Az egér vízszintes helyét a felület saját, 1100 egység széles rendszerébe váltjuk.
    x = x * 1100 / windowWidth;
    // Az egér függőleges helyét a felület saját, 720 egység magas rendszerébe váltjuk.
    y = y * 720 / windowHeight;
    // A vízszintes elmozdulás mértékével arányosan körbefordul a kamera.
    cameraAngle -= (x-lastX) * 0.5;
    // A függőleges elmozdulás emeli vagy süllyeszti a kamerát; a pólusok előtt megállítjuk.
    cameraHeight = std::clamp(cameraHeight+(y-lastY)*0.5, -85.0, 85.0);
    // Eltároljuk az új vízszintes egérpozíciót a következő elmozduláshoz.
    lastX = x;
    // Eltároljuk az új függőleges egérpozíciót is.
    lastY = y;
}

// A program indulásakor ez a függvény fut le.
int main() {
    // Létrehozzuk az ablakot és elindítjuk az eseménykezelést.
    return openWindow();
}
