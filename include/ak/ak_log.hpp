
#if !defined(AK_LOG_FATAL)
#define AK_LOG_FATAL(msg)			do { std::cout << "AK-FATAL: " << msg << std::endl; } while(false)
#endif

#if !defined(AK_LOG_ERROR)
#define AK_LOG_ERROR(msg)			do { std::cout << "AK-ERROR: " << msg << std::endl; } while(false)
#endif

#if !defined(AK_LOG_WARNING)
#define AK_LOG_WARNING(msg)			do { std::cout << "AK-WARNING: " << msg << std::endl; } while(false)
#endif

#if !defined(AK_LOG_INFO)
#define AK_LOG_INFO(msg)			do { std::cout << "AK-INFO: " << msg << std::endl; } while(false)
#endif

#if !defined(AK_LOG_VERBOSE)
#define AK_LOG_VERBOSE(msg)			do { std::cout << "AK-VERBOSE: " << msg << std::endl; } while(false)
#endif

#if !defined(AK_LOG_DEBUG)
#define AK_LOG_DEBUG(msg)			do { std::cout << "AK-DEBUG: " << msg << std::endl; } while(false)
#endif

#if !defined(AK_LOG_DEBUG_VERBOSE)
#define AK_LOG_DEBUG_VERBOSE(msg)	do { std::cout << "AK-DEBUG-VERBOSE: " << msg << std::endl; } while(false)
#endif
