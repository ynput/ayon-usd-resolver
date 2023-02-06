#include "AYON_Logging.h"
#include <iostream>

void AYON_LogUtils::WriteToStream(const std::string& Caption, const std::string& Body)
{
    std::cerr << std::endl
        << SEPARATOR << std::endl
        << Caption << std::endl
        << SEPARATOR << std::endl
        << Body << std::endl
        << SEPARATOR << std::endl
        << std::endl;
}
