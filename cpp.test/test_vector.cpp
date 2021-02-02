#include <vector>
#include <iostream>

using namespace std;

int main(int argc, char* argv[])
{
    vector<vector<int> > myVec;
    cout << myVec.size() << endl;
    vector<int> rVec;
    cout << rVec.size() << endl;
    myVec.emplace_back(rVec);
    cout << myVec.size() << endl;

    return 0;
}
