/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/camera/hud.hxx
 */

#include <cmath>
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <vector>
#include <iostream>
#include <sstream>
#include <map>
#include <string>
#include <algorithm>
#include <typeinfo>

#include <GL/gl.h>
#include <libconfig.h++>

#include "hud.hxx"
#include "camera.hxx"
#include "dynamic_camera.hxx"
#include "src/globals.hxx"
#include "src/secondary/global_chat.hxx"
#include "src/sim/collision.hxx"
#include "src/control/human_controller.hxx"
#include "src/control/hc_conf.hxx"
#include "src/ship/ship.hxx"
#include "src/ship/cell/cell.hxx"
#include "src/ship/sys/ship_system.hxx"
#include "src/ship/insignia.hxx"
#include "src/ship/auxobj/shield.hxx"
#include "src/graphics/vec.hxx"
#include "src/graphics/matops.hxx"
#include "src/graphics/shader.hxx"
#include "src/graphics/cmn_shaders.hxx"
#include "src/graphics/shader_loader.hxx"
#include "src/graphics/square.hxx"
#include "src/graphics/glhelp.hxx"
#include "src/exit_conditions.hxx"
#include "src/core/lxn.hxx"

using namespace hc_conf;
using namespace std;
using namespace libconfig;

#define RETICLE_SIZE 0.03f

#ifndef AB_OPENGL_14
#define VERTEX_TYPE shader::VertTexc
#define UNIFORM_TYPE WeaponHUDInfo
DELAY_SHADER(hudReticle)
  sizeof(VERTEX_TYPE),
  VATTRIB(vertex), VATTRIB(texCoord), NULL,
  true,
  UNIFLOAT(topBarVal), UNIFLOAT(botBarVal), UNIFLOAT(leftBarVal), UNIFLOAT(rightBarVal),
  UNIFORM (topBarCol), UNIFORM (botBarCol), UNIFORM (leftBarCol), UNIFORM (rightBarCol),
  NULL
END_DELAY_SHADER(static reticleShader);
#endif /* AB_OPENGL_14 */

/* A status icon is associated with an OpenGL VBO and
 * a drawing mode, which is one of GL_LINES,
 * GL_LINE_STRIP, and GL_LINE_LOOP.
 */
struct StatusIcon {
  GLuint vao, vbo;
  GLenum drawMode;
  unsigned length;
};

/* Maps status names to their respective icons. */
static map<string,StatusIcon> statusIcons;

/* Loads a set of vertices from the given parent setting.
 * If the included argument is true, vertices are automatically
 * expanded to segmented line format if they aren't already,
 * and further include directives will be processed. Otherwise,
 * neither of these actions is performed.
 */
static void loadIconVertices(vector<shader::quickV>& vertices, const Setting& item, bool included) {
  const char* formatStr = item["format"];
  GLenum format;

  if (0 == strcmp(formatStr, "lines"))
    format = GL_LINES;
  else if (0 == strcmp(formatStr, "strip"))
    format = GL_LINE_STRIP;
  else if (0 == strcmp(formatStr, "loop"))
    format = GL_LINE_LOOP;
  else {
    cerr << "FATAL: Unknown format specifier for icon " << item.getName() << ": " << formatStr << endl;
    exit(EXIT_MALFORMED_CONFIG);
  }

  if (included && item.exists("include")) {
    for (unsigned i=0; i<(unsigned)item["include"].getLength(); ++i)
      loadIconVertices(vertices, item.getParent()[(const char*)item["include"][i]], true);
  }

  unsigned initSz = vertices.size();
  for (unsigned i=0; i<(unsigned)item["vertices"].getLength(); i+=2) {
    if (included && format != GL_LINES && vertices.size() > initSz+1)
      //Duplicate previous vertex
      vertices.push_back(vertices.back());
    //Load next vertex
    shader::quickV v = {{{(float)item["vertices"][i], (float)item["vertices"][i+1]}}};
    vertices.push_back(v);
  }
  //Close loop if necessary
  if (included && format == GL_LINE_LOOP) {
    vertices.push_back(vertices.back());
    vertices.push_back(vertices[initSz]);
  }
}

/* Loads icons from images/hud_icons.dat. */
static void loadStatusIcons() {
  if (headless) return;

  Config config;
  try {
    config.readFile("images/hud_icons.dat");
  } catch (...) {
    cerr << "FATAL: Unable to load images/hud_icons.dat" << endl;
    exit(EXIT_MALFORMED_CONFIG);
  }

  const Setting& root(config.getRoot());

  for (unsigned i=0; i<(unsigned)root.getLength(); ++i) {
    string name(root[i].getName());
    const char* formatStr = root[i]["format"];
    GLenum format;
    if (0 == strcmp(formatStr, "lines"))
      format = GL_LINES;
    else if (0 == strcmp(formatStr, "strip"))
      format = GL_LINE_STRIP;
    else if (0 == strcmp(formatStr, "loop"))
      format = GL_LINE_LOOP;
    else {
      cerr << "FATAL: Unknown format specifier for icon " << name << ": " << formatStr << endl;
      exit(EXIT_MALFORMED_CONFIG);
    }

    vector<shader::quickV> vertices;

    if (root[i].exists("include")) {
      if (format != GL_LINES) {
        cerr << "FATAL: Non-lines format for icon with include statement: " << name << endl;
        exit(EXIT_MALFORMED_CONFIG);
      }
      for (unsigned j=0; j<(unsigned)root[i]["include"].getLength(); ++j)
        loadIconVertices(vertices, root[(const char*)root[i]["include"][j]], true);
    }

    loadIconVertices(vertices, root[i], false);

    StatusIcon icon;
    icon.vao = newVAO();
    glBindVertexArray(icon.vao);
    icon.vbo = newVBO();
    icon.drawMode = format;
    icon.length = vertices.size();
    glBindBuffer(GL_ARRAY_BUFFER, icon.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(shader::quickV)*icon.length, &vertices[0], GL_STATIC_DRAW);
    shader::quick->setupVBO();

    statusIcons[name] = icon;
  }
}

namespace hud_user_messages {            //0123456789ABCDEF
  char msg0[MAX_HUD_USER_MESSAGE_LEN+1] = "Abendstern Pre-A",
       msg1[MAX_HUD_USER_MESSAGE_LEN+1] = "Build date:",
       msg2[MAX_HUD_USER_MESSAGE_LEN+1] = __DATE__,
       msg3[MAX_HUD_USER_MESSAGE_LEN+1] = __TIME__;
  void setmsg(unsigned msgix, const char* txt) {
    char* msg;
    switch (msgix) {
      case 0: msg=msg0; break;
      case 1: msg=msg1; break;
      case 2: msg=msg2; break;
      case 3: msg=msg3; break;
      default: return;
    }
    unsigned i=0;
    while (i < MAX_HUD_USER_MESSAGE_LEN && *txt)
      *msg++ = *txt++;
    *msg=0; //Force NUL-termination
  }
}

HUD::HUD(Ship* ref, GameField* field) :
  ship(ref),
  font(*sysfontStipple), smallFont(*smallFontStipple),
  timeSinceDamage(10000), stensilBufferSetup(0), fullScreenMap(false)
{
  setRef(ref);
  if (statusIcons.empty()) loadStatusIcons();
}

void HUD::update(float et) noth {
  timeSinceDamage+=et;
}

void HUD::drawChat() noth {
  libconfig::Setting& colour(conf["conf"]["hud"]["colours"]["standard"]);
  font.preDraw(colour[0], colour[1], colour[2], 1);
  float fh = font.getHeight();
  float y = vheight-fh;
  unsigned charsAcross = 1024;
  //unsigned softBreakPoint = (unsigned)(0.8f*charsAcross);
  static char linebuff[1025];
  for (unsigned i=0; i<global_chat::messages.size(); ++i) {
    const char* msg=global_chat::messages[i].text.c_str();
    while (*msg) {
      for (unsigned off=0; off<charsAcross && *msg; ++msg, ++off) {
        linebuff[off]=*msg;
        linebuff[off+1]=0;
        if (isspace(*msg) && font.width(linebuff) > 0.9f) break;
        if (font.width(linebuff) > 0.99f) break;
      }
      //Draw line; don't clear stack if there is more
      //text left
      font.draw(linebuff, 0, y, 2.0f, *msg);
      y-=fh;
    }
  }

  font.postDraw();
}

void HUD::draw(Camera* camera) noth {
  BEGINGP("HUD")

  if (!ship.ref) {
    //Draw only chat
    mPush();
    mPush(matrix_stack::view);
    mId(matrix_stack::view);
    mTrans(-1, -1, matrix_stack::view);
    mUScale(2, matrix_stack::view);
    mConc(matrix(1,0,0,0,
                 0,1/(vheight=screenH/(float)screenW),0,0,
                 0,0,1,0,
                 0,0,0,1), matrix_stack::view);
    #ifdef AB_OPENGL_14
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 1.0, 0.0, vheight=screenH/(float)screenW, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    #endif
    mId();

    drawChat();

    mPop();
    mPop(matrix_stack::view);
    return;
  }

  Ship* s=(Ship*)ship.ref;
  bool showWeaponInfo = s->controller
                     && typeid(*s->controller) == typeid(HumanController);

  /* Handle flickering due to damage */
  float flickerProbability = showWeaponInfo
                          ? 1 - 1.5*currentStrength / originalStrength
                          : 0;
  if (flickerProbability > 0.9f) flickerProbability=0.9f;
  else if (flickerProbability < 0.1f) flickerProbability=0;
  if (rand()/(float)RAND_MAX < flickerProbability) return; //flicker

  Ship* t=(Ship*)s->target.ref;
  float visualRotation = ((DynamicCamera*)camera)->getVisualRotation();
  float colourStd[4], colourWarn[4], colourDang[4], colourSpec[4];
  {
    libconfig::Setting& colours(conf["conf"]["hud"]["colours"]);
    for (unsigned i=0; i<4; ++i) {
      colourStd[i] = colours["standard"][i];
      colourWarn[i] = colours["warning"][i];
      colourDang[i] = colours["danger"][i];
      colourSpec[i] = colours["special"][i];
    }
  }

  Weapon currWeapon = showWeaponInfo
                    ? ((HumanController*)s->controller)->currentWeapon
                    : (Weapon)-1;
  const char* weaponDisplayName=NULL, * weaponIconName=NULL, * weaponComment=NULL;
  if (showWeaponInfo) {
    switch (currWeapon) {
      case Weapon_EnergyCharge:
        SL10N(weaponDisplayName, hud_abbr, energy_charge)
        weaponIconName = "energy_charge";
        break;
      case Weapon_MagnetoBomb:
        SL10N(weaponDisplayName, hud_abbr, magneto_bomb)
        weaponIconName = "magneto_bomb";
        break;
      case Weapon_PlasmaBurst:
        SL10N(weaponDisplayName, hud_abbr, plasma_burst)
        weaponIconName = "plasma_burst";
        break;
      case Weapon_SGBomb:
        SL10N(weaponDisplayName, hud_abbr, semiguided_bomb)
        weaponIconName = "semiguided_bomb";
        break;
      case Weapon_GatlingPlasma:
        SL10N(weaponDisplayName, hud_abbr, gatling_plasma_burst)
        weaponIconName = "gatling_plasma_burst";
        break;
      case Weapon_Monophase:
        SL10N(weaponDisplayName, hud_abbr, monophasic)
        weaponIconName = "monophasic";
        break;
      case Weapon_Missile:
        SL10N(weaponDisplayName, hud_abbr, missile)
        weaponIconName = "missile_launcher";
        break;
      case Weapon_ParticleBeam:
        SL10N(weaponDisplayName, hud_abbr, particle_beam)
        weaponIconName = "particle_beam";
        break;
      default:
        cerr << "FATAL: Unexpected current weapon: " << currWeapon;
        exit(EXIT_PROGRAM_BUG);
    }
  }

  //Triangular ship indicators for maps;
  //here because two code-paths need these
  static GLuint triVAO=0, triVBO;
  if (!triVAO) {
    triVAO=newVAO();
    glBindVertexArray(triVAO);
    triVBO=newVBO();
    glBindBuffer(GL_ARRAY_BUFFER, triVBO);
    static const float size = 0.01f;
    static shader::colourStipleV vertices[4] = {
      {{{ -size/2, +size/3 }}},
      {{{ 0, 0 }}},
      {{{ size, 0 }}},
      {{{ -size/2, -size/3 }}},
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    shader::colourStiple->setupVBO();
  }

  //First, draw target indicators around non-player ships.
  //Indicator colours:
  //  Special   friend
  //  Danger    enemy non-target
  //  Warning   (flashing) enemy target
  //  nothing   self, neuteral, dead
  {
    static GLuint vao=0, vbo;
    if (!vao) {
      vao=newVAO();
      glBindVertexArray(vao);
      vbo=newVBO();
      glBindBuffer(GL_ARRAY_BUFFER, vbo);

      const shader::colourStipleV vertices[4] = {
        {{{-1,-1}}}, {{{+1,-1}}}, {{{+1,+1}}}, {{{-1,+1}}},
      };
      glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
      shader::colourStiple->setupVBO();
    }

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    float r, g, b;
    radar_t::iterator it, end;
    s->radarBounds(it, end);
    GameField& field(*s->getField());
    while (it != end) {
      Ship* ship=*it++;
      if (ship == s) continue; //Nothing for self
      if (ship == t && 512 < field.fieldClock % 1024) continue; //Blink off for target
      Alliance alliance=getAlliance(s->insignia, ship->insignia);
      if (!ship->hasPower()) continue; //Dead
      //Skip if off screen
      float radius = ship->getRadius();
      if (ship->getX()+radius < cameraX1
      ||  ship->getX()-radius > cameraX2
      ||  ship->getY()+radius < cameraY1
      ||  ship->getY()-radius > cameraY2)
        continue;

      //OK, determine colours
      if (ship == t) {
        r = colourWarn[0];
        g = colourWarn[1];
        b = colourWarn[2];
      } else if (alliance == Allies) {
        r = colourSpec[0];
        g = colourSpec[1];
        b = colourSpec[2];
      } else if (alliance == Neutral) {
        r = colourStd [0];
        g = colourStd [1];
        b = colourStd [2];
      } else {
        r = colourDang[0];
        g = colourDang[1];
        b = colourDang[2];
      }

      //Transform and draw
      mPush();
      mTrans(ship->getX(), ship->getY());
      mRot(visualRotation);
      mUScale(1/cameraZoom);
      smallFont.preDraw(r,g,b,1);
      smallFont.draw(ship->tag.c_str(),
                     -ship->getBoundingSquareHalfEdge()*cameraZoom,
                     +ship->getBoundingSquareHalfEdge()*cameraZoom);
      smallFont.postDraw();
      mUScale(cameraZoom*ship->getBoundingSquareHalfEdge());
      glBindVertexArray(vao);
      glBindBuffer(GL_ARRAY_BUFFER, vbo);
      shader::colourStipleU uni = { {{r,g,b,1}}, (int)screenW, (int)screenH };
      shader::colourStiple->activate(&uni);
      glDrawArrays(GL_LINE_LOOP, 0, 4);
      mPop();
    }
  }

  if (!fullScreenMap) {
    const char* wstatus = showWeaponInfo? weapon_getStatus(s, currWeapon) : NULL;
    if (wstatus) {
      /* If the target exists and is close enough, we implement a
       * "smart" targetting system of sorts. We use the velocity
       * difference between the player's ship and the target to
       * predect the time it would take the projectile to reach
       * the target (assuming perfect aim), then place the reticule
       * there. We have the following variables:
       *   ps   Vector, position of our ship
       *   pt   Vector, position of the target
       *   vs   Vector, our ship velocity
       *   vt   Vector, target ship velocity
       *   s    Scalar, speed of projectile
       *   va   Unit vector, our ship's angle
       *
       * Relative velocity is (vs-vt). We are only concerned with the
       * velocity to or from us, so that is
       *   dot(norm(ps-pt),(vs-vt))
       * Relative speed between projectile and ship is then
       *   s + dot(norm(ps-pt),(vs-vt))
       * And time is
       *  t = len(ps-pt) / (s - dot(norm(ps-pt),(vs-vt)))
       * We can then determine "future would-be collision location" as
       *   pf = ps + t*(s*va+vs);
       * The extrapolated position of the target is
       *   px = pt + vt*t
       * We want to represent how far off we are from the future, so subtract
       * the future difference from current position, yielding the draw position:
       *   pd = pf + pt - px
       *
       * We clamp the final coordinates to [0.1,0.1]x[0.9,vheight*0.9] so the
       * reticle doesn't go off-screen. If there is no target, or it's more than
       * 10 screens away, just draw on the player's ship.
       */
      float retX, retY;
      bool draw=true;
      WeaponHUDInfo info;
      info.leftBarVal=info.rightBarVal=info.topBarVal=info.botBarVal=0;
      if (t) info.dist = sqrt((s->getX()-t->getX())*(s->getX()-t->getX()) +
                              (s->getY()-t->getY())*(s->getY()-t->getY()));
      else info.dist=0;
      weapon_getWeaponInfo(s, currWeapon, info);
      if (!t
      ||  (s->getX()-t->getX())*(s->getX()-t->getX())
        + (s->getY()-t->getY())*(s->getY()-t->getY()) > 100) {
        //Too far away or non-existent
        draw=false;
      } else {
        float psx = s->getX(), psy = s->getY();
        float ptx = t->getX(), pty = t->getY();
        float vsx = s->getVX(), vsy = s->getVY();
        float vtx = t->getVX(), vty = t->getVY();
        float vax = cos(s->getRotation());
        float vay = sin(s->getRotation());
        float s = info.speed;
        float dx = ptx-psx;
        float dy = pty-psy;
        float d = sqrt(dx*dx + dy*dy);
        float t = d / (s + (dx/d*(vsx-vtx) + dy/d*(vsy-vty)));
        float pfx = psx + t*(s*vax+vsx);
        float pfy = psy + t*(s*vay+vsy);
        float pxx = ptx + vtx*t;
        float pxy = pty + vty*t;
        retX = pfx + ptx - pxx;
        retY = pfy + pty - pxy;
      }

      string status(wstatus);
      assert(statusIcons.find(status) != statusIcons.end());
      const StatusIcon& icon(statusIcons[status]);

      if (draw) {
        mPush();
        mTrans(retX, retY);
        mRot(visualRotation);
        mUScale(1/cameraZoom * RETICLE_SIZE);
        glBindVertexArray(icon.vao);
        glBindBuffer(GL_ARRAY_BUFFER, icon.vbo);
        shader::quickU uni = {{{1,1,1,1}}};
        shader::quick->activate(&uni);
        glDrawArrays(icon.drawMode, 0, icon.length);

#ifndef AB_OPENGL_14
        mUScale(2.5f);
        mTrans(-0.5f,-0.5f);
        square_graphic::bind();
        reticleShader->activate(&info);
        square_graphic::draw();
#endif /* AB_OPENGL_14 */
        mPop();
      }
    } //else nothing to draw, no weapon here

    //Draw primary HUD
    mPush();
    mPush(matrix_stack::view);
    mId(matrix_stack::view);
    mTrans(-1, -1, matrix_stack::view);
    mUScale(2, matrix_stack::view);
    mConc(matrix(1,0,0,0,
                 0,1/(vheight=screenH/(float)screenW),0,0,
                 0,0,1,0,
                 0,0,0,1), matrix_stack::view);
    #ifdef AB_OPENGL_14
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 1.0, 0.0, vheight=screenH/(float)screenW, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    #endif
    mId();

    float fh=font.getHeight();
    float fw=font.width("M");
    //Draw labels
    font.preDraw(colourStd[0], colourStd[1], colourStd[2], 1);
    static string pcthss = _(hud_abbr, pow_cap_throt_heat_stren_stab);
    font.draw(pcthss.c_str(), fw/2.0f, 5*fh, 999, false, &fw);
    string targetName;
    SL10N(targetName, hud, target_prefix)
    if (!isCompositionBufferInUse) {
      if (t) {
        targetName += t->tag;
      } else {
        targetName += _(hud, target_none);
      }
    } else {
      targetName = _(hud, message_prefix);
      targetName += compositionBuffer;
    }
    font.draw(targetName.c_str(), 7*fw, 0);

    ostringstream lvlWriter, cntWriter;
    if (wstatus) {
      static string lvl(_(hud_abbr, level)), cnt(_(hud_abbr, count));
      lvlWriter << lvl << weapon_getEnergyLevel(s, currWeapon);
      cntWriter << cnt << (unsigned)(s->getCurrentCapacitance() / weapon_getLaunchEnergy(s, currWeapon));
    } else {
      static string syserr404(_(hud_abbr, weapon_not_found0)), notprsnt(_(hud_abbr, weapon_not_found1));
      lvlWriter << syserr404;
      cntWriter << notprsnt;
    }

    if (showWeaponInfo) {
      weaponComment = weapon_getComment(s, currWeapon);
      font.draw(lvlWriter.str().c_str(), 12*fw, 4*fh);
      font.draw(cntWriter.str().c_str(), 12*fw, 3*fh);
      font.draw(weaponDisplayName, 8*fw, 2*fh);
      if (weaponComment)
        font.draw(weaponComment, 8*fw, 1*fh);
    }

    font.draw(hud_user_messages::msg0, 1 - 40.5f*fw, 4*fh);
    font.draw(hud_user_messages::msg1, 1 - 40.5f*fw, 3*fh);
    font.draw(hud_user_messages::msg2, 1 - 40.5f*fw, 2*fh);
    font.draw(hud_user_messages::msg3, 1 - 40.5f*fw, 1*fh);

    font.postDraw();

    //Draw chat messages
    drawChat();

    /* Draw minimap.
     * In order to be more useful, perform a semispherical scaling.
     * Transform each coordinate as follows:
     *   (xn = sin(pi*(x-cx)/vMapWidth)/2+0.5)*(x2-x1)+x1
     * and similar for yn.
     */
    {
      const float vMapWidth = 64;
      const float x1 = 1 - 8*fw, x2 = 1, y1 = 0, y2=5*fh;
      const float ratio = (x2-x1)/(y2-y1);
      const float vMapHeight = vMapWidth / ratio;
      const float minX = s->getX() - vMapWidth/2, maxX = s->getX() + vMapWidth/2;
      const float minY = s->getY() - vMapHeight/2, maxY = s->getY() + vMapHeight/2;
      const float cx = s->getX(), cy = s->getY();

      #define TX(x) ((sin(pi*(x-cx)/vMapWidth )/2+0.5f)*(x2-x1))
      #define TY(y) ((sin(pi*(y-cy)/vMapHeight)/2+0.5f)*(y2-y1))

      float fx = s->getX() + 10000*s->getVX(),
            fy = s->getY() + 10000*s->getVY();
      bool alarm = fx < 0 || fx > s->getField()->width
                || fy < 0 || fy > s->getField()->height;

      static GLuint backVAO=0, backVBO, horzLineVAO, horzLineVBO, vertLineVAO, vertLineVBO;
      if (!backVAO) {
        backVAO=newVAO();
        glBindVertexArray(backVAO);
        backVBO=newVBO();
        glBindBuffer(GL_ARRAY_BUFFER, backVBO);
        const shader::colourStipleV backVerts[4] = {
          {{{x1,y1}}}, {{{x2,y1}}}, {{{x1,y2}}}, {{{x2,y2}}},
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(backVerts), backVerts, GL_STATIC_DRAW);
        shader::colourStiple->setupVBO();

        /* The stensil buffer will be stipled, so we don't need to waste time
         * (and risk artifacts) stipling everything else.
         */
        horzLineVAO=newVAO();
        glBindVertexArray(horzLineVAO);
        horzLineVBO=newVBO();
        glBindBuffer(GL_ARRAY_BUFFER, horzLineVBO);
        const shader::quickV horzLineVerts[2] = {
          {{{x1,y1}}}, {{{x2,y1}}},
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(horzLineVerts), horzLineVerts, GL_STATIC_DRAW);
        shader::quick->setupVBO();

        vertLineVAO=newVAO();
        glBindVertexArray(vertLineVAO);
        vertLineVBO=newVBO();
        glBindBuffer(GL_ARRAY_BUFFER, vertLineVBO);
        const shader::quickV vertLineVerts[2] = {
          {{{x1,y1}}}, {{{x1,y2}}},
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertLineVerts), vertLineVerts, GL_STATIC_DRAW);
        shader::quick->setupVBO();
      }

      if (!stensilBufferSetup--) {
        glClearStencil(0);
        glClear(GL_STENCIL_BUFFER_BIT);
        glEnable(GL_STENCIL_TEST);
        glStencilFunc(GL_ALWAYS,1,0xFF);
        glStencilOp(GL_INCR,GL_INCR,GL_INCR);
        stensilBufferSetup=600;
      }

      glBindVertexArray(backVAO);
      glBindBuffer(GL_ARRAY_BUFFER, backVBO);
      shader::colourStipleU uni = { {{0,0,0,1}}, (int)screenW, (int)screenH };
      shader::colourStiple->activate(&uni);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

      glEnable(GL_STENCIL_TEST);
      glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
      glStencilFunc(GL_EQUAL,1,0xFF);

      //Draw grid lines
      glBindVertexArray(vertLineVAO);
      glBindBuffer(GL_ARRAY_BUFFER, vertLineVBO);
      float* lineCol = (alarm && 128 < s->getField()->fieldClock % 256? colourDang : colourStd);
      shader::quickU luni = { {{lineCol[0], lineCol[1], lineCol[2], 1}} };
      for (int x=((int)max(0.0f,minX))&~3; x <= min((float)s->getField()->width, maxX); x+=4) {
        mPush();
        mTrans(TX(x), 0);
        shader::quick->activate(&luni);
        glDrawArrays(GL_LINES, 0, 2);
        mPop();
      }

      glBindVertexArray(horzLineVAO);
      glBindBuffer(GL_ARRAY_BUFFER, horzLineVBO);
      for (int y=((int)max(0.0f,minY))&~3; y <= min((float)s->getField()->height, maxY); y+=4) {
        mPush();
        mTrans(0, TY(y));
        shader::quick->activate(&luni);
        glDrawArrays(GL_LINES, 0, 2);
        mPop();
      }

      //Other ships
      //Since quick and colourStiple use the same vertex format,
      //we can get away with using quick here.
      //Draw neutrals and friends first, then self and target, then enemies
      radar_t::iterator begin, end;
      s->radarBounds(begin,end);
      float r, g, b, scale;
      glBindVertexArray(triVAO);
      glBindBuffer(GL_ARRAY_BUFFER, triVBO);
      for (unsigned i=0; i<3; ++i) {
        for (radar_t::iterator it=begin; it != end; ++it) {
          Ship* ship = *it;
          if (!ship->hasPower()) continue;

          if (ship->getX() < minX || ship->getX() > maxX
          ||  ship->getY() < minY || ship->getY() > maxY)
            continue; //Won't show up anyway

          Alliance alliance = getAlliance(s->insignia, ship->insignia);
          if (ship == s) {
            if (i != 1) continue;
            r=g=b=1;
            scale=2;
          } else if (ship == t) {
            if (i != 1) continue;
            //Flash
            if (512 < s->getField()->fieldClock % 1024) continue;
            r=colourWarn[0];
            g=colourWarn[1];
            b=colourWarn[2];
            scale=2;
          } else if (alliance == Allies) {
            if (i != 0) continue;
            r=colourSpec[0];
            g=colourSpec[1];
            b=colourSpec[2];
            scale=0.5f;
          } else if (alliance == Enemies) {
            if (i != 2) continue;
            r=colourDang[0];
            g=colourDang[1];
            b=colourDang[2];
            scale=1;
          } else /* Neutral */ {
            if (i != 0) continue;
            r=colourStd[0];
            g=colourStd[1];
            b=colourStd[2];
            scale=0.5f;
          }

          mPush();
          mTrans(TX(ship->getX())+x1, TY(ship->getY())+y1);
          mRot(ship->getRotation());
          mUScale(scale);
          shader::quickU uni = {{{r,g,b,1}}};
          shader::quick->activate(&uni);
          glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
          mPop();
        }
      }

      glDisable(GL_STENCIL_TEST);
    }
    //Draw lines
    {
      static GLuint vao=0, vbo;
      static unsigned numLines;
      if (!vao) {
        vao=newVAO();
        glBindVertexArray(vao);

        vbo=newVBO();

        const shader::colourStipleV lines[] = {
          //Toprule
          {{{0,5*fh}}}, {{{1,5*fh}}},
          //Right border for status area
          {{{21*fw,5*fh}}}, {{{21*fw,1*fh}}},
          //Left border for info area
          {{{1 - 41*fw, 5*fh}}}, {{{1 - 41*fw, 1*fh}}},
          //Toprule for target/chat
          {{{7*fw,1*fh}}}, {{{1 - 8*fw, 1*fh}}},
          //Right border for bars, left border for status area and target/chat
          {{{7*fw,5*fh}}}, {{{7*fw,0}}},
          //Left border for minimap
          {{{1 - 8*fw, 5*fh}}}, {{{1 - 8*fw, 0}}},

          //Borders for current weapon indicator
          {{{8*fw, 5*fh}}}, {{{8*fw, 3*fh}}},
          {{{11*fw,5*fh}}}, {{{11*fw,3*fh}}},
          {{{8*fw, 3*fh}}}, {{{11*fw,3*fh}}},
        };
        numLines = lenof(lines);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(lines), lines, GL_STATIC_DRAW);
        shader::colourStiple->setupVBO();
      }

      float dangerBlend = 1;
      if (timeSinceDamage < 500)
        dangerBlend = timeSinceDamage/500;
      shader::colourStipleU uni = { {{ colourStd[0]*dangerBlend + colourDang[0]*(1-dangerBlend),
                                       colourStd[1]*dangerBlend + colourDang[1]*(1-dangerBlend),
                                       colourStd[2]*dangerBlend + colourDang[2]*(1-dangerBlend),
                                       1}}, (int)screenW, (int)screenH };
      glBindVertexArray(vao);
      glBindBuffer(GL_ARRAY_BUFFER, vbo);
      shader::colourStiple->activate(&uni);
      glDrawArrays(GL_LINES, 0, numLines);
    }

    //Draw bars
    {
      static GLuint vao=0, vbo;
      if (!vao) {
        vao=newVAO();
        glBindVertexArray(vao);
        vbo=newVBO();

        const shader::colourStipleV bar[4] = {
          {{{fw*0.1f,0}}}, {{{fw*0.9f,0}}}, {{{fw*0.1f,5*fh}}}, {{{fw*0.9f,5*fh}}},
        };
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(bar), bar, GL_STATIC_DRAW);
        shader::colourStiple->setupVBO();
      }

      glBindVertexArray(vao);
      glBindBuffer(GL_ARRAY_BUFFER, vbo);
      #define DRAW_BAR(ix,val,colour) { \
        mPush(); \
        mTrans(ix*fw+fw/2,0); \
        mScale(1,val); \
        shader::colourStipleU uni = { {{colour[0],colour[1],colour[2],1}}, \
                                      (int)screenW, (int)screenH }; \
        shader::colourStiple->activate(&uni);\
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); \
        mPop(); \
      }
      float greenPower = min(1.0f, s->getPowerUsagePercent());
      DRAW_BAR(0, greenPower, colourStd)
      if (s->getPowerUsagePercent() > 1) {
        float yellowPower = min(1.0f, s->getPowerUsagePercent() - 1);
        DRAW_BAR(0, yellowPower, colourWarn)
      }

      DRAW_BAR(1, s->getCapacitancePercent(), colourStd)
      DRAW_BAR(2, s->getTrueThrust(), colourWarn)
      DRAW_BAR(3, s->getHeatPercent(), colourDang)

      float minShieldStab=65536, minShieldStr=65536;
      const vector<Shield*>& shields(s->getShields());
      for (unsigned i=0; i<shields.size(); ++i) {
        if (shields[i]->getStrength()/shields[i]->getMaxStrength() < minShieldStr)
          minShieldStr = shields[i]->getStrength()/shields[i]->getMaxStrength();
        if (shields[i]->getStability() < minShieldStab)
          minShieldStab = shields[i]->getStability();
      }
      if (minShieldStab <= 1) {
        DRAW_BAR(4, minShieldStr, colourSpec)
        DRAW_BAR(5, minShieldStab, colourSpec)
      }
    }

    //Draw current weapon symbol
    if (showWeaponInfo) {
      mPush();
      mTrans(9.5*fw, 4*fh);
      mScale(1.5f*fw,fh);
      const StatusIcon& icon(statusIcons[string(weaponIconName)]);
      glBindVertexArray(icon.vao);
      glBindBuffer(GL_ARRAY_BUFFER, icon.vbo);
      shader::colourStipleU uni =
        { {{colourStd[0], colourStd[1], colourStd[2], 1}},
          (int)screenW, (int)screenH };
      shader::colourStiple->activate(&uni);
      glDrawArrays(icon.drawMode, 0, icon.length);
      mPop();
    }

    //Draw arrows
    {
      static GLuint vao=0, vbo;
      const float cx = (21*fw + (1 - 24*fw))/2, cy = 3.0f*fh;
      if (!vao) {
        vao=newVAO();
        glBindVertexArray(vao);
        vbo=newVBO();
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        const float rad = 2*fh;
        const shader::colourStipleV vertices[4] = {
          {{{0,0}}},
          {{{rad/2,-fh/3}}},
          {{{rad/2,+fh/3}}},
          {{{rad,0}}},
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        shader::colourStiple->setupVBO();
      }
      glBindVertexArray(vao);
      glBindBuffer(GL_ARRAY_BUFFER, vbo);

      mPush();
      mTrans(cx,cy);
      shader::colourStipleU northUni =
        { {{colourStd[0], colourStd[1], colourStd[2], 1}},
          (int)screenW, (int)screenH };
      shader::colourStipleU targUni =
        { {{colourWarn[0], colourWarn[1], colourWarn[2], 1}},
          (int)screenW, (int)screenH };
      mRot(-visualRotation+pi/2);
      shader::colourStiple->activate(&northUni);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

      if (t) {
        float dx = t->getX() - s->getX(), dy = t->getY() - s->getY();
        mRot(atan2(dy, dx) - pi/2);
        shader::colourStiple->activate(&targUni);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
      }

      mPop();
    }

    mPop();
    mPop(matrix_stack::view);
  } else {
    //Full-screen map
    mPush();
    mPush(matrix_stack::view);
    mId(matrix_stack::view);
    mTrans(-1, -1, matrix_stack::view);
    mUScale(2, matrix_stack::view);
    mConc(matrix(1,0,0,0,
                 0,1/(vheight=screenH/(float)screenW),0,0,
                 0,0,1,0,
                 0,0,0,1), matrix_stack::view);
    #ifdef AB_OPENGL_14
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 1.0, 0.0, vheight=screenH/(float)screenW, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    #endif

    mId();

    glBindVertexArray(triVAO);
    glBindBuffer(GL_ARRAY_BUFFER, triVBO);
    float r,g,b;
    radar_t::iterator it, end;
    s->radarBounds(it,end);
    GameField& field(*s->getField());
    while (it != end) {
      Ship* ship=*it++;
      if (!ship->hasPower()) continue; //Dead

      Alliance alliance = getAlliance(ship->insignia, s->insignia);
      if (ship == s) {
        //Draw self as white
        r=1;
        g=1;
        b=1;
      } else if (ship == t) {
        //Blink
        if (512 < field.fieldClock % 1024) continue;
        r=colourWarn[0];
        g=colourWarn[1];
        b=colourWarn[2];
      } else if (alliance == Allies) {
        r=colourSpec[0];
        g=colourSpec[1];
        b=colourSpec[2];
      } else if (alliance == Enemies) {
        r=colourDang[0];
        g=colourDang[1];
        b=colourDang[2];
      } else /* Neutral */ {
        r=colourStd[0];
        g=colourStd[1];
        b=colourStd[2];
      }

      mPush();
      mTrans(ship->getX()/field.width, ship->getY()*vheight/field.height);
      mRot(ship->getRotation());
      shader::colourStipleU uni = { {{r,g,b,1}}, (int)screenW, (int)screenH };
      shader::colourStiple->activate(&uni);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
      mPop();
    }

    mPop();
    mPop(matrix_stack::view);
  }
  ENDGP
}

void HUD::setRef(Ship* s) noth {
  if (ship.ref == s) return;
  ship.assign(s);
  if (s) {
    originalStrength=0;
    for (unsigned i=0; i<s->cells.size(); ++i)
      originalStrength += s->cells[i]->getMaxDamage();
    currentStrength=originalStrength;
  }
}

void HUD::damage(float amt) noth {
  if (ship.ref) {
    Ship* s = (Ship*)ship.ref;
    currentStrength=0;
    for (unsigned i=0; i<s->cells.size(); ++i)
      currentStrength += s->cells[i]->getMaxDamage() - s->cells[i]->getCurrDamage();
  }
  timeSinceDamage=0;
}

void hud_toggleFullScreenMap_on(Ship* s, ActionDatum& arg) {
  ((HUD*)arg.ptr)->fullScreenMap=true;
}
void hud_toggleFullScreenMap_off(Ship* s, ActionDatum& arg) {
  ((HUD*)arg.ptr)->fullScreenMap=false;
}

void HUD::hc_conf_bind() {
  ActionDatum thisPtr;
  thisPtr.ptr=this;
  DigitalAction toggleFullScreenMap={ thisPtr, false, false, hud_toggleFullScreenMap_on, hud_toggleFullScreenMap_off };
  bind( toggleFullScreenMap, "show fullscreen map" );
}
