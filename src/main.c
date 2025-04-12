#include <iostream>
#include <boost/dynamic_bitset.hpp>

int main(){


    boost::dynamic_bitset<> B2(2);
    B2 = B2+1;
    std::cout << B2 << std::endl;
    //std::cout << (int)B2 << std::endl;

}