#include <iostream>
#include <vector>
#include <algorithm>

struct Person{
    int a;
    int b;
};

int main(){
    std::vector<int> temp = {1,2,3,4,5,6,7,8,9};
    temp.erase(std::remove_if(temp.begin(), temp.end(), [](auto a) -> bool {return a == 7;}), temp.end());
    for (auto i : temp){
        std::cout<<"wartosc wektora: "<<i<<std::endl;
    }
    std::cout<<temp.back()<<std::endl;

    Person p = {.a = 1, .b=3};
    Person p2 = {.a = 2, .b=4};
    std::vector<Person> person_vector[2];
    person_vector[0].push_back(p);
    person_vector[0].push_back(p2);
    int my_clock = 3;
    my_clock = std::max(my_clock, person_vector[0].back().a);
    std::cout<<my_clock<<std::endl;


    std::vector<int> fuck_me[2];
    for (int i : fuck_me[0]){
        std::cout<<"fuck me"<<std::endl;
    }
    std::cout<<fuck_me[0].size()<<std::endl;
}