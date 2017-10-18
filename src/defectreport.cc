
#include <vector>
#include <system_error>

#include "llvm/Support/raw_ostream.h"

#include "verbosity.h"
#include "defect.hh"
#include "defectreport.hh"

namespace dpu
{

void Defectreport::save (const char *path)
{
   std::error_code ec;
   llvm::raw_fd_ostream f (path, ec, llvm::sys::fs::OpenFlags::F_None);
   if (ec.value() != 0)
      throw std::system_error (ec, path);
   llvm::yaml::Output out (f);
   out << *this;
}

void Defectreport::add_defect (const Defect &d)
{
   //PRINT ("dpu: defect found: %s", d.description.c_str());
   defects.emplace_back (d);
}

} // namespace
