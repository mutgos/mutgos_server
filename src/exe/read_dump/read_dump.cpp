/*
 * read_dump.cpp
 */

#include <string>
#include <iostream>
#include "text/text_StringConversion.h"

#include "logging/log_Logger.h"
#include "dbdump/dbdump_MutgosDumpFileReader.h"

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cerr << "Syntax is: " << argv[0] << " <dump_file>" << std::endl;
        return -1;
    }

    mutgos::log::Logger::init(true);

    const std::string dump_file = argv[1];
    std::string message;
    mutgos::dbdump::MutgosDumpFileReader reader(dump_file);

    if (reader.parse(message))
    {
        std::cout << "Success: Parsing complete." << std::endl
                  << "  Message: " << message << std::endl;
    }
    else
    {
        std::cerr << "FAILURE: Parsing did NOT complete." << std::endl
                  << "  Line: " << mutgos::text::to_string(
                     reader.get_current_line_index()) << std::endl
                  << "  Message: " << message << std::endl;
    }

    return 0;
}