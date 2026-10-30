#include <iostream>
#include <vector>
#include <string>
using namespace std;

int twice(int x) { return x * 2; }

int main() {
    std::vector<int> v = {1, 2, 3};
    v.push_back(10);
    std::string word = "hi";
    int sum = 0;
    for (auto x : v) {
        sum += x;
    }
    cout << word << " " << sum << " " << v.size() << endl;
    std::cout << "twice: " << twice(21) << std::endl;
    return 0;
}
