#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <omp.h>

using namespace std;
using Bitset = vector<uint64_t>;
using Key = uint64_t;

// === Global data structures ===
static vector<int> uIds;
static vector<int> iIds;
static unordered_map<int, int> uIndex;
static unordered_map<int, int> iIndex;
static vector<Bitset> adjBitsets;
static vector<vector<int>> iAdjList;
static size_t numBlocks;

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

// === Function declarations ===
bool isPaperFormat(const string& edge_file);
void loadPaperGraphData(const string& edge_file);
void loadStandardGraphData(const string& field_file, const string& edge_file);
unsigned long long countButterflies();
int mainOMP();


int mainOMP() {
    const string field_file = "paperGraph.txt";
    const string edge_file = "edges.dat";

    double t_start = omp_get_wtime();

    if (isPaperFormat(field_file)) {
        loadPaperGraphData(field_file);
    }
    else {
        loadStandardGraphData(field_file, edge_file);
    }

    unsigned long long total = countButterflies();
    double t_end = omp_get_wtime();

    cout << endl;
    cout << "OpenMP VERSION:\n";
    cout << "BUTTERFLIES COUNTED: " << total << "\n Elapsed time = " << (t_end - t_start)
        << " seconds\n\n";

    return 0;
}

// === Detect format by peeking first line ===
bool isPaperFormat(const string& edge_file) {
    ifstream fin(edge_file);
    if (!fin.is_open()) {
        cerr << "Error opening " << edge_file << "\n";
        exit(1);
    }
    string hdr;
    getline(fin, hdr);
    return (hdr == "AdjacencyGraph");
}

// === Paper-format loader ===
void loadPaperGraphData(const string& edge_file) {
    // Clear any existing data
    uIds.clear(); iIds.clear();
    uIndex.clear(); iIndex.clear();
    adjBitsets.clear(); iAdjList.clear();

    map<int, string> nodes;
    map<int, vector<int>> edges;

    ifstream file(edge_file);
    if (!file.is_open()) throw runtime_error("Could not open file");

    string line;
    getline(file, line); // skip "AdjacencyGraph"
    getline(file, line);
    int u_size = stoi(line);
    getline(file, line);
    int v_size = stoi(line);

    // Read U partition
    for (int u = 0; u < u_size; ++u) {
        getline(file, line);
        istringstream iss(line);
        nodes[u] = "u" + to_string(u);
        int v_local;
        while (iss >> v_local) {
            int vg = v_local + u_size;
            edges[u].push_back(vg);
            edges[vg].push_back(u);
        }
    }
    // Read I partition
    for (int v = 0; v < v_size; ++v) {
        int vg = u_size + v;
        getline(file, line);
        istringstream iss(line);
        nodes[vg] = "i" + to_string(v);
        int u_local;
        while (iss >> u_local) {
            auto& ev = edges[vg];
            if (find(ev.begin(), ev.end(), u_local) == ev.end()) ev.push_back(u_local);
            auto& eu = edges[u_local];
            if (find(eu.begin(), eu.end(), vg) == eu.end()) eu.push_back(vg);
        }
    }
    file.close();

    // Build IDs and indices
    for (auto& kv : nodes) {
        if (kv.second[0] == 'u') uIds.push_back(kv.first);
        else if (kv.second[0] == 'i') iIds.push_back(kv.first);
    }
    for (size_t i = 0; i < uIds.size(); ++i) uIndex[uIds[i]] = (int)i;
    for (size_t i = 0; i < iIds.size(); ++i) iIndex[iIds[i]] = (int)i;

    // Allocate adjacency
    numBlocks = (iIds.size() + 63) / 64;
    adjBitsets.assign(uIds.size(), Bitset(numBlocks, 0ULL));
    iAdjList.assign(iIds.size(), vector<int>());

    // Populate bitsets and I-side lists
    for (auto& kv : edges) {
        int global = kv.first;
        auto uit = uIndex.find(global);
        if (uit == uIndex.end()) continue;
        int uidx = uit->second;
        for (int dst : kv.second) {
            auto iit = iIndex.find(dst);
            if (iit == iIndex.end()) continue;
            int iidx = iit->second;
            size_t blk = iidx >> 6;
            size_t ofs = iidx & 63;
            adjBitsets[uidx][blk] |= (1ULL << ofs);
            iAdjList[iidx].push_back(uidx);
        }
    }
}

// === Standard-format loader ===
void loadStandardGraphData(const string& field_file, const string& edge_file) {
    // Clear any existing data
    uIds.clear(); iIds.clear();
    uIndex.clear(); iIndex.clear();
    adjBitsets.clear(); iAdjList.clear();

    ifstream fin(field_file);
    if (!fin) { cerr << "Error opening " << field_file << "\n"; exit(1); }
    int id; char type;
    while (fin >> id >> type) {
        if (type == 'u' || type == 'U') uIds.push_back(id);
        else if (type == 'i' || type == 'I') iIds.push_back(id);
    }
    fin.close();
    for (size_t i = 0; i < uIds.size(); ++i) uIndex[uIds[i]] = (int)i;
    for (size_t i = 0; i < iIds.size(); ++i) iIndex[iIds[i]] = (int)i;

    // Rank U by degree
    vector<int> deg(uIds.size(), 0);
    ifstream fe(edge_file);
    string a, b; double w;
    while (fe >> a >> b >> w) {
        int uid = -1;
        if (a[0] == 'u' || a[0] == 'U') uid = stoi(a.substr(1));
        if (b[0] == 'u' || b[0] == 'U') uid = stoi(b.substr(1));
        auto it = uIndex.find(uid);
        if (it != uIndex.end()) deg[it->second]++;
    }
    fe.close();
    sort(uIds.begin(), uIds.end(), [&](int x, int y) {
        return deg[uIndex[x]] > deg[uIndex[y]];
        });
    for (size_t i = 0; i < uIds.size(); ++i) uIndex[uIds[i]] = (int)i;

    // Allocate
    numBlocks = (iIds.size() + 63) / 64;
    adjBitsets.assign(uIds.size(), Bitset(numBlocks, 0ULL));
    iAdjList.assign(iIds.size(), vector<int>());

    // Build adjacencies
    unordered_set<Key> seen;
    ifstream ff(edge_file);
    while (ff >> a >> b >> w) {
        int uid = -1, iid = -1;
        if (a[0] == 'u') uid = stoi(a.substr(1)); else if (a[0] == 'i') iid = stoi(a.substr(1));
        if (b[0] == 'u') uid = stoi(b.substr(1)); else if (b[0] == 'i') iid = stoi(b.substr(1));
        auto uit = uIndex.find(uid);
        auto iit = iIndex.find(iid);
        if (uit == uIndex.end() || iit == iIndex.end()) continue;
        Key key = ((Key)uit->second << 32) | (uint32_t)iit->second;
        if (seen.insert(key).second) {
            size_t blk = iit->second >> 6;
            size_t ofs = iit->second & 63;
            adjBitsets[uit->second][blk] |= (1ULL << ofs);
            iAdjList[iit->second].push_back(uit->second);
        }
    }
    ff.close();
}

// === Butterfly counting ===
unsigned long long countButterflies() {
    size_t I = iAdjList.size();
    unordered_map<Key, uint32_t> wedgeCount;
    wedgeCount.reserve(I * 2);

#pragma omp parallel
    {
        unordered_map<Key, uint32_t> localMap;
        localMap.reserve(512);
#pragma omp for schedule(dynamic)
        for (long int i = 0; i < (long int)I; ++i) {
            const auto& nbrs = iAdjList[i];
            for (size_t p = 0; p < nbrs.size(); ++p) {
                for (size_t q = p + 1; q < nbrs.size(); ++q) {
                    int u = nbrs[p], v = nbrs[q];
                    if (u > v) swap(u, v);
                    Key k = ((Key)u << 32) | (uint32_t)v;
                    localMap[k]++;
                }
            }
        }
#pragma omp critical
        for (auto& kv : localMap) wedgeCount[kv.first] += kv.second;
    }

    unsigned long long butterflies = 0ULL;
    for (auto& kv : wedgeCount) {
        uint64_t c = kv.second;
        if (c > 1) butterflies += c * (c - 1) / 2;
    }
    return butterflies;
}
