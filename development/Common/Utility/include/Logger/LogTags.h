// core yaget library all log tags. If you are a core library developer/maintainer
// add any new tags here. If you are developing executable, dll, add your own tags to existing ones
// by including code snippet below (adjusted for your environment) in one of your cpp files, preferably 'main'
//
//namespace yaget::ylog
//{
//  yaget::Strings GetRegisteredTags()
//  {
//      yaget::Strings tags =
//      {
//          #include "Logger/LogTags.h"
//          "YOUR_TAG_NAME_HERE",
//          "ANOTHER_YOUR_TAG_NAME_HERE",
//      };
//
//      return tags;
//  }
//} // namespace yaget::ylog
//

// hashed values for core log tags
//  INPT = 1414549065
//  PROF = 1179603536
//  APP  =  542134337
//  UTIL = 1279874133
//  METR = 1381254477
//  DEVV = 1448494404
//  FILE = 1162627398
//  IDS  =  542327881
//  INPT = 1414549065
//  DIRE = 1163020612
//  POOL = 1280266064
//  MULT = 1414288717
//  INIT = 1414090313
//  REND = 1145980242
//  SQL  =  541872467
//  IGUI = 1230325577
//  VTS  =  542331990
//  WIN  =  542001495
//  PONG = 1196314448

"MAIN",
"INPT",
"PROF",
"APP",
"UTIL",
"METR",
"DEVV",
"FILE",
"IDS",
"INPT",
"DIRE",
"POOL",
"MULT",
"INIT",
"SQL",
"IGUI",
"VTS",
"WATC",
"CONV",
"WIN",
"WIND",
"GSYS",
"DB", // make sure that last entry has , or you may get compile error or worst a silent false positive
