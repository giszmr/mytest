#include <cstdio>
#include <cstdlib>

using namespace std;

typedef struct node
{
    char data;
    struct node *pLChild;
    struct node *pRChild;
}BiTree, PBiTree;

PBiTree CreateBiTree(PBiTree pNode)
{
    char c = 0;
    cin >> c;
    pNode = new BiTree;
    pCurNode->data = c;
    pCurNode->pLChild = NULL;
    pCurNode->pRChild = NULL;
    if (c == '#')
    {
        return
    }
    if(biTree)
}


int main(int argc, char*argv[])
{
    return 0;
}
