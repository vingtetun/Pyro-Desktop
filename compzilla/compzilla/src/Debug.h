
#ifdef DEBUG
#ifdef WITH_SPEW
#define SPEW(format...) printf("   - " format)
#else
#define SPEW(format...)
#endif
#define INFO(format...) printf(" *** " format)
#define WARNING(format...) printf(" !!! " format)
#define ERROR(format...) fprintf(stderr, format)
#else
#define SPEW(format...) do { } while (0)
#define INFO(format...) do { } while (0)
#define WARNING(format...) do { } while (0)
#define ERROR(format...) do { } while (0)
#endif

#ifdef DEBUG_EVENTS
#define SPEW_EVENT(format...) printf("  [EVENT] " format)
#else
#define SPEW_EVENT(format...) do { } while (0)
#endif
