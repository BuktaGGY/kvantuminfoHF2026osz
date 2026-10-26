#include "quantum.hpp"
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <GL/gl.h>
#include <GL/glu.h>
#else
#define GL_SILENCE_DEPRECATION
#include <GLUT/glut.h>
#endif
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iomanip>
#include <sstream>

using namespace quantum;
struct Color { float r,g,b; };
const Color ink{.87f,.91f,.98f}, muted{.49f,.56f,.68f}, teal{.32f,.88f,.77f};
const Color palette[]={{.32f,.88f,.77f},{1,.66f,.36f},{.68f,.53f,1},{.98f,.44f,.59f},{.36f,.7f,1},{.92f,.85f,.37f}};
struct Track { double theta,phi; int color; };
struct Hit { float x,y,w,h; std::function<void()> action; };
std::vector<Track> tracks={{0,0,0}};
std::vector<Gate> gates;
std::vector<Hit> hits;
int width=1440,height=900,selected=0,step=0,scroll=0,activeField=-1,slider=-1;
int pixelWidth=1440,pixelHeight=900;
double theta=0,phi=0,angle=90,yaw=40,elevation=23,distance=4.4;
bool running=false,dragging=false,showPaths=true,showInitial=true,freshInput=false;
int lastX=0,lastY=0;
std::string input,status="Valassz kezdoallapotot, majd epits kapusorozatot.",snapshot;
auto lastTick=std::chrono::steady_clock::now();
#ifdef _WIN32
HDC device; GLuint fontBase;
#endif
constexpr float leftW=286,rightW=302;
std::string fixed(double d,int n=3) { if(std::abs(d)<.5*std::pow(10,-n))d=0; std::ostringstream s;s<<std::fixed<<std::setprecision(n)<<d;return s.str(); }
std::string complexText(Complex z) { return fixed(z.real())+(z.imag()<-.0005?" - ":" + ")+fixed(std::abs(z.imag()))+"i"; }
void color(Color c,float alpha=1){glColor4f(c.r,c.g,c.b,alpha);}
void rect(float x,float y,float w,float h,Color c){ color(c);glBegin(GL_QUADS);glVertex2f(x,y);glVertex2f(x+w,y);glVertex2f(x+w,y+h);glVertex2f(x,y+h);glEnd(); }
void text(float x,float y,const std::string& s,Color c=ink){
    color(c);glRasterPos2f(x,y+13);
#ifdef _WIN32
    glListBase(fontBase);glCallLists((GLsizei)s.size(),GL_UNSIGNED_BYTE,s.c_str());
#else
    for(unsigned char ch:s)glutBitmapCharacter(GLUT_BITMAP_8_BY_13,ch);
#endif
}
void line2(float x,float y,float x2,float y2,Color c){color(c);glBegin(GL_LINES);glVertex2f(x,y);glVertex2f(x2,y2);glEnd();}
void button(float x,float y,float w,float h,const std::string& label,std::function<void()> action,bool enabled=true,bool accent=false){
    rect(x,y,w,h,enabled?(accent?Color{.12f,.35f,.33f}:Color{.13f,.17f,.24f}):Color{.08f,.10f,.15f});
    text(x+(w-label.size()*8)/2,y+(h-16)/2,label,enabled?(accent?teal:ink):muted);
    if(enabled)hits.push_back({x,y,w,h,std::move(action)});
}
State current(int index){return evolve(fromAngles(tracks[index].theta,tracks[index].phi),gates,step);}
void stop(){running=false;}
void syncFields(){theta=tracks[selected].theta;phi=tracks[selected].phi;}
void setInitial(double t,double p){stop();theta=std::clamp(t,0.0,180.0);phi=std::clamp(p,0.0,360.0);tracks[selected].theta=theta;tracks[selected].phi=phi;status="Kezdoallapot frissitve; a sorozat ujraszamolva.";}
void commit(){
    if(activeField<0)return;
    try { size_t pos=0;double v=std::stod(input,&pos);if(pos!=input.size()||!std::isfinite(v))throw std::invalid_argument("value");
        if(activeField==0)setInitial(v,phi);else if(activeField==1)setInitial(theta,v);else{angle=std::clamp(v,-360.0,360.0);stop();}
    }catch(...){status="Ervenytelen szam. Pelda: 45.5 vagy -90";}
    activeField=-1;input.clear();
}
void field(float x,float y,int id,double value){
    rect(x,y,73,25,activeField==id?Color{.17f,.27f,.34f}:Color{.10f,.14f,.20f});
    text(x+5,y+4,activeField==id?input:fixed(value,1),activeField==id?teal:ink);
    hits.push_back({x,y,73,25,[id,value]{activeField=id;input=fixed(value,1);freshInput=true;}});
}
void sliderDraw(float y,int id,const std::string& label,double value,double max){
    text(20,y,label,muted);field(190,y-3,id,value);
    rect(22,y+33,238,4,Color{.18f,.23f,.31f});rect(22,y+33,float(238*value/max),4,teal);
    rect(float(22+238*value/max)-4,y+28,8,14,teal);
    hits.push_back({16,y+23,250,24,[id]{slider=id;}});
}
void stepForward(){stop();if(step<(int)gates.size())++step;}
void addGate(std::string n){stop();if(gates.size()>=128){status="Legfeljebb 128 kapu adhato meg.";return;}gates.push_back({n,angle});status=n+" hozzaadva. Leptetes: jobbra nyil / Lejatszas.";scroll=std::max(0,int(gates.size())-std::max(1,(height-562)/31));}
std::string gateLabel(const Gate& g){return g.name+((g.name=="Rx"||g.name=="Ry"||g.name=="Rz"||g.name=="P")?"("+fixed(g.degrees,0)+")":"");}
void demo(){stop();tracks={{0,0,0},{90,0,1},{90,90,2}};selected=0;syncFields();gates={{"H"},{"S"},{"Ry",60},{"T"},{"Rx",90}};step=0;scroll=0;status="Pelda: harom allapot, ugyanaz az ot kapu. Inditsd el!";}
void vertex(Vec3 v){glVertex3d(v.x,v.y,v.z);}
Vec3 cross(Vec3 a,Vec3 b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
Vec3 unit(Vec3 v){double n=std::sqrt(v.x*v.x+v.y*v.y+v.z*v.z);return {v.x/n,v.y/n,v.z/n};}
void arrow(Vec3 v,Color c,float alpha=1,float thick=3){
    color(c,alpha);glLineWidth(thick);glBegin(GL_LINES);glVertex3d(0,0,0);vertex(v);glEnd();
    Vec3 u=unit(cross(v,std::abs(v.z)<.9?Vec3{0,0,1}:Vec3{0,1,0})),w=cross(v,u);
    glBegin(GL_TRIANGLE_FAN);vertex(v);
    for(int k=0;k<=18;++k){double a=k*2*pi/18;vertex({v.x*.89+.035*(u.x*cos(a)+w.x*sin(a)),v.y*.89+.035*(u.y*cos(a)+w.y*sin(a)),v.z*.89+.035*(u.z*cos(a)+w.z*sin(a))});}glEnd();glLineWidth(1);
}
void text3(double x,double y,double z,const char* s,Color c){
    color(c);glRasterPos3d(x,y,z);
#ifdef _WIN32
    glListBase(fontBase);glCallLists((GLsizei)std::char_traits<char>::length(s),GL_UNSIGNED_BYTE,s);
#else
    for(;*s;++s)glutBitmapCharacter(GLUT_BITMAP_8_BY_13,*s);
#endif
}
void sphere(){
    int vx=int(leftW),vw=std::max(1,width-int(leftW+rightW));
    int viewW=std::max(1,vw*pixelWidth/width),viewH=std::max(1,(height-165)*pixelHeight/height);
    glViewport(vx*pixelWidth/width,82*pixelHeight/height,viewW,viewH);glMatrixMode(GL_PROJECTION);glLoadIdentity();gluPerspective(37,double(viewW)/viewH,.1,50);
    glMatrixMode(GL_MODELVIEW);glLoadIdentity();
    double az=yaw*pi/180,el=elevation*pi/180;
    gluLookAt(distance*cos(el)*cos(az),distance*cos(el)*sin(az),distance*sin(el),0,0,0,0,0,1);
    glEnable(GL_DEPTH_TEST);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    // An opaque, faint wireframe keeps the back/front hemisphere depth legible.
    color({.18f,.28f,.36f});
    for(int lat=-60;lat<=60;lat+=30){double t=lat*pi/180;glBegin(GL_LINE_LOOP);for(int k=0;k<128;++k){double a=k*2*pi/128;glVertex3d(cos(t)*cos(a),cos(t)*sin(a),sin(t));}glEnd();}
    for(int lon=0;lon<180;lon+=30){double a=lon*pi/180;glBegin(GL_LINE_LOOP);for(int k=0;k<128;++k){double t=k*2*pi/128;glVertex3d(cos(t)*cos(a),cos(t)*sin(a),sin(t));}glEnd();}
    color({.34f,.44f,.56f});glBegin(GL_LINES);glVertex3d(-1.28,0,0);glVertex3d(1.28,0,0);glVertex3d(0,-1.28,0);glVertex3d(0,1.28,0);glVertex3d(0,0,-1.28);glVertex3d(0,0,1.28);glEnd();
    text3(1.32,0,0,"+X  |+>",{.9f,.57f,.43f});text3(0,1.32,0,"+Y  |+i>",{.51f,.74f,1});
    text3(0,0,1.34,"|0>  +Z",teal);text3(0,0,-1.36,"|1>  -Z",muted);
    text3(-1.35,0,0,"|->",muted);text3(0,-1.35,0,"|-i>",muted);
    // State trajectories stay on the unit sphere, including rotations > 180 degrees.
    for(int j=0;j<(int)tracks.size();++j){
        Color c=palette[tracks[j].color];State s=fromAngles(tracks[j].theta,tracks[j].phi);
        if(showInitial&&step>0){glEnable(GL_LINE_STIPPLE);glLineStipple(1,0x3333);color(c,.4f);glBegin(GL_LINES);glVertex3d(0,0,0);vertex(bloch(s));glEnd();glDisable(GL_LINE_STIPPLE);}
        if(showPaths){color(c,j==selected?.85f:.4f);glLineWidth(j==selected?2:1);
            for(int k=0;k<step;++k){auto v=bloch(s);glBegin(GL_LINE_STRIP);int samples=std::max(32,int(std::abs(gates[k].degrees)/3));for(int p=0;p<=samples;++p)vertex(pathPoint(v,gates[k],double(p)/samples));glEnd();s=applyGate(matrix(gates[k]),s);}}
        arrow(bloch(current(j)),c,j==selected?1:.7f,j==selected?4:2);
    }
    glDisable(GL_DEPTH_TEST);
}
void ui(){
    hits.clear();glViewport(0,0,pixelWidth,pixelHeight);glMatrixMode(GL_PROJECTION);glLoadIdentity();glOrtho(0,width,height,0,-1,1);glMatrixMode(GL_MODELVIEW);glLoadIdentity();
    rect(0,0,width,73,{.065f,.09f,.135f});text(22,19,"B L O C H   L A B",teal);text(22,43,"KVANTUMINFORMATIKA / 2026",muted);
    text(leftW+27,24,"EGY QUBIT. MINDEN IRANY.",ink);text(leftW+27,45,"Interaktiv OpenGL allapotter",muted);
    button(width-187,22,163,31,"Bemutato betoltese",demo);
    rect(0,74,leftW,height-74,{.075f,.10f,.145f});rect(width-rightW,74,rightW,height-74,{.075f,.10f,.145f});
    text(20,93,"01 / KEZDOALLAPOT",teal);text(20,119,"|psi> = cos(theta/2)|0>",muted);text(20,137," + exp(i phi)sin(theta/2)|1>",muted);
    sliderDraw(173,0,"theta / fok",theta,180);sliderDraw(237,1,"phi / fok",phi,360);
    const char* presets[]={"|0>","|1>","|+>","|->","|+i>","|-i>"};
    double ts[]={0,180,90,90,90,90},ps[]={0,0,0,180,90,270};
    for(int k=0;k<6;++k)button(20+(k%3)*83,304+(k/3)*34,77,28,presets[k],[k,ts,ps]{setInitial(ts[k],ps[k]);});
    text(20,390,"02 / ALLAPOTOK",teal);text(20,413,"Kozoes kapusorozat, max. 6",muted);
    for(int j=0;j<(int)tracks.size();++j){float y=441+j*32;rect(20,y,244,28,j==selected?Color{.14f,.23f,.28f}:Color{.10f,.13f,.19f});rect(28,y+8,10,10,palette[tracks[j].color]);
        text(47,y+6,"Allapot "+std::to_string(j+1)+"   t="+fixed(tracks[j].theta,0),j==selected?ink:muted);
        hits.push_back({20,y,244,28,[j]{selected=j;syncFields();}});}
    button(20,641,117,31,"+ Uj allapot",[]{bool used[6]={};for(auto t:tracks)used[t.color]=true;int c=0;while(c<6&&used[c])++c;if(c<6){tracks.push_back({theta,phi,c});selected=int(tracks.size())-1;stop();}},tracks.size()<6);
    button(145,641,119,31,"Torles",[]{if(tracks.size()>1){tracks.erase(tracks.begin()+selected);selected=std::min(selected,int(tracks.size())-1);syncFields();stop();}},tracks.size()>1);
    button(20,689,244,29,showPaths?"[x] Kapuk palyaja":"[ ] Kapuk palyaja",[]{showPaths=!showPaths;});
    button(20,725,244,29,showInitial?"[x] Kezdoallapot jelolese":"[ ] Kezdoallapot jelolese",[]{showInitial=!showInitial;});
    text(20,782,"Egerhuzas: gomb forgatasa",muted);text(20,802,"Gorgo / +/-: nagyitas",muted);text(20,822,"Enter: szam ertek elfogadasa",muted);
    float rx=width-rightW+19;
    text(rx,93,"03 / KVANTUMKAPUK",teal);text(rx,119,"Kattints a sorozat boviteshez",muted);
    const char* names[]={"X","Y","Z","H","S","Sdg","T","Tdg","Rx","Ry","Rz","P"};
    for(int k=0;k<12;++k){std::string n=names[k];button(rx+(k%4)*67,148+(k/4)*39,60,32,n,[n]{addGate(n);});}
    text(rx,278,"Forgatas / fazis (fok)",muted);field(rx+184,274,2,angle);
    button(rx,309,59,27,"-90",[]{angle=-90;});button(rx+67,309,59,27,"45",[]{angle=45;});button(rx+134,309,59,27,"90",[]{angle=90;});button(rx+201,309,59,27,"180",[]{angle=180;});
    text(rx,359,"04 / KAPUSOROZAT",teal);text(rx,383,"Lepes: "+std::to_string(step)+" / "+std::to_string(gates.size()),muted);
    button(rx+201,378,27,24,"^",[]{--scroll;},scroll>0);
    button(rx+233,378,27,24,"v",[]{++scroll;},scroll+std::max(1,(height-562)/31)<(int)gates.size());
    button(rx,411,59,32,"|<",[]{stop();step=0;},step>0);button(rx+67,411,59,32,"<",[]{stop();if(step>0)--step;},step>0);
    button(rx+134,411,59,32,">",stepForward,step<(int)gates.size());button(rx+201,411,59,32,">|",[]{stop();step=int(gates.size());},step<(int)gates.size());
    button(rx,451,260,33,running?"Szunet [Space]":"Lejatszas [Space]",[]{if(running)stop();else if(!gates.empty()){if(step==(int)gates.size())step=0;running=true;lastTick=std::chrono::steady_clock::now();}},!gates.empty(),true);
    int visible=std::max(1,(height-562)/31);scroll=std::clamp(scroll,0,std::max(0,int(gates.size())-visible));
    if(gates.empty()){text(rx,509,"Meg nincs kapu a sorozatban.",muted);text(rx,533,"Kezdd peldaul egy H kapuval.",muted);}
    for(int j=scroll;j<std::min(int(gates.size()),scroll+visible);++j){float y=501+(j-scroll)*31;rect(rx,y,260,27,j<step?Color{.10f,.23f,.23f}:Color{.10f,.13f,.19f});
        text(rx+7,y+6,std::to_string(j+1)+". "+gateLabel(gates[j]),j<step?teal:ink);
        if(j==step)text(rx+155,y+6,"kov.",muted);
        button(rx+232,y+3,23,21,"x",[j]{stop();gates.erase(gates.begin()+j);if(j<step)--step;});}
    button(rx,height-48,260,30,"Sorozat torlese",[]{stop();gates.clear();step=0;scroll=0;},!gates.empty());
    const auto state=current(selected);const auto v=bloch(state);float cx=leftW+24,cw=width-leftW-rightW-48;
    text(cx,94,"ALLAPOT "+std::to_string(selected+1)+" / "+std::to_string(step)+". LEPES",palette[tracks[selected].color]);
    button(cx+cw-92,88,92,27,"Nezet alap",[]{yaw=40;elevation=23;distance=4.4;});
    text(cx,126,"alpha = "+complexText(state[0]),muted);text(cx,147,"beta  = "+complexText(state[1]),muted);
    text(cx,height-168,"BLOCH  ("+fixed(v.x)+", "+fixed(v.y)+", "+fixed(v.z)+")",ink);
    double p0=std::clamp(std::norm(state[0]),0.0,1.0);rect(cx,height-134,cw,9,{.20f,.23f,.32f});rect(cx,height-134,cw*p0,9,palette[tracks[selected].color]);
    text(cx,height-115,"P(0) = "+fixed(100*p0,1)+"%",ink);text(cx+cw-144,height-115,"P(1) = "+fixed(100*(1-p0),1)+"%",ink);
    text(cx,height-86,"Z bazisu meresi valoszinusegek",muted);
    line2(cx,height-60,cx+cw,height-60,{.19f,.23f,.30f});
    std::string msg=status;size_t cap=std::max(0,int(cw/8));if(msg.size()>cap)msg=msg.substr(0,cap>3?cap-3:0)+"...";text(cx,height-44,msg,muted);
}
void writeSnapshot(){
    std::vector<unsigned char> pixels(size_t(pixelWidth)*pixelHeight*3);glPixelStorei(GL_PACK_ALIGNMENT,1);glReadBuffer(GL_BACK);glReadPixels(0,0,pixelWidth,pixelHeight,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
    std::ofstream f(snapshot,std::ios::binary);f<<"P6\n"<<pixelWidth<<" "<<pixelHeight<<"\n255\n";for(int y=pixelHeight-1;y>=0;--y)f.write((char*)pixels.data()+size_t(y)*pixelWidth*3,pixelWidth*3);
}
void display(){
    glClearColor(.045f,.065f,.10f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);sphere();ui();
    if(!snapshot.empty()){writeSnapshot();std::exit(0);}
#ifdef _WIN32
    SwapBuffers(device);
#else
    glutSwapBuffers();
#endif
}
void reshape(int w,int h){pixelWidth=std::max(w,1);pixelHeight=std::max(h,1);width=std::max(w,1200);height=std::max(h,880);}
void sliderMove(int x){double f=std::clamp((x-22)/238.0,0.0,1.0);if(slider==0)setInitial(f*180,phi);else if(slider==1)setInitial(theta,f*360);}
void mouse(int b,int state,int x,int y){
    x=x*width/pixelWidth;y=y*height/pixelHeight;
    if(b==3||b==4){if(state==0){if(x>width-rightW)scroll+=(b==3?-1:1);else if(x>leftW)distance=std::clamp(distance+(b==3?-.2:.2),2.7,8.0);}return;}
    if(b!=0)return;
    if(state!=0){dragging=false;slider=-1;return;}
    commit();for(auto it=hits.rbegin();it!=hits.rend();++it)if(x>=it->x&&x<=it->x+it->w&&y>=it->y&&y<=it->y+it->h){auto action=it->action;action();if(slider>=0)sliderMove(x);return;}
    if(x>leftW&&x<width-rightW&&y>175&&y<height-180){dragging=true;lastX=x;lastY=y;}
}
void motion(int x,int y){x=x*width/pixelWidth;y=y*height/pixelHeight;if(slider>=0)sliderMove(x);else if(dragging){yaw-=(x-lastX)*.45;elevation=std::clamp(elevation+(y-lastY)*.45,-85.0,85.0);lastX=x;lastY=y;}}
void key(unsigned char k,int=0,int=0){
    if(activeField>=0){if(k==13)commit();else if(k==27){activeField=-1;input.clear();}else if(k==8||k==127){if(freshInput)input.clear();else if(!input.empty())input.pop_back();freshInput=false;}else if((k>='0'&&k<='9')||k=='.'||k=='-'||k=='+'){if(freshInput)input.clear();freshInput=false;if(input.size()<8)input+=char(k);}return;}
    if(k==' '){if(running)stop();else if(!gates.empty()){if(step==(int)gates.size())step=0;running=true;lastTick=std::chrono::steady_clock::now();}}
    if(k=='+'||k=='=')distance=std::max(2.7,distance-.2);
    if(k=='-')distance=std::min(8.0,distance+.2);
    if(k=='r'||k=='R'){stop();step=0;}
    if(k=='x'||k=='X')addGate("X");if(k=='y'||k=='Y')addGate("Y");if(k=='z'||k=='Z')addGate("Z");if(k=='h'||k=='H')addGate("H");
}
void tick(){if(running&&std::chrono::duration<double>(std::chrono::steady_clock::now()-lastTick).count()>.8){if(step<(int)gates.size())++step;if(step==(int)gates.size())stop();lastTick=std::chrono::steady_clock::now();}}
#ifdef _WIN32
LRESULT CALLBACK wndProc(HWND win,UINT msg,WPARAM wp,LPARAM lp){
    switch(msg){
    case WM_GETMINMAXINFO:{auto m=(MINMAXINFO*)lp;m->ptMinTrackSize.x=1100;m->ptMinTrackSize.y=650;return 0;}
    case WM_SIZE:reshape(LOWORD(lp),HIWORD(lp));return 0;
    case WM_LBUTTONDOWN:SetCapture(win);mouse(0,0,GET_X_LPARAM(lp),GET_Y_LPARAM(lp));return 0;
    case WM_LBUTTONUP:ReleaseCapture();mouse(0,1,GET_X_LPARAM(lp),GET_Y_LPARAM(lp));return 0;
    case WM_MOUSEMOVE:motion(GET_X_LPARAM(lp),GET_Y_LPARAM(lp));return 0;
    case WM_MOUSEWHEEL:{POINT p{GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};ScreenToClient(win,&p);mouse(GET_WHEEL_DELTA_WPARAM(wp)>0?3:4,0,p.x,p.y);return 0;}
    case WM_CHAR:key((unsigned char)wp);return 0;
    case WM_KEYDOWN:if(activeField<0){if(wp==VK_RIGHT)stepForward();if(wp==VK_LEFT){stop();step=std::max(0,step-1);}}return 0;
    case WM_CLOSE:DestroyWindow(win);return 0;
    case WM_DESTROY:PostQuitMessage(0);return 0;
    default:return DefWindowProc(win,msg,wp,lp);}
}
int main(int argc,char** argv){
    for(int i=1;i<argc;++i){if(std::string(argv[i])=="--demo")demo();if(std::string(argv[i])=="--demo-end"){demo();step=int(gates.size());}if(std::string(argv[i])=="--snapshot"&&i+1<argc)snapshot=argv[++i];}
    SetProcessDPIAware();HINSTANCE inst=GetModuleHandle(nullptr);WNDCLASSA wc{};wc.style=CS_OWNDC;wc.lpfnWndProc=wndProc;wc.hInstance=inst;wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.lpszClassName="BlochLab";RegisterClassA(&wc);
    RECT r{0,0,std::min(width,GetSystemMetrics(SM_CXSCREEN)-60),std::min(height,GetSystemMetrics(SM_CYSCREEN)-100)};AdjustWindowRect(&r,WS_OVERLAPPEDWINDOW,FALSE);
    HWND win=CreateWindowA("BlochLab","Bloch Lab - Kvantuminformatika 2026",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,r.right-r.left,r.bottom-r.top,nullptr,nullptr,inst,nullptr);
    device=GetDC(win);PIXELFORMATDESCRIPTOR pfd{};pfd.nSize=sizeof(pfd);pfd.nVersion=1;pfd.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;pfd.iPixelType=PFD_TYPE_RGBA;pfd.cColorBits=24;pfd.cDepthBits=24;
    int pf=ChoosePixelFormat(device,&pfd);if(!pf||!SetPixelFormat(device,pf,&pfd)){MessageBoxA(win,"OpenGL pixel format failed","Error",MB_OK);return 1;}
    HGLRC ctx=wglCreateContext(device);if(!ctx||!wglMakeCurrent(device,ctx)){MessageBoxA(win,"OpenGL context failed","Error",MB_OK);return 1;}
    HFONT font=CreateFontA(-13,8,0,0,FW_NORMAL,FALSE,FALSE,FALSE,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,FIXED_PITCH,"Consolas");auto oldFont=SelectObject(device,font);fontBase=glGenLists(256);wglUseFontBitmapsA(device,0,256,fontBase);
    ShowWindow(win,SW_SHOW);MSG msg{};bool alive=true;while(alive){while(PeekMessage(&msg,nullptr,0,0,PM_REMOVE)){if(msg.message==WM_QUIT){alive=false;break;}TranslateMessage(&msg);DispatchMessage(&msg);}if(alive){tick();display();Sleep(16);}}
    glDeleteLists(fontBase,256);SelectObject(device,oldFont);DeleteObject(font);wglMakeCurrent(nullptr,nullptr);wglDeleteContext(ctx);ReleaseDC(win,device);return 0;
}
#else
void timer(int){tick();glutPostRedisplay();glutTimerFunc(16,timer,0);}
void special(int k,int,int){if(activeField<0){if(k==GLUT_KEY_RIGHT)stepForward();if(k==GLUT_KEY_LEFT){stop();step=std::max(0,step-1);}}}
int main(int argc,char** argv){
    for(int i=1;i<argc;++i){if(std::string(argv[i])=="--demo")demo();if(std::string(argv[i])=="--demo-end"){demo();step=int(gates.size());}if(std::string(argv[i])=="--snapshot"&&i+1<argc)snapshot=argv[++i];}
    int args=1;glutInit(&args,argv);glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGBA|GLUT_DEPTH);glutInitWindowSize(std::min(width,glutGet(GLUT_SCREEN_WIDTH)-40),std::min(height,glutGet(GLUT_SCREEN_HEIGHT)-100));glutCreateWindow("Bloch Lab - Kvantuminformatika 2026");
    glutDisplayFunc(display);glutReshapeFunc(reshape);glutMouseFunc(mouse);glutMotionFunc(motion);glutKeyboardFunc(key);glutSpecialFunc(special);glutTimerFunc(16,timer,0);glutMainLoop();return 0;
}
#endif
