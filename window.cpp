// ABLAKKEZELÉS: Windows alatt a Windows API, macOS alatt a GLUT készíti az ablakot.
// Az eseményeket továbbadjuk a főprogramnak; a kvantumállapotokat itt nem módosítjuk.

// Az ablakkezelés és az OpenGL eléréséhez szükséges deklarációk.
#include "window.hpp"
// A min, max és clamp segít az értékeket megfelelő határok között tartani.
#include <algorithm>

// Ez a fajl csak az ablakot es az esemenyek tovabbitasat intezi.
// A kvantumfizika es a rajzolas nem itt talalhato.
// Az ablak rajzterületének kezdő szélessége pixelben.
int windowWidth = 1100;
// Az ablak rajzterületének kezdő magassága pixelben.
int windowHeight = 720;

// Ez az ág csak Windowsra fordításkor kerül a programba.
#ifdef _WIN32
// A Windows rajzolási felületének azonosítója; ehhez kapcsoljuk az OpenGL-t.
HDC device;
// A betűket tároló OpenGL-listák első azonosítója.
GLuint fontBase;

// A kapott szöveget az aktuális OpenGL-színnel és szövegpozíciónál rajzoljuk ki.
void drawText(const std::string& text) {
    // Megadjuk a betűk rajzait tartalmazó listasorozat kezdetét.
    glListBase(fontBase);
    // Minden karakter kódja kiválasztja és kirajzolja a hozzá tartozó betűt.
    glCallLists((GLsizei)text.size(), GL_UNSIGNED_BYTE, text.c_str());
}

// A Windows ezt a függvényt hívja egy ablaküzenettel; w és l az üzenet kiegészítő adatai.
LRESULT CALLBACK events(HWND window, UINT message, WPARAM w, LPARAM l) {
    // Az üzenet típusa alapján választjuk ki a szükséges műveletet.
    switch (message) {
        // Az ablak mérete megváltozott.
        case WM_SIZE:
            // Az üzenet alsó részéből kiolvassuk a szélességet, és legalább 1-re állítjuk.
            windowWidth = std::max(1, (int)LOWORD(l));
            // Az üzenet felső részéből kiolvassuk a magasságot; ez sem lehet nulla.
            windowHeight = std::max(1, (int)HIWORD(l));
            // Sikeres befejezés; eseménykezelőben azt jelzi, hogy az üzenetet kezeltük.
            return 0;
        // Lenyomták a bal egérgombot.
        case WM_LBUTTONDOWN:
            // Húzás közben akkor is ide érkezzenek az egéresemények, ha az egér kimegy az ablakból.
            SetCapture(window);
            // Átadjuk a főprogramnak a lenyomás helyét és tényét.
            mousePressed(GET_X_LPARAM(l), GET_Y_LPARAM(l), true);
            // Sikeres befejezés; eseménykezelőben azt jelzi, hogy az üzenetet kezeltük.
            return 0;
        // Felengedték a bal egérgombot.
        case WM_LBUTTONUP:
            // Megszüntetjük az egér eseményeinek kizárólagos követését.
            ReleaseCapture();
            // Jelezzük a főprogramnak, hogy véget ért a lenyomás.
            mousePressed(GET_X_LPARAM(l), GET_Y_LPARAM(l), false);
            // Sikeres befejezés; eseménykezelőben azt jelzi, hogy az üzenetet kezeltük.
            return 0;
        // Az egér új helyre mozdult.
        case WM_MOUSEMOVE:
            // Az új egérpozíciót átadjuk a főprogramnak.
            mouseMoved(GET_X_LPARAM(l), GET_Y_LPARAM(l));
            // Sikeres befejezés; eseménykezelőben azt jelzi, hogy az üzenetet kezeltük.
            return 0;
        // Egy begépelt karakter érkezett.
        case WM_CHAR:
            // A karakter kódját átadjuk a számbevitel kezeléséhez.
            keyPressed((unsigned char)w);
            // Sikeres befejezés; eseménykezelőben azt jelzi, hogy az üzenetet kezeltük.
            return 0;
        // A felhasználó bezárná az ablakot.
        case WM_CLOSE:
            // Megkérjük a Windowst az ablak megszüntetésére.
            DestroyWindow(window);
            // Sikeres befejezés; eseménykezelőben azt jelzi, hogy az üzenetet kezeltük.
            return 0;
        // Az ablak megszűnt, ezért az alkalmazás eseményciklusa is befejezhető.
        case WM_DESTROY:
            // Kilépési üzenetet teszünk a sorba, sikeres befejezési kóddal.
            PostQuitMessage(0);
            // Sikeres befejezés; eseménykezelőben azt jelzi, hogy az üzenetet kezeltük.
            return 0;
    }
    // A nem kezelt eseményeknél a Windows alapértelmezett viselkedését kérjük.
    return DefWindowProc(window, message, w, l);
}

// Itt állítjuk be az adott rendszer ablakát és az OpenGL megjelenítést.
int openWindow() {
    // Jelezzük, hogy az alkalmazás kezeli a kijelző méretezését, ne nagyítsa fel automatikusan a Windows.
    SetProcessDPIAware();
    // Lekérjük a futó program Windows-azonosítóját.
    HINSTANCE instance = GetModuleHandle(nullptr);
    // Üres leírás az ablak típusához; a kapcsos zárójel lenullázza a mezőket.
    WNDCLASSA type{};
    // Az ablak saját, tartós rajzolási felületet kapjon.
    type.style = CS_OWNDC;
    // Az ablak eseményeit az events függvény kezeli majd.
    type.lpfnWndProc = events;
    // Az ablaktípust ehhez a programhoz rendeljük.
    type.hInstance = instance;
    // A szokásos nyíl alakú egérkurzort választjuk.
    type.hCursor = LoadCursor(nullptr, IDC_ARROW);
    // Belső nevet adunk az ablaktípusnak.
    type.lpszClassName = "SimpleBloch";
    // A típust regisztráljuk a Windowsnál, hogy létrehozhassunk belőle ablakot.
    RegisterClassA(&type);

    // A kívánt belső rajzterület határai.
    RECT size{0, 0, windowWidth, windowHeight};
    // Hozzáadjuk az ablakkeret és címsor méretét a kívánt rajzterülethez.
    AdjustWindowRect(&size, WS_OVERLAPPEDWINDOW, FALSE);
    // Létrehozzuk az ablakot a megadott típussal, címmel és szokásos ablakkerettel.
    HWND window = CreateWindowA("SimpleBloch", "Bloch-gomb", WS_OVERLAPPEDWINDOW,
        // A helyet a Windows válassza meg; a szélességet és magasságot mi adjuk meg.
        CW_USEDEFAULT, CW_USEDEFAULT, size.right-size.left, size.bottom-size.top,
        // Nincs szülőablak, külön menü vagy további indulási adat.
        nullptr, nullptr, instance, nullptr);
    // Lekérjük a létrehozott ablak rajzolási felületét.
    device = GetDC(window);

    // A kívánt képpontformátum leírását alapértékekre állítjuk.
    PIXELFORMATDESCRIPTOR format{};
    // A struktúra méretét a Windows API számára adjuk meg.
    format.nSize = sizeof(format);
    // A képpontformátum-leírás verziója.
    format.nVersion = 1;
    // Ablakba rajzolunk OpenGL-lel; két képpuffert kérünk a villogás elkerüléséhez.
    format.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    // A képpontokat vörös, zöld, kék és alfa összetevőkkel kezeljük.
    format.iPixelType = PFD_TYPE_RGBA;
    // 24 bites színpontosságot kérünk.
    format.cColorBits = 24;
    // 24 bites mélységpuffert kérünk a térbeli takarásokhoz.
    format.cDepthBits = 24;
    // A Windows megkeresi a kért formátumhoz illő megjelenítési módot.
    int pixelFormat = ChoosePixelFormat(device, &format);
    // Ha nem sikerült megfelelő módot kiválasztani vagy beállítani, hibakóddal kilépünk.
    if (!pixelFormat || !SetPixelFormat(device, pixelFormat, &format)) return 1;
    // Létrehozzuk az OpenGL rajzolási környezetét az ablakhoz.
    HGLRC context = wglCreateContext(device);
    // Az új OpenGL-környezetet aktívvá tesszük; hiba esetén kilépünk.
    if (!context || !wglMakeCurrent(device, context)) return 1;

    // Kis, normál vastagságú betűtípust kérünk a feliratokhoz.
    HFONT font = CreateFontA(-13, 8, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        // Karakterkészlet és alapértelmezett megjelenítési pontosság.
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        // Simított, azonos szélességű karaktereket kérünk Consolas betűtípussal.
        ANTIALIASED_QUALITY, FIXED_PITCH, "Consolas");
    // Kiválasztjuk az új betűtípust, és eltesszük a régit a későbbi visszaállításhoz.
    HGDIOBJ oldFont = SelectObject(device, font);
    // Helyet foglalunk 256 karakter OpenGL-rajzának.
    fontBase = glGenLists(256);
    // A Windows betűit OpenGL-ben használható karakterrajzokká alakítjuk.
    wglUseFontBitmapsA(device, 0, 256, fontBase);
    // Láthatóvá tesszük a már elkészített ablakot.
    ShowWindow(window, SW_SHOW);

    // Ebben tároljuk a soron következő Windows-eseményt.
    MSG message{};
    // Amíg igaz, az alkalmazás fő ciklusa tovább fut.
    bool open = true;
    // Az alkalmazás addig dolgozik, amíg nem kérünk kilépést.
    while (open) {
        // Kiolvassuk és eltávolítjuk a sorból az összes várakozó eseményt.
        while (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE)) {
            // Kilépési üzenet esetén leállítjuk a fő ciklust.
            if (message.message == WM_QUIT) open = false;
            // A billentyűeseményekből szükség esetén karakteres üzenetet is készítünk.
            TranslateMessage(&message);
            // Az üzenetet elküldjük az ablak eseménykezelőjének.
            DispatchMessage(&message);
        }
        // Bezárás után már ne kezdjünk új képkockát rajzolni.
        if (open) {
            // A main.cpp megrajzolja az aktuális gömböt és kezelőfelületet.
            draw();
            // Az elkészült háttérképet megjelenítjük az ablakban.
            SwapBuffers(device);
            // Rövid szünetet tartunk, hogy a ciklus ne terhelje feleslegesen a processzort.
            Sleep(16);
        }
    }
    // Felszabadítjuk a karakterek OpenGL-listáit.
    glDeleteLists(fontBase, 256);
    // Visszaállítjuk a korábbi betűtípust a rajzolási felületen.
    SelectObject(device, oldFont);
    // Felszabadítjuk az általunk létrehozott Windows-betűtípust.
    DeleteObject(font);
    // Leválasztjuk az aktív OpenGL-környezetet.
    wglMakeCurrent(nullptr, nullptr);
    // Megszüntetjük a létrehozott OpenGL-környezetet.
    wglDeleteContext(context);
    // Visszaadjuk a Windowsnak az igénybe vett rajzolási felületet.
    ReleaseDC(window, device);
    // Sikeres befejezés; eseménykezelőben azt jelzi, hogy az üzenetet kezeltük.
    return 0;
}
// A Windows helyett a macOS-hez tartozó megoldást használjuk.
#else
// A kapott szöveget az aktuális OpenGL-színnel és szövegpozíciónál rajzoljuk ki.
void drawText(const std::string& text) {
    // A GLUT-tal egyesével kirajzoljuk a szöveg karaktereit, 8×13-as betűkkel.
    for (char letter : text) glutBitmapCharacter(GLUT_BITMAP_8_BY_13, letter);
}

// A GLUT ezt hívja, amikor újra kell rajzolni az ablakot.
void display() {
    // A main.cpp megrajzolja az aktuális gömböt és kezelőfelületet.
    draw();
    // Megjelenítjük a háttérpufferbe megrajzolt képet.
    glutSwapBuffers();
}
// A GLUT átadja az új ablakszélességet és -magasságot.
void resize(int w, int h) {
    // Elmentjük az új szélességet; a későbbi osztások miatt nem lehet nulla.
    windowWidth = std::max(1, w);
    // Elmentjük az új magasságot, legalább 1 értékkel.
    windowHeight = std::max(1, h);
}
// A leütött karaktert kezeljük; az itt nem használt két paraméter az egér helye lenne.
void keyboard(unsigned char key, int, int) {
    // Továbbítjuk a karaktert a főprogram számbeviteléhez.
    keyPressed(key);
    // Kérjük az ablak újrarajzolását, hogy látszódjon a változás.
    glutPostRedisplay();
}
// A GLUT megadja a gomb típusát, állapotát és az egérpozíciót.
void mouse(int button, int state, int x, int y) {
    // Csak a bal egérgombot kezeljük; lenyomáskor true értéket adunk tovább.
    if (button == GLUT_LEFT_BUTTON) mousePressed(x, y, state == GLUT_DOWN);
    // Kérjük az ablak újrarajzolását, hogy látszódjon a változás.
    glutPostRedisplay();
}
// A GLUT ezt hívja, amikor lenyomott egérgomb mellett mozog az egér.
void motion(int x, int y) {
    // Az új pozíciót továbbadjuk a kameraforgatáshoz.
    mouseMoved(x, y);
    // Kérjük az ablak újrarajzolását, hogy látszódjon a változás.
    glutPostRedisplay();
}
// Itt állítjuk be az adott rendszer ablakát és az OpenGL megjelenítést.
int openWindow() {
    // A GLUT-nak egyetlen indulási argumentumot adunk: a program nevét.
    int argc = 1;
    // A program neve módosítható karaktertömbként, ahogy a GLUT várja.
    char name[] = "Bloch-gomb";
    // Az argumentumlista végére nullmutató kerül.
    char* argv[] = {name, nullptr};
    // Elindítjuk és előkészítjük a GLUT könyvtárat.
    glutInit(&argc, argv);
    // Két képpuffert, színes megjelenítést és mélységpuffert kérünk.
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    // Beállítjuk a létrehozandó ablak kezdő méretét.
    glutInitWindowSize(windowWidth, windowHeight);
    // Létrehozzuk az OpenGL-ablakot a megadott címmel.
    glutCreateWindow("Bloch-gomb");
    // Megadjuk, melyik függvény rajzolja újra az ablakot.
    glutDisplayFunc(display);
    // Átméretezéskor a resize függvény fusson le.
    glutReshapeFunc(resize);
    // Billentyűleütéskor a keyboard függvény fusson le.
    glutKeyboardFunc(keyboard);
    // Egérgomb lenyomásakor vagy felengedésekor a mouse függvényt hívja.
    glutMouseFunc(mouse);
    // Egérhúzáskor a motion függvényt hívja.
    glutMotionFunc(motion);
    // Elindul a GLUT eseményciklusa: várja és továbbítja az eseményeket.
    glutMainLoop();
    // Sikeres befejezés; eseménykezelőben azt jelzi, hogy az üzenetet kezeltük.
    return 0;
}
// Itt ér véget az operációs rendszertől függő rész.
#endif
