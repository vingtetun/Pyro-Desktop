
#include <prlog.h>

extern PRLogModuleInfo *compzillaLog;

#ifdef DEBUG
#define INFO(format...)    PR_LOG(compzillaLog, PR_LOG_WARNING, (" *** " format))
#define WARNING(format...) PR_LOG(compzillaLog, PR_LOG_WARNING, (" !!! " format))
#define ERROR(format...)   PR_LOG(compzillaLog, PR_LOG_ERROR,   (format))
#else
#define INFO(format...) do { } while (0)
#define WARNING(format...) do { } while (0)
#define ERROR(format...) do { } while (0)
#endif

#ifdef DEBUG_SPEW
#define SPEW(format...) PR_LOG(compzillaLog, PR_LOG_DEBUG, ("   - " format))
#else
#define SPEW(format...) do { } while (0)
#endif

#ifdef DEBUG_EVENTS
#define SPEW_EVENT(format...) PR_LOG(compzillaLog, PR_LOG_DEBUG, ("  [EVENT] " format))
#else
#define SPEW_EVENT(format...) do { } while (0)
#endif
