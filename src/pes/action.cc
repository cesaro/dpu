
#include "pes/action.hh"

namespace dpu {

const char *action_type_str (unsigned t)
{
   return action_type_str ((ActionType) t);
}

const char *action_type_str (ActionType t)
{
   switch (t)
   {
   // loads
   case ActionType::RD8       : return "RD8";
   case ActionType::RD16      : return "RD16";
   case ActionType::RD32      : return "RD32";
   case ActionType::RD64      : return "RD64";
   // stores
   case ActionType::WR8       : return "WR8";
   case ActionType::WR16      : return "WR16";
   case ActionType::WR32      : return "WR32";
   case ActionType::WR64      : return "WR64";
   // memory management
   case ActionType::MALLOC    : return "MALLOC";
   case ActionType::FREE      : return "FREE";
   // threads
   case ActionType::THCREAT   : return "THCREAT";
   case ActionType::THSTART   : return "THSTART";
   case ActionType::THEXIT    : return "THEXIT";
   case ActionType::THJOIN    : return "THJOIN";
   // locks
   //case ActionType::MTXINIT   : return "MTX-INIT";
   case ActionType::MTXLOCK   : return "MTX-LOCK";
   case ActionType::MTXUNLK   : return "MTX-UNLK";
   }
}

void Action::pretty_print ()
{
   // MALLOC   0x1122334411223344, 0x1122334411223344B
   // FREE     0x182391293
   // WR64     *0x1122334411223344 =  0x1122334411223344
   // RD64     *0x1122334411223344 == 0x1122334411223344
   // THCREAT  123
   // THSTART  123
   // THJOIN   123
   // THEXIT   
   // MTX-INIT 0x1122334411223344, 0x1133
   // MTX-LOCK 0x1122334411223344
   // MTX-UNLK 0x1122334411223344

   const char *eq = "";
   switch (type)
   {
   // loads
   case ActionType::RD8       :
   case ActionType::RD16      :
   case ActionType::RD32      :
   case ActionType::RD64      :
      eq = "=";

   // stores
   case ActionType::WR8       :
   case ActionType::WR16      :
   case ActionType::WR32      :
   case ActionType::WR64      :
      printf ("%s *%#-18lx =%s %#-18lx\n",
            action_type_str (type), addr, eq, val);
      break;

   case ActionType::MALLOC    :
   //case ActionType::MTXINIT   :
      printf ("%s %#-18lx, %#-18lx\n", action_type_str (type), addr, val);
      break;

   case ActionType::FREE      :
   case ActionType::MTXLOCK   :
   case ActionType::MTXUNLK   :
      printf ("%s %#-18lx\n", action_type_str (type), addr);
      break;

   case ActionType::THCREAT   :
   case ActionType::THSTART   :
   case ActionType::THEXIT    :
   case ActionType::THJOIN    :
      printf ("%s %u\n", action_type_str (type), (unsigned) val);
      break;
   }
}

} // namespace
