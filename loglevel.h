#ifndef LOG_LEVEL_H
#define LOG_LEVEL_H

#if(LogLevel ==  0)
#define log_error( x ) std::cout<< "[Error:]" << __FUNCTION__ << " " << __FILE__ << " +" << __LINE__ << "  ###" << x << "###" << std::endl << std::endl;
#define log_info( x ) std::cout<< "[info:]" << __FUNCTION__ << " " << __FILE__ << " +" << __LINE__ << "  ###" << x << "###" << std::endl << std::endl;
#define log_debug( x ) std::cout<< "[debug:]" << __FUNCTION__ << " " << __FILE__ <<  " +" << __LINE__ << "  ###" << x << "###" << std::endl << std::endl;
#elif(LogLevel == 1)
#define log_error( x ) std::cout<< "[Error:]" << __FUNCTION__ << " " << __FILE__ <<  " +" << __LINE__ << "  ###" << x << "###" << std::endl << std::endl;
#define log_info( x ) std::cout<< "[info:]" << __FUNCTION__ << " " << __FILE__ << " +" << __LINE__ << "  ###" << x << "###" << std::endl << std::endl;
#define log_debug( x )
#elif(LogLevel == 2)
#define log_error( x ) std::cout<< "[Error:]" << __FUNCTION__ << " " << __FILE__ <<  " +" << __LINE__ << "  ###" << x << "###" << std::endl << std::endl;
#define log_info( x )
#define log_debug( x )
#else
#define log_error( x )
#define log_info( x )
#define log_debug( x )
#endif


#endif
