
#ifndef _DEFECTREPORT_HH_
#define _DEFECTREPORT_HH_

#include <vector>
#include <string>
#include <cstdint>
#include <system_error>

#include <llvm/Support/YAMLTraits.h>

#include "defect.hh"

namespace dpu
{

struct Defectreport
{
   // tool version
   std::string dpuversion;

   // verification instance + execution environment
   std::string path;
   std::vector<std::string> argv;
   std::vector<std::string> environ;
   uint64_t memsize;
   uint64_t defaultstacksize;
   uint64_t tracesize;
   unsigned optlevel;

   // unfolding exploration
   int alt;
   unsigned kbound;

   // abort + exit(x) with x!=0
   unsigned nr_exitnz;
   unsigned nr_abort;

   // defects
   std::vector<Defect> defects;

   void save (const char *path);
   void add_defect (const Defect &d);
};

} // namespace

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

// yaml trait for the vector of defects
LLVM_YAML_IS_SEQUENCE_VECTOR(dpu::Defect);

// yaml trait for the defect report
template<>
struct llvm::yaml::MappingTraits<dpu::Defectreport> {
   static void mapping (llvm::yaml::IO &io, dpu::Defectreport &r)
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

inline
llvm::raw_ostream &operator<< (llvm::raw_ostream &os, dpu::Defectreport &r)
{
   llvm::yaml::Output out (os);
   out << r;
   return os;
}

#endif
