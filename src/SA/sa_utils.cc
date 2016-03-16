#include <iostream>
#include <cstring>
#include "llvm/IR/BasicBlock.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"

#include "sa_utils.tpp"

using namespace std;
using namespace llvm;

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

/* Prints the contents of a block
 * (used fir debugging purpose)
 */

void dumpBlock( llvm::BasicBlock* bb ) {
    for( auto ins = bb->begin() ; ins != bb->end() ; ins++ ) {
        ins->print( errs() );
        std::cerr << "\n";
    }
}

/* Returns the list of names that are not functions of the second list
 */

std::vector<llvm::Function*> notInList( std::vector<llvm::Function*> funclist, std::vector<std::string> nameslist ) {
    std::vector<llvm::Function*> notInList;
    for( auto fun = funclist.begin() ; fun != funclist.end() ; fun++ ) {
        std::string n = (*fun)->getName().str();
        if( ! itemexists( nameslist, n ) ) {
            notInList.push_back( *fun );
        }
    }
    return notInList;
}

/* Get an (unsorted) list of blocks used by a function
 */

std::vector<llvm::BasicBlock*> blocklist( llvm::Function* fun ) {
    llvm::BasicBlock* current;
    llvm::TerminatorInst *term;
    int numsucc;
    llvm::BasicBlock* next;
    
    llvm::BasicBlock* entryBlock = fun->begin();
    std::vector<llvm::BasicBlock*> visitedblocks;
    std::vector<llvm::BasicBlock*> nextblocks;
        
    /* Get the blocks in a list. The order does not matter here */
    
    visitedblocks.push_back( entryBlock );
    nextblocks.push_back( entryBlock );

    do{
        current = nextblocks.back();
        nextblocks.pop_back();
        term = current->getTerminator();
        numsucc = term->getNumSuccessors();

        for( int i = 0 ; i < numsucc ; i++ ) {
            next = term->getSuccessor( i );
            if( ! itemexists( visitedblocks, next ) ) {
                nextblocks.push_back( next );
            } 
            visitedblocks.push_back( next );
        }
    } while( !nextblocks.empty() );
    
    return visitedblocks;
}

