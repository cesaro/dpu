
#ifndef _C15U_DEFECTREPORT_HH_
#define _C15U_DEFECTREPORT_HH_

#include <vector>
#include <string>
#include <cstdint>
#include <system_error>

#include "llvm/Support/raw_ostream.h"

#include "defect.hh"

struct Defectreport
{
   std::string dpuversion;
   std::string path;
   std::vector<std::string> argv;
   std::vector<std::string> environ;
   int alt;
   unsigned kbound;
   uint64_t memsize;
   uint64_t defaultstacksize;
   uint64_t tracesize;
   unsigned optlevel;

   std::vector<Defect> defects;

   inline void save (const char *path);
};

// yaml trait for "const char *" (for some reason it doesn't work)
template<>
struct llvm::yaml::ScalarTraits<const char *> {
   static void output (const char * &str, void *ctx, llvm::raw_ostream &out)
   {
      out << str;
   }
   static llvm::StringRef input (llvm::StringRef scalar, void *ctx, const char * &str)
   {
      return "Parsing unimplemented!!!";
   }
   bool mustQuote (llvm::StringRef str)
   {
      return true;
   }
};

// yaml trait for the argv/environ vectors
LLVM_YAML_IS_SEQUENCE_VECTOR(std::string);

// yaml trait for the vector of defects
LLVM_YAML_IS_SEQUENCE_VECTOR(Defect);

// yaml trait for the defect report
template<>
struct llvm::yaml::MappingTraits<Defectreport> {
   static void mapping (llvm::yaml::IO &io, Defectreport &r)
   {
      io.mapRequired ("dpu-version", r.dpuversion);
      io.mapRequired ("bitcode", r.path);
      io.mapRequired ("argv", r.argv);
      io.mapRequired ("environ", r.environ);
      io.mapRequired ("alt", r.alt);
      io.mapRequired ("kbound", r.kbound);
      io.mapRequired ("mem-size", r.memsize);
      io.mapRequired ("default-stack-size", r.defaultstacksize);
      io.mapRequired ("trace-size", r.tracesize);
      io.mapRequired ("optlevel", r.optlevel);
      io.mapRequired ("defects", r.defects);
   }
};

inline void Defectreport::save (const char *path)
{
   std::error_code ec;
   llvm::raw_fd_ostream f (path, ec, llvm::sys::fs::OpenFlags::F_None);
   if (ec.value() != 0)
      throw std::system_error (ec, path);
   llvm::yaml::Output out (f);
   out << *this;
}
#endif
