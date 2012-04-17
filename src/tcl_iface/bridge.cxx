
  /**
   * @file
   * @author C++-Tcl Bridge Code Generator
   * @brief Autogenerated bindings; <b>not intended for human consumption</b>
   *
   * AUTOGENERATED BY generate.tcl. DO NOT EDIT DIRECTLY.
   * @see src/tcl_iface/readme.txt
   */

  #include <map>
  #include <set>
  #include <vector>
  #include <string>
  #include <cstring>
  #include <cstdio>
  #include <cstdlib>
  #include <iostream>

  #include <GL/gl.h>
  #include <SDL.h>
  #include <tcl.h>
  #include <itcl.h>
  #include <libconfig.h++>

  #include "src/tcl_iface/bridge.hxx"
  #include "src/tcl_iface/implementation.hxx"
  #include "src/tcl_iface/dynfun.hxx"
  #include "src/exit_conditions.hxx"
  #include "src/globals.hxx"

  #pragma GCC diagnostic ignored "-Wunused-label"
  #pragma GCC diagnostic ignored "-Waddress"
  using namespace std;
  using namespace tcl_glue_implementation;
  using namespace libconfig;

  //Commands get their zeroth argument as their own name;
  //code generation is simpler if we drop this
  #define SHIFT ++objv, --objc

class Ship;
class NetworkConnection;
class InputNetworkGeraet;
int trampoline1(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline3(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline5(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline7(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline9(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline11(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline13(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline15(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline17(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline19(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline21(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline23(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline25(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();

int trampoline27(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline29(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline31(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline33(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline35(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline37(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline39(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline41(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline43(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline45(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline47(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline49(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline51(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline53(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline55(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline57(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int get59(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get61(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get63(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get65(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get67(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get69(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get71(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get73(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get75(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get77(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get79(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get81(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get83(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
void classdec85(bool,Tcl_Interp*) throw();
void classdec98(bool,Tcl_Interp*) throw();
int get108(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get110(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get112(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get114(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get116(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int trampoline118(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int get120(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get122(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int trampoline124(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();

void classdec126(bool,Tcl_Interp*) throw();
void classdec145(bool,Tcl_Interp*) throw();
int trampoline164(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline166(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();

void classdec168(bool,Tcl_Interp*) throw();
int get189(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get191(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
void classdec193(bool,Tcl_Interp*) throw();
void classdec208(bool,Tcl_Interp*) throw();

int get211(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);int set212(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get214(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);int set215(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get217(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);int set218(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get220(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);int set221(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
void classdec223(bool,Tcl_Interp*) throw();

void classdec230(bool,Tcl_Interp*) throw();
void classdec231(bool,Tcl_Interp*) throw();
void classdec306(bool,Tcl_Interp*) throw();
void classdec307(bool,Tcl_Interp*) throw();
void classdec344(bool,Tcl_Interp*) throw();
void classdec379(bool,Tcl_Interp*) throw();
void classdec386(bool,Tcl_Interp*) throw();
void classdec393(bool,Tcl_Interp*) throw();

void classdec417(bool,Tcl_Interp*) throw();

void classdec426(bool,Tcl_Interp*) throw();

void classdec439(bool,Tcl_Interp*) throw();
void classdec440(bool,Tcl_Interp*) throw();
void classdec441(bool,Tcl_Interp*) throw();
void classdec442(bool,Tcl_Interp*) throw();
void classdec443(bool,Tcl_Interp*) throw();

void classdec444(bool,Tcl_Interp*) throw();
void classdec453(bool,Tcl_Interp*) throw();
void classdec456(bool,Tcl_Interp*) throw();
int trampoline584(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline586(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline588(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();

void classdec590(bool,Tcl_Interp*) throw();

int trampoline609(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline611(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();

int trampoline613(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline615(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();

void classdec617(bool,Tcl_Interp*) throw();
void classdec630(bool,Tcl_Interp*) throw();
void classdec643(bool,Tcl_Interp*) throw();
void classdec646(bool,Tcl_Interp*) throw();
void classdec657(bool,Tcl_Interp*) throw();
void classdec660(bool,Tcl_Interp*) throw();
void classdec663(bool,Tcl_Interp*) throw();
int get672(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
void classdec674(bool,Tcl_Interp*) throw();

void classdec683(bool,Tcl_Interp*) throw();

void classdec686(bool,Tcl_Interp*) throw();
int trampoline689(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();

void classdec691(bool,Tcl_Interp*) throw();

void classdec708(bool,Tcl_Interp*) throw();

void classdec724(bool,Tcl_Interp*) throw();

void classdec749(bool,Tcl_Interp*) throw();

void classdec752(bool,Tcl_Interp*) throw();

void classdec765(bool,Tcl_Interp*) throw();
int get778(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get780(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);int set781(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);

int trampoline783(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline785(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();

void classdec787(bool,Tcl_Interp*) throw();

void classdec792(bool,Tcl_Interp*) throw();
int trampoline805(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
void classdec807(bool,Tcl_Interp*) throw();
int trampoline816(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();

void classdec818(bool,Tcl_Interp*) throw();

void classdec839(bool,Tcl_Interp*) throw();
void classdec842(bool,Tcl_Interp*) throw();
void classdec857(bool,Tcl_Interp*) throw();

void classdec878(bool,Tcl_Interp*) throw();

int get923(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get925(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
void classdec927(bool,Tcl_Interp*) throw();
int trampoline1026(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline1028(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline1030(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline1032(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline1034(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline1036(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();

int trampoline1038(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline1040(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline1042(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline1044(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int get1046(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);int set1047(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);

int get1049(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get1051(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get1053(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get1055(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get1057(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);int set1058(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get1060(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);int set1061(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get1063(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);int set1064(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get1066(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);int set1067(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get1069(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);int set1070(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);

int get1072(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get1074(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);int set1075(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get1077(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);int set1078(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get1080(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);int set1081(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get1083(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);int set1084(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get1086(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);int set1087(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get1089(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);int set1090(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get1092(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);int set1093(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get1095(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);int set1096(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get1098(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);int set1099(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get1101(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);int set1102(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get1104(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);int set1105(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);

int get1107(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get1109(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get1111(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get1113(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get1115(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get1117(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
void classdec1119(bool,Tcl_Interp*) throw();

void classdec1132(bool,Tcl_Interp*) throw();
void classdec1133(bool,Tcl_Interp*) throw();
void classdec1134(bool,Tcl_Interp*) throw();
int get1154(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int get1156(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);int set1157(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);

void classdec1159(bool,Tcl_Interp*) throw();

void classdec1162(bool,Tcl_Interp*) throw();

void classdec1167(bool,Tcl_Interp*) throw();
void classdec1168(bool,Tcl_Interp*) throw();

void classdec1169(bool,Tcl_Interp*) throw();

void classdec1194(bool,Tcl_Interp*) throw();
void classdec1195(bool,Tcl_Interp*) throw();

void classdec1211(bool,Tcl_Interp*) throw();

void classdec1212(bool,Tcl_Interp*) throw();
void classdec1213(bool,Tcl_Interp*) throw();

void classdec1214(bool,Tcl_Interp*) throw();

void classdec1223(bool,Tcl_Interp*) throw();

void classdec1226(bool,Tcl_Interp*) throw();

void classdec1235(bool,Tcl_Interp*) throw();

void classdec1244(bool,Tcl_Interp*) throw();

void classdec1245(bool,Tcl_Interp*) throw();
void classdec1250(bool,Tcl_Interp*) throw();

void classdec1255(bool,Tcl_Interp*) throw();
void classdec1268(bool,Tcl_Interp*) throw();
void classdec1297(bool,Tcl_Interp*) throw();

void classdec1348(bool,Tcl_Interp*) throw();
int get1443(ClientData,Tcl_Interp*,int,Tcl_Obj*const[]);
int trampoline1445(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline1447(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline1449(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline1451(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();

int trampoline1453(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline1455(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline1457(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline1459(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline1461(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();

int trampoline1463(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline1465(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline1467(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline1469(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline1471(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline1473(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline1475(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline1477(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline1479(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();

int trampoline1481(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline1483(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline1485(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();

int trampoline1487(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline1489(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline1491(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();

void classdec1493(bool,Tcl_Interp*) throw();

int trampoline1496(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();
int trampoline1498(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();

int trampoline1500(ClientData,Tcl_Interp*,int,Tcl_Obj*const[])throw();


  Tcl_Interp* newInterpreter(bool safe, Tcl_Interp* master) {
    Tcl_Interp* interp = newInterpreterImpl(safe,master);
    //Autodecs follow

Tcl_CreateObjCommand(interp, "c++ trampoline1", trampoline1, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline3", trampoline3, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline5", trampoline5, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline7", trampoline7, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline9", trampoline9, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline11", trampoline11, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline13", trampoline13, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline15", trampoline15, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline17", trampoline17, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline19", trampoline19, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline21", trampoline21, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline23", trampoline23, 0, NULL);
if (!safe) {
Tcl_CreateObjCommand(interp, "c++ trampoline25", trampoline25, 0, NULL);
}
Tcl_CreateObjCommand(interp, "c++ trampoline27", trampoline27, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline29", trampoline29, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline31", trampoline31, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline33", trampoline33, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline35", trampoline35, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline37", trampoline37, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline39", trampoline39, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline41", trampoline41, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline43", trampoline43, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline45", trampoline45, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline47", trampoline47, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline49", trampoline49, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline51", trampoline51, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline53", trampoline53, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline55", trampoline55, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline57", trampoline57, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get59", get59, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get61", get61, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get63", get63, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get65", get65, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get67", get67, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get69", get69, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get71", get71, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get73", get73, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get75", get75, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get77", get77, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get79", get79, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get81", get81, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get83", get83, 0, NULL);
classdec85(safe, interp);
classdec98(safe, interp);
Tcl_CreateObjCommand(interp, "c++ get108", get108, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get110", get110, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get112", get112, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get114", get114, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get116", get116, 0, NULL);
if (!safe) {
Tcl_CreateObjCommand(interp, "c++ trampoline118", trampoline118, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get120", get120, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get122", get122, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline124", trampoline124, 0, NULL);
}
classdec126(safe, interp);
classdec145(safe, interp);
if (!safe) {
Tcl_CreateObjCommand(interp, "c++ trampoline164", trampoline164, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline166", trampoline166, 0, NULL);
}
classdec168(safe, interp);
Tcl_CreateObjCommand(interp, "c++ get189", get189, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get191", get191, 0, NULL);
classdec193(safe, interp);
if (!safe) {
classdec208(safe, interp);
}
Tcl_CreateObjCommand(interp, "c++ get211", get211, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ set212", set212, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get214", get214, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ set215", set215, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get217", get217, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ set218", set218, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get220", get220, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ set221", set221, 0, NULL);
if (!safe) {
classdec223(safe, interp);
}

classdec231(safe, interp);

classdec307(safe, interp);
classdec344(safe, interp);
classdec379(safe, interp);
classdec386(safe, interp);
if (!safe) {
classdec393(safe, interp);
}
if (!safe) {
classdec417(safe, interp);
}
if (!safe) {
classdec426(safe, interp);
}





{
      TypeExport* ex=new TypeExport(typeid(DynFun2<void,Ship*,bool>::fun_t));
      ex->isAObject=false;
      ex->tclClassName = "fun<void:Ship*,bool>::fun_t";
      typeExports[&typeid(DynFun2<void,Ship*,bool>::fun_t)]=ex;
    }
classdec444(safe, interp);
classdec453(safe, interp);
classdec456(safe, interp);
Tcl_CreateObjCommand(interp, "c++ trampoline584", trampoline584, 0, NULL);
if (!safe) {
Tcl_CreateObjCommand(interp, "c++ trampoline586", trampoline586, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline588", trampoline588, 0, NULL);
}
if (!safe) {
classdec590(safe, interp);
}
Tcl_CreateObjCommand(interp, "c++ trampoline609", trampoline609, 0, NULL);
if (!safe) {
Tcl_CreateObjCommand(interp, "c++ trampoline611", trampoline611, 0, NULL);
}
Tcl_CreateObjCommand(interp, "c++ trampoline613", trampoline613, 0, NULL);
if (!safe) {
Tcl_CreateObjCommand(interp, "c++ trampoline615", trampoline615, 0, NULL);
}
classdec617(safe, interp);
classdec630(safe, interp);
classdec643(safe, interp);
classdec646(safe, interp);
classdec657(safe, interp);
classdec660(safe, interp);
classdec663(safe, interp);
Tcl_CreateObjCommand(interp, "c++ get672", get672, 0, NULL);
if (!safe) {
classdec674(safe, interp);
}
if (!safe) {
classdec683(safe, interp);
}
if (!safe) {
classdec686(safe, interp);
Tcl_CreateObjCommand(interp, "c++ trampoline689", trampoline689, 0, NULL);
}
if (!safe) {
classdec691(safe, interp);
}
if (!safe) {
classdec708(safe, interp);
}
if (!safe) {
classdec724(safe, interp);
}
if (!safe) {
classdec749(safe, interp);
}
if (!safe) {
classdec752(safe, interp);
}
if (!safe) {
classdec765(safe, interp);
Tcl_CreateObjCommand(interp, "c++ get778", get778, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get780", get780, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ set781", set781, 0, NULL);
}
if (!safe) {
Tcl_CreateObjCommand(interp, "c++ trampoline783", trampoline783, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline785", trampoline785, 0, NULL);
}
if (!safe) {
classdec787(safe, interp);
}
if (!safe) {
classdec792(safe, interp);
Tcl_CreateObjCommand(interp, "c++ trampoline805", trampoline805, 0, NULL);
classdec807(safe, interp);
Tcl_CreateObjCommand(interp, "c++ trampoline816", trampoline816, 0, NULL);
}
if (!safe) {
classdec818(safe, interp);
}
classdec839(safe, interp);
classdec842(safe, interp);
if (!safe) {
classdec857(safe, interp);
}
if (!safe) {
classdec878(safe, interp);
}
if (!safe) {
Tcl_CreateObjCommand(interp, "c++ get923", get923, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get925", get925, 0, NULL);
classdec927(safe, interp);
Tcl_CreateObjCommand(interp, "c++ trampoline1026", trampoline1026, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline1028", trampoline1028, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline1030", trampoline1030, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline1032", trampoline1032, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline1034", trampoline1034, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline1036", trampoline1036, 0, NULL);
}
Tcl_CreateObjCommand(interp, "c++ trampoline1038", trampoline1038, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline1040", trampoline1040, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline1042", trampoline1042, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline1044", trampoline1044, 0, NULL);
if (!safe) {
Tcl_CreateObjCommand(interp, "c++ get1046", get1046, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ set1047", set1047, 0, NULL);
}
Tcl_CreateObjCommand(interp, "c++ get1049", get1049, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get1051", get1051, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get1053", get1053, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get1055", get1055, 0, NULL);
if (!safe) {
Tcl_CreateObjCommand(interp, "c++ get1057", get1057, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ set1058", set1058, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get1060", get1060, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ set1061", set1061, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get1063", get1063, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ set1064", set1064, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get1066", get1066, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ set1067", set1067, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get1069", get1069, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ set1070", set1070, 0, NULL);
}
Tcl_CreateObjCommand(interp, "c++ get1072", get1072, 0, NULL);
if (!safe) {
Tcl_CreateObjCommand(interp, "c++ get1074", get1074, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ set1075", set1075, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get1077", get1077, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ set1078", set1078, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get1080", get1080, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ set1081", set1081, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get1083", get1083, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ set1084", set1084, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get1086", get1086, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ set1087", set1087, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get1089", get1089, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ set1090", set1090, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get1092", get1092, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ set1093", set1093, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get1095", get1095, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ set1096", set1096, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get1098", get1098, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ set1099", set1099, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get1101", get1101, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ set1102", set1102, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get1104", get1104, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ set1105", set1105, 0, NULL);
}
Tcl_CreateObjCommand(interp, "c++ get1107", get1107, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get1109", get1109, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get1111", get1111, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get1113", get1113, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get1115", get1115, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get1117", get1117, 0, NULL);
if (!safe) {
classdec1119(safe, interp);
}
if (!safe) {


classdec1134(safe, interp);
Tcl_CreateObjCommand(interp, "c++ get1154", get1154, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ get1156", get1156, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ set1157", set1157, 0, NULL);
}
if (!safe) {
classdec1159(safe, interp);
}
if (!safe) {
classdec1162(safe, interp);
}
if (!safe) {
classdec1167(safe, interp);

}
if (!safe) {
classdec1169(safe, interp);
}
if (!safe) {

classdec1195(safe, interp);
}
if (!safe) {
classdec1211(safe, interp);
}
if (!safe) {
classdec1212(safe, interp);
classdec1213(safe, interp);
{
      TypeExport* ex=new TypeExport(typeid(DynFun1<InputNetworkGeraet*,NetworkConnection*>::fun_t));
      ex->isAObject=false;
      ex->tclClassName = "fun<InputNetworkGeraet*:NetworkConnection*>::fun_t";
      typeExports[&typeid(DynFun1<InputNetworkGeraet*,NetworkConnection*>::fun_t)]=ex;
    }
classdec1214(safe, interp);
}
if (!safe) {
classdec1223(safe, interp);
}
if (!safe) {
classdec1226(safe, interp);
}
if (!safe) {
classdec1235(safe, interp);
}
if (!safe) {

}
if (!safe) {
classdec1245(safe, interp);
classdec1250(safe, interp);
}
if (!safe) {
classdec1255(safe, interp);
classdec1268(safe, interp);
classdec1297(safe, interp);
}
classdec1348(safe, interp);
Tcl_CreateObjCommand(interp, "c++ get1443", get1443, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline1445", trampoline1445, 0, NULL);
if (!safe) {
Tcl_CreateObjCommand(interp, "c++ trampoline1447", trampoline1447, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline1449", trampoline1449, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline1451", trampoline1451, 0, NULL);
}
Tcl_CreateObjCommand(interp, "c++ trampoline1453", trampoline1453, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline1455", trampoline1455, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline1457", trampoline1457, 0, NULL);
if (!safe) {
Tcl_CreateObjCommand(interp, "c++ trampoline1459", trampoline1459, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline1461", trampoline1461, 0, NULL);
}
Tcl_CreateObjCommand(interp, "c++ trampoline1463", trampoline1463, 0, NULL);
if (!safe) {
Tcl_CreateObjCommand(interp, "c++ trampoline1465", trampoline1465, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline1467", trampoline1467, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline1469", trampoline1469, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline1471", trampoline1471, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline1473", trampoline1473, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline1475", trampoline1475, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline1477", trampoline1477, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline1479", trampoline1479, 0, NULL);
}
if (!safe) {
Tcl_CreateObjCommand(interp, "c++ trampoline1481", trampoline1481, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline1483", trampoline1483, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline1485", trampoline1485, 0, NULL);
}
if (!safe) {
Tcl_CreateObjCommand(interp, "c++ trampoline1487", trampoline1487, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline1489", trampoline1489, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline1491", trampoline1491, 0, NULL);
}
if (!safe) {
classdec1493(safe, interp);
}
if (!safe) {
Tcl_CreateObjCommand(interp, "c++ trampoline1496", trampoline1496, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline1498", trampoline1498, 0, NULL);
}
if (!safe) {
Tcl_CreateObjCommand(interp, "c++ trampoline1500", trampoline1500, 0, NULL);
}
newInterpreterImplPost(interp);
invokingInterpreter=master;
return interp; }
