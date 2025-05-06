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
#define NUM_THREADS 8
using namespace std;


int main() {
    mainSerial();
    omp_set_num_threads(NUM_THREADS); 
    mainOMP();
    return 0;
}
