#include <iostream>
#include <unordered_map>
#include <boost/dynamic_bitset.hpp>
using namespace std;

class test {
public:
    boost::dynamic_bitset<> PHR;

    // Constructor to initialize PHR with size 5
    test() : PHR(5) {}
};

struct history {
    test T;
};

int main() {
    history temp;
    std::unordered_map<int, history> pred;
    pred.emplace(100, temp);

    // Modify temp's PHR
    temp.T.PHR.set(3);

    // Access the stored history
    auto& hist = pred.at(100); // notice the & to get reference

    cout << temp.T.PHR << endl;   // prints temp's PHR
    cout << hist.T.PHR << endl;   // prints stored pred[100]'s PHR

    return 0;
}
