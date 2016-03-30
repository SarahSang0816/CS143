#include "Bruinbase.h"
#include "SqlEngine.h"
#include "BTreeNode.h"
#include "BTreeIndex.h"
#include <cstdio>
#include <iostream>
using namespace std;
int main(int argc, const char * argv[])
{
    SqlEngine::run(stdin);
    return 0;
}
