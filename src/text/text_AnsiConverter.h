/*
 * text_AnsiConverter.h
 */

#ifndef MUTGOS_TEXT_ANSICONVERTER_H
#define MUTGOS_TEXT_ANSICONVERTER_H

#include "text_ExternalText.h"

namespace mutgos
{
namespace text
{
    /**
     * Converts the given line to ANSI color.
     * @param line[in] The line of external text to convert.
     * @return The line as an ANSI colorified string.
     */
    std::string to_ansi(const ExternalTextLine &line);
}
}

#endif //MUTGOS_TEXT_ANSICONVERTER_H
