// KÖZÖS ABLAKKEZELŐ FELÜLET: a főprogram ezeken a függvényeken keresztül használja az ablakot.
// A deklaráció megadja egy függvény nevét és paramétereit; a megvalósítás a .cpp fájlban van.

// Ezt a fejlécet egy fordítási egységen belül csak egyszer dolgozza fel a fordító.
#pragma once

// Az OpenGL fejlec eleresi utja operacios rendszerenkent elter.
// Ez az ág csak Windowsra fordításkor kerül a programba.
#ifdef _WIN32
// Megakadályozzuk, hogy a Windows min/max makrói ütközzenek a C++ függvényekkel.
#define NOMINMAX
// A Windows ablakokhoz és rendszerhívásokhoz szükséges típusok és függvények.
#include <windows.h>
// Segédek az egérpozíció kiolvasásához a Windows üzeneteiből.
#include <windowsx.h>
// Az OpenGL alapvető rajzolófüggvényei Windowson.
#include <GL/gl.h>
// OpenGL segédfüggvények, például a perspektíva és a kamera beállítása.
#include <GL/glu.h>
// A Windows helyett a macOS-hez tartozó megoldást használjuk.
#else
// A macOS elavultnak jelöli az OpenGL-t; az erről szóló fordítási figyelmeztetést elrejtjük.
#define GL_SILENCE_DEPRECATION
// A macOS GLUT könyvtára ablakkezelést, billentyű- és egéreseményeket biztosít.
#include <GLUT/glut.h>
// Itt ér véget az operációs rendszertől függő rész.
#endif
// Szövegek tárolása és összefűzése.
#include <string>

// Az ablak szélessége; az extern jelzi, hogy a változó a window.cpp fájlban jön létre.
extern int windowWidth;
// Az ablak magassága; ezt is a window.cpp tárolja és frissíti.
extern int windowHeight;

// Ezeket a main.cpp valositja meg.
// A főprogram egy teljes képkockát rajzoló függvényének előzetes deklarációja.
void draw();
// Egy leütött karaktert átadunk a főprogramnak.
void keyPressed(unsigned char key);
// Egérpozíció és a bal egérgomb állapota: down=true lenyomáskor.
void mousePressed(int x, int y, bool down);
// Az egér új helyét átadjuk a kameraforgatáshoz.
void mouseMoved(int x, int y);

// Ezt a ket fuggvenyt az operacios rendszerhez tartozo resz adja.
// Szöveg rajzolása az OpenGL-ben korábban beállított helyre.
void drawText(const std::string& text);
// Az ablak létrehozása; visszatéréskor a 0 sikeres befejezést jelent.
int openWindow();
