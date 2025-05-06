#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <sstream>
#include <algorithm>
#include <chrono>

using namespace std;
using Bitset = vector<uint64_t>;

// Portable popcount
#if defined(__GNUC__)
#define POPCOUNT __builtin_popcountll
#elif __has_include(<bit>)
#include <bit>
#define POPCOUNT std::popcount
#else
inline int popcount_fallback(uint64_t x) {
    int c = 0;
    while (x) { x &= (x - 1); ++c; }
    return c;
}
#define POPCOUNT popcount_fallback
#endif

// Global data structures (Serial)
using KeySerial = uint64_t;
vector<int> uIdsSerial;
vector<int> iIdsSerial;
unordered_map<int, int> uIndexSerial;
unordered_map<int, int> iIndexSerial;
vector<Bitset> adjBitsetsSerial;      // U-side bitsets (Serial)
vector<vector<int>> iAdjListSerial;    // I-side adjacency lists for wedges (Serial)
size_t numBlocksSerial;

// Read paper-format adjacency graph (Serial)
void read_paper_graph_Serial(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) throw runtime_error("Could not open file");

    string line;
    // Header
    getline(file, line);
    // Partition sizes
    getline(file, line);
    int u_size = stoi(line);
    getline(file, line);
    int v_size = stoi(line);

    // Temporary edge storage
    map<int, vector<int>> tmpEdges;
    // U-nodes initialization
    uIdsSerial.clear(); iIdsSerial.clear();
    uIndexSerial.clear(); iIndexSerial.clear();
    for (int u = 0; u < u_size; ++u) {
        uIdsSerial.push_back(u);
        uIndexSerial[u] = u;
    }
    for (int v = 0; v < v_size; ++v) {
        int v_global = v + u_size;
        iIdsSerial.push_back(v_global);
        iIndexSerial[v_global] = v;
    }
    // Read U partition neighbors
    for (int u = 0; u < u_size; ++u) {
        getline(file, line);
        istringstream iss(line);
        int v_local;
        while (iss >> v_local) {
            int v_global = v_local + u_size;
            tmpEdges[u].push_back(v_global);
            tmpEdges[v_global].push_back(u);
        }
    }
    // Read V partition neighbors
    for (int v = 0; v < v_size; ++v) {
        getline(file, line);
        istringstream iss(line);
        int u_local;
        int v_global = v + u_size;
        while (iss >> u_local) {
            auto& ve = tmpEdges[v_global];
            if (find(ve.begin(), ve.end(), u_local) == ve.end()) ve.push_back(u_local);
            auto& ue = tmpEdges[u_local];
            if (find(ue.begin(), ue.end(), v_global) == ue.end()) ue.push_back(v_global);
        }
    }
    file.close();

    // Build bitsets and I-side lists
    size_t U = uIdsSerial.size();
    size_t I = iIdsSerial.size();
    numBlocksSerial = (I + 63) / 64;
    adjBitsetsSerial.assign(U, Bitset(numBlocksSerial, 0ULL));
    iAdjListSerial.assign(I, vector<int>());
    for (int u_global : uIdsSerial) {
        int u_idx = uIndexSerial[u_global];
        for (int nb : tmpEdges[u_global]) {
            if (nb >= u_size) {
                int i_idx = iIndexSerial[nb];
                adjBitsetsSerial[u_idx][i_idx >> 6] |= (1ULL << (i_idx & 63));
                iAdjListSerial[i_idx].push_back(u_idx);
            }
        }
    }
}

// Read nodes from field.dat (Serial)
void read_field2Serial(const string& field_file) {
    ifstream fin(field_file);
    if (!fin) { cerr << "Error opening " << field_file << "\n"; exit(1); }
    int id; char type;
    uIdsSerial.clear(); iIdsSerial.clear();
    uIndexSerial.clear(); iIndexSerial.clear();
    while (fin >> id >> type) {
        if (type == 'u' || type == 'U') uIdsSerial.push_back(id);
        else if (type == 'i' || type == 'I') iIdsSerial.push_back(id);
    }
    fin.close();
    for (size_t idx = 0; idx < uIdsSerial.size(); ++idx)
        uIndexSerial[uIdsSerial[idx]] = static_cast<int>(idx);
    for (size_t idx = 0; idx < iIdsSerial.size(); ++idx)
        iIndexSerial[iIdsSerial[idx]] = static_cast<int>(idx);
}

// First pass: compute U-side degrees and reorder by descending degree (Serial)
void rank_by_degreeSerial(const string& edge_file) {
    vector<int> degs(uIdsSerial.size(), 0);
    ifstream fin(edge_file);
    if (!fin) { cerr << "Error opening " << edge_file << " for degree computation\n"; exit(1); }
    string a, b; double w;
    while (fin >> a >> b >> w) {
        int uid = -1;
        if (a[0] == 'u' || a[0] == 'U') uid = stoi(a.substr(1));
        if (b[0] == 'u' || b[0] == 'U') uid = stoi(b.substr(1));
        auto it = uIndexSerial.find(uid);
        if (it != uIndexSerial.end()) degs[it->second]++;
    }
    fin.close();
    vector<int> sortedUSerial = uIdsSerial;
    sort(sortedUSerial.begin(), sortedUSerial.end(), [&](int x, int y) {
        return degs[uIndexSerial[x]] > degs[uIndexSerial[y]];
        });
    uIdsSerial.swap(sortedUSerial);
    for (size_t i = 0; i < uIdsSerial.size(); ++i)
        uIndexSerial[uIdsSerial[i]] = static_cast<int>(i);
}

// Build both bitset adjacency and I-side lists (Serial)
void build_adjacenciesSerial(const string& edge_file) {
    ifstream fin(edge_file);
    if (!fin) { cerr << "Error opening " << edge_file << " for adjacency build\n"; exit(1); }
    unordered_set<KeySerial> seen;
    string a, b; double w;
    while (fin >> a >> b >> w) {
        int uid = -1, iid = -1;
        if (a[0] == 'u' || a[0] == 'U') uid = stoi(a.substr(1));
        else if (a[0] == 'i' || a[0] == 'I') iid = stoi(a.substr(1));
        if (b[0] == 'u' || b[0] == 'U') uid = stoi(b.substr(1));
        else if (b[0] == 'i' || b[0] == 'I') iid = stoi(b.substr(1));
        auto uit = uIndexSerial.find(uid);
        auto iit = iIndexSerial.find(iid);
        if (uit == uIndexSerial.end() || iit == iIndexSerial.end()) continue;
        int uIdx = uit->second, iIdx = iit->second;
        KeySerial key = (static_cast<KeySerial>(uIdx) << 32) | static_cast<uint32_t>(iIdx);
        if (seen.insert(key).second) {
            adjBitsetsSerial[uIdx][iIdx >> 6] |= (1ULL << (iIdx & 63));
            iAdjListSerial[iIdx].push_back(uIdx);
        }
    }
    fin.close();
}

// Count butterflies via wedge grouping (Serial)
unsigned long long count_butterflies3Serial() {
    unordered_map<KeySerial, uint32_t> wedgeCountSerial;
    wedgeCountSerial.reserve(iAdjListSerial.size() * 4);
    for (size_t i = 0; i < iAdjListSerial.size(); ++i) {
        const auto& nbrs = iAdjListSerial[i];
        for (size_t p = 0, d = nbrs.size(); p < d; ++p) {
            for (size_t q = p + 1; q < d; ++q) {
                int u = nbrs[p], v = nbrs[q];
                if (u > v) swap(u, v);
                KeySerial k = (static_cast<KeySerial>(u) << 32) | static_cast<uint32_t>(v);
                wedgeCountSerial[k]++;
            }
        }
    }
    unsigned long long butterfliesSerial = 0ULL;
    for (const auto& kv : wedgeCountSerial) {
        uint64_t c = kv.second;
        if (c > 1) butterfliesSerial += c * (c - 1) / 2;
    }
    return butterfliesSerial;
}

int mainSerial() {
    auto t_start = chrono::high_resolution_clock::now();

    char choose = 0;
    while (choose != '1' && choose != '2')
    {
        cout << "Enter 1 to use paper graph dataset, 2 to use generated dataset" << endl;
        cin >> choose;
    }

    string field_file = "field.dat";
    string edge_file = "edges.dat";
    if (choose == '1')
    {
        field_file = "paperGraph.txt";
    }

    // Detect format by inspecting first line of field_file
    ifstream test(field_file);
    string header;
    getline(test, header);
    test.close();

    if (header == "AdjacencyGraph") {
        read_paper_graph_Serial(field_file);
    }
    else {
        read_field2Serial(field_file);
        rank_by_degreeSerial(edge_file);
        size_t U = uIdsSerial.size();
        size_t I = iIdsSerial.size();
        numBlocksSerial = (I + 63) / 64;
        adjBitsetsSerial.assign(U, Bitset(numBlocksSerial, 0ULL));
        iAdjListSerial.assign(I, vector<int>());
        build_adjacenciesSerial(edge_file);
    }

    unsigned long long totalSerial = count_butterflies3Serial();
    auto t_end = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = t_end - t_start;

    cout << "\nSERIAL VERSION:\n";
    cout << "BUTTERFLIES COUNTED: " << totalSerial << "\n";
    cout << "Elapsed time = " << elapsed.count() << " seconds\n";
    return 0;
}
