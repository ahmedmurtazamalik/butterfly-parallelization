#include <mpi.h>
#include <omp.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <cstdint>

using namespace std;
using Bitset = vector<uint64_t>;
using Key = uint64_t;

// Portable popcount as before
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

vector<int> uIds, iIds;
unordered_map<int, int> uIndex;
size_t numBlocks;

void read_field(const string& field_file) {
    ifstream fin(field_file);
    if (!fin) { cerr << "Error opening " << field_file << "\n"; exit(1); }
    int id;
    char type;
    while (fin >> id >> type) {
        if (type == 'u' || type == 'U') uIds.push_back(id);
        else if (type == 'i' || type == 'I') iIds.push_back(id);
    }
    fin.close();
    uIndex.clear();
    for (size_t idx = 0; idx < uIds.size(); ++idx)
        uIndex[uIds[idx]] = idx;
}

void rank_by_degree(const string& edge_file) {
    vector<int> degs(uIds.size(), 0);
    ifstream fin(edge_file);
    if (!fin) { cerr << "Error opening " << edge_file << "\n"; exit(1); }
    string a, b; double w;
    while (fin >> a >> b >> w) {
        int uid = -1;
        if (a[0] == 'u' || a[0] == 'U') uid = stoi(a.substr(1));
        if (b[0] == 'u' || b[0] == 'U') uid = stoi(b.substr(1));
        if (uIndex.count(uid)) degs[uIndex[uid]]++;
    }
    fin.close();
    vector<int> sortedU = uIds;
    sort(sortedU.begin(), sortedU.end(), [&](int x, int y) {
        return degs[uIndex[x]] > degs[uIndex[y]];
        });
    uIds.swap(sortedU);
    uIndex.clear();
    for (size_t i = 0; i < uIds.size(); ++i)
        uIndex[uIds[i]] = i;
}

void build_adjacencies(const string& edge_file, vector<Bitset>& adjBitsets) {
    ifstream fin(edge_file);
    if (!fin) { cerr << "Error opening " << edge_file << "\n"; exit(1); }
    unordered_set<Key> seen;
    string a, b; double w;
    size_t I = iIds.size();
    while (fin >> a >> b >> w) {
        int uid = -1, iid = -1;
        // Parse u and i ids
        if (a[0] == 'u' || a[0] == 'U') uid = stoi(a.substr(1));
        else if (a[0] == 'i' || a[0] == 'I') iid = stoi(a.substr(1));
        if (b[0] == 'u' || b[0] == 'U') uid = stoi(b.substr(1));
        else if (b[0] == 'i' || b[0] == 'I') iid = stoi(b.substr(1));
        if (!uIndex.count(uid) || iid == -1) continue;
        auto iit = find(iIds.begin(), iIds.end(), iid);
        if (iit == iIds.end()) continue;
        int iIdx = distance(iIds.begin(), iit);
        int uIdx = uIndex[uid];
        Key key = (static_cast<Key>(uIdx) << 32) | iIdx;
        if (seen.insert(key).second) {
            size_t block = iIdx >> 6;
            size_t offset = iIdx & 63;
            adjBitsets[uIdx][block] |= (1ULL << offset);
        }
    }
    fin.close();
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    double t_start = MPI_Wtime();
    vector<Bitset> adjBitsets;
    size_t U = 0, I = 0;

    if (rank == 0) {
        read_field("paperField.dat");
        rank_by_degree("paperEdges.dat");
        U = uIds.size();
        I = iIds.size();
        numBlocks = (I + 63) / 64;
        adjBitsets.assign(U, Bitset(numBlocks, 0));
        build_adjacencies("paperEdges.dat", adjBitsets);
    }

    MPI_Bcast(&U, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);
    MPI_Bcast(&I, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);
    MPI_Bcast(&numBlocks, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);

    vector<int> uIds_local(U), iIds_local(I);
    if (rank == 0) {
        copy(uIds.begin(), uIds.end(), uIds_local.begin());
        copy(iIds.begin(), iIds.end(), iIds_local.begin());
    }
    MPI_Bcast(uIds_local.data(), U, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(iIds_local.data(), I, MPI_INT, 0, MPI_COMM_WORLD);

    vector<Bitset> adjBitsets_local(U, Bitset(numBlocks));
    if (rank == 0) {
        for (size_t u = 0; u < U; ++u)
            copy(adjBitsets[u].begin(), adjBitsets[u].end(), adjBitsets_local[u].begin());
    }
    for (size_t u = 0; u < U; ++u)
        MPI_Bcast(adjBitsets_local[u].data(), numBlocks, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);

    size_t chunk = U / size;
    size_t remainder = U % size;
    size_t start = rank * chunk + (rank < remainder ? rank : remainder);
    size_t end = start + chunk + (rank < remainder ? 1 : 0);

    unsigned long long local_count = 0;

    #pragma omp parallel for schedule(dynamic) reduction(+:local_count)
    for (size_t u = start; u < end; ++u) {
        for (size_t v = u + 1; v < U; ++v) {
            uint64_t common = 0;
            #pragma omp simd reduction(+:common)
            for (size_t b = 0; b < numBlocks; ++b)
                common += POPCOUNT(adjBitsets_local[u][b] & adjBitsets_local[v][b]);
            local_count += common * (common - 1) / 2;
        }
    }

    unsigned long long total = 0;
    MPI_Reduce(&local_count, &total, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        double t_end = MPI_Wtime();
        cout << "MPI Version:\nButterflies counted: " << total
            << "\nElapsed time: " << t_end - t_start << "s\n";
    }

    MPI_Finalize();
    return 0;
}

