#include <iostream>
#include "PrintUsage.h"

using namespace std;

void PrintUsage(){
    cout << "saberdaq-specific options:\n";
    cout << "    --comment\t\tquoted string to go together with the hdf5 output file\n";
    cout << "    -e, --event [N]\tnumber of events to record\n";
    cout << "    --prefix [foo]\tprefix to be added to output as foo_yyyymmdd_hhmmss.hdf5\n";
    cout << "    --output [foo.hdf5]\tspecify the output filename directly (no date information appended)\n";
    cout << "    /graphics/[c,C,b,B]\tUI command to change the board/channel displayed, lower case to increment\n";
}
