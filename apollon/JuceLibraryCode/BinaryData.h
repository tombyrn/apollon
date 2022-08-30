/* =========================================================================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#pragma once

namespace BinaryData
{
    extern const char*   load_black_png;
    const int            load_black_pngSize = 11340;

    extern const char*   load_white_png;
    const int            load_white_pngSize = 11381;

    extern const char*   pause_black_png;
    const int            pause_black_pngSize = 9535;

    extern const char*   pause_white_png;
    const int            pause_white_pngSize = 9520;

    extern const char*   play_black_png;
    const int            play_black_pngSize = 12466;

    extern const char*   play_white_png;
    const int            play_white_pngSize = 12360;

    // Number of elements in the namedResourceList and originalFileNames arrays.
    const int namedResourceListSize = 6;

    // Points to the start of a list of resource names.
    extern const char* namedResourceList[];

    // Points to the start of a list of resource filenames.
    extern const char* originalFilenames[];

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding data and its size (or a null pointer if the name isn't found).
    const char* getNamedResource (const char* resourceNameUTF8, int& dataSizeInBytes);

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding original, non-mangled filename (or a null pointer if the name isn't found).
    const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8);
}
