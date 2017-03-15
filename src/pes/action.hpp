
bool Action::operator== (const Action &other) const
{
   switch (type)
   {
   // loads, stores
   case ActionType::RD8 :
   case ActionType::RD16 :
   case ActionType::RD32 :
   case ActionType::RD64 :
   case ActionType::WR8 :
   case ActionType::WR16 :
   case ActionType::WR32 :
   case ActionType::WR64 :
   // malloc
   case ActionType::MALLOC :
      return type == other.type and addr == other.addr and val == other.val;

   // free and locks
   case ActionType::FREE :
   case ActionType::MTXLOCK :
   case ActionType::MTXUNLK :
      return type == other.type and addr == other.addr;

   // threads
   case ActionType::THCREAT :
   case ActionType::THJOIN :
      return type == other.type and val == other.val;

   // threads
   case ActionType::THSTART :
   case ActionType::THEXIT :
      return type == other.type;

   default :
      ASSERT (0);
   }
};

inline static std::ostream & operator<< (std::ostream &os, const enum ActionType &a)
{
   switch (a)
   {
   // loads, stores
   case ActionType::RD8 :
      os << "RD8";
      return os;
   case ActionType::RD16 :
      os << "RD16";
      return os;
   case ActionType::RD32 :
      os << "RD32";
      return os;
   case ActionType::RD64 :
      os << "RD64";
      return os;
   case ActionType::WR8 :
      os << "WR8";
      return os;
   case ActionType::WR16 :
      os << "WR16";
      return os;
   case ActionType::WR32 :
      os << "WR32";
      return os;
   case ActionType::WR64 :
      os << "WR64";
      return os;

   // malloc
   case ActionType::MALLOC :
      os << "MALLOC";
      return os;
   case ActionType::FREE :
      os << "FREE";
      return os;

   // locks
   case ActionType::MTXLOCK :
      os << "MTXLOCK";
      return os;
   case ActionType::MTXUNLK :
      os << "MTXUNLK";
      return os;

   // threads
   case ActionType::THCREAT :
      os << "THCREAT";
      return os;
   case ActionType::THJOIN :
      os << "THJOIN";
      return os;

   // threads
   case ActionType::THSTART :
      os << "THSTART";
      return os;
   case ActionType::THEXIT :
      os << "THEXIT";
      return os;

   default :
      ASSERT (0);
      os << "??";
      return os;
   }
}
