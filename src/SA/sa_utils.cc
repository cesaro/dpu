#include <iostream>
#include <cstring>

using namespace std;

/* Compares two function names:
 * s1 is a hard-defined name
 * s2 is a name obtained from the IR
 * Returns 0 if the names are the same, something else otherwise
 */

int safeCompareFname( const char* s1, std::string s2 ){
    int l1, l2;
    
    l1 = std::strlen( s1 );
    l2 = s2.size();

    return (l1==l2)?(std::strcmp( s1, s2.c_str() )):-1;
}
