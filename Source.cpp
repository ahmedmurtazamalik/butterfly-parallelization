#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <string>
#include <map>
#include <ctime>
#include "openMPCount.h"
#include "serialCount.h"

using namespace std;


int main() {
    mainSerial();
    mainOMP();
    return 0;
}
