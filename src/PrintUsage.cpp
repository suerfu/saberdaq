#include <iostream>
#include "PrintUsage.h"

using namespace std;

void PrintUsage(){
    cout << "saberdaq-specific options:\n";
    cout << "\t--comment\tquoted string to go together with the hdf5 output file\n";
    cout << "\t-e, --event [N]\tnumber of events to record\n";
    cout << "\t--prefix [foo]\tprefix to be added to output as foo_yyyymmdd_hhmmss.hdf5\n";
    cout << "\t--output [foo.hdf5]\tspecify the output filename directly (no date information appended)\n";
    cout << "\t/graphics/[c,C,b,B]\tUI command to change the board/channel displayed, lower case to increment\n";
}
