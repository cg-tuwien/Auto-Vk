
#if !defined(AVK_LOG_FATAL)
#define AVK_LOG_FATAL(msg)			do { std::cout << "AVK-FATAL: " << msg << std::endl; } while(false)
#endif

#if !defined(AVK_LOG_ERROR)
#define AVK_LOG_ERROR(msg)			do { std::cout << "AVK-ERROR: " << msg << std::endl; } while(false)
#endif

#if !defined(AVK_LOG_WARNING)
#define AVK_LOG_WARNING(msg)			do { std::cout << "AVK-WARNING: " << msg << std::endl; } while(false)
#endif

#if !defined(AVK_LOG_INFO)
#define AVK_LOG_INFO(msg)			do { std::cout << "AVK-INFO: " << msg << std::endl; } while(false)
#endif

#if !defined(AVK_LOG_VERBOSE)
#define AVK_LOG_VERBOSE(msg)			do { std::cout << "AVK-VERBOSE: " << msg << std::endl; } while(false)
#endif

#if !defined(AVK_LOG_DEBUG)
#define AVK_LOG_DEBUG(msg)			do { std::cout << "AVK-DEBUG: " << msg << std::endl; } while(false)
#endif

#if !defined(AVK_LOG_DEBUG_VERBOSE)
#define AVK_LOG_DEBUG_VERBOSE(msg)	do { std::cout << "AVK-DEBUG-VERBOSE: " << msg << std::endl; } while(false)
#endif
