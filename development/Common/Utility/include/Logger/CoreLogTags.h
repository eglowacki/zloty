// core yaget library all log tags. If you are a core library developer/maintainer
// add any new tags here. If you are developing executable, dll, add your own tags to existing ones
// by including code snippet below (adjusted for your environment) in one of your cpp files, preferably 'main'
//
//  yaget::Strings yaget::ylog::GetRegisteredTags()
//  {
//      yaget::Strings tags =
//      {
//          #include "Logger/CoreLogTags.h"
//          "YOUR_TAG_NAME_HERE",
//          "ANOTHER_YOUR_TAG_NAME_HERE",
//      };
//
//      return tags;
//  }
//

// hashed values for core log tags
//  APP  =    5263425
//  ASET = 1413829441
//  CLNT = 1414417475
//  CORE = 1163022147
//  DB   = 4261429828
//  DEVV = 1448494404
//  DIRE = 1163020612
//  FILE = 1162627398
//  GSYS = 1398362951
//  IDS  =    5456969
//  IGUI = 1230325577
//  INIT = 1414090313
//  INPT = 1414549065
//  METR = 1381254477
//  MULT = 1414288717
//  POOL = 1280266064
//  PROF = 1179603536
//  SPAM = 1296126035
//  VTS  =    5461078
//  VTSD = 1146311766
//  WATC = 1129595223
//  WIN  =    5130583

"CORE",
"INPT",
"PROF",
"APP",
"METR",
"DEVV",
"FILE",
"IDS",
"DIRE",
"POOL",
"MULT",
"INIT",
"DB",
"IGUI",
"VTS",
"VTSD",
"WATC",
"WIN",
"GSYS",
"SYSC",
"ASET", // make sure that last entry has , or you may get compile error or worst a silent false positive
