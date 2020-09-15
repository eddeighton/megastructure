
#include <boost/stacktrace.hpp>

#include <sstream>

std::string stacktraceUtil()
{
    std::ostringstream os;
    
    os << boost::stacktrace::stacktrace();
    
    return os.str();
}