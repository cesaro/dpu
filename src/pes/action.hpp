
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

