#ifndef _DATARACEANALYSIS_HH_
#define _DATARACEANALYSIS_HH_

#include "stid/replay.hh"

#include "pes/action.hh"
#include "pes/config.hh"

#include "unfolder/replay.hh"
#include "unfolder/stream-converter.hh"
#include "unfolder/unfolder.hh"

#include "memory-region.hh"
#include "datarace.hh"

namespace dpu {

class DataRaceAnalysis;

template<>
struct StreamConverterTraits<DataRaceAnalysis>
{
protected:
   /// A factory for and container of the Redboxes that label blue events in the
   /// Unfolding constructed by the DataRaceAnalysis.
   RedboxFactory redboxfac;

   /// The next event where StreamConverter<DataRaceAnalysis> will write a
   /// Redbox.
   Event *blue;
};

class DataRaceAnalysis : public Unfolder<DataRaceAnalysis>
{
public :

   /// A pointer to a DataRace object, if one data race is ever found.
   DataRace *race;

   /// The list of all defects found during the exploration.
   Defectreport report;

   /// Stores the liset of program executions where we are going to search for
   /// data races.
   std::vector<stid::Replay> replays;

   /// Maximum number of seconds that the analysis is able to work. Set to 0 to
   /// disable.
   unsigned timeout;

public:
   /// Constructor
   DataRaceAnalysis () :
      Unfolder (prepare_executor_config ()),
      race (nullptr),
      report (),
      replays ()
   {}

   /// Initializes the fields of a Defectreport with the parameters of this
   /// C15unfolder. This method can be overloaded in subclasses. Those overloadings
   /// will, probably call to this implementation to fill the report fields with
   /// the parmeters stored in this class, and then add others possibly stored
   /// in the subclasses.
   void report_init ();

   /// Perform the analysis.
   void run ();

   /// Returns true iff configuration \p has two racy events.
   static DataRace *check_data_races (const Config &c);

private :

   /// Initialize and return the parameters of a stid::Executor
   stid::ExecutorConfig prepare_executor_config () const;
};

// implementation of inline methods, outside of the namespace
#include "datarace-analysis.hpp"

} // namespace

#endif
