/*
 * osinterface_types.h
 */

#ifndef MUTGOS_OSINTERFACE_TYPES_H_
#define MUTGOS_OSINTERFACE_TYPES_H_

#include <cstdint>
#include <ctime>

#define MG_UnsignedInt mutgos::osinterface::OsTypes::UnsignedInt
#define MG_SignedInt mutgos::osinterface::OsTypes::SignedInt
#define MG_VeryLongUnsignedInt mutgos::osinterface::OsTypes::VeryLongUnsignedInt
#define MG_LongUnsignedInt mutgos::osinterface::OsTypes::LongUnsignedInt
#define MG_Float mutgos::osinterface::OsTypes::Float

#define MG_NewLine "\n"

namespace mutgos
{
namespace osinterface
{
    class OsTypes
    {
    public:
        /** Represents time in seconds since epoch.  This may need
            changing if not compiling 64 bit! */
        typedef time_t TimeEpochType;

        /** 4 byte unsigned integer */
        typedef uint32_t UnsignedInt;

        /** 4 byte signed integer */
        typedef int32_t SignedInt;

        // TODO Merge this into LongUnsignedInt and rename?
        /** 8 byte unsigned integer */
        typedef uint64_t VeryLongUnsignedInt;

        /** 8 byte unsigned integer */
        typedef uint64_t LongUnsignedInt;

        /** Standard 8 byte float */
        typedef float Float;

        /** Standard floating point */
        typedef long double Double;

        /** 1 byte unsigned integer */
        typedef unsigned char UnsignedInt8;
    };
}
} /* namespace mutgos */
#endif /* OSINTERFACE_TYPES_H_ */
