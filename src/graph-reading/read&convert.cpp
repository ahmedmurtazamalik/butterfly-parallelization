#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <string>
#include <map>

using namespace std;

int main()
{
    unsigned long long uCount = 0;
    unsigned long long iCount = 0;
    unsigned long long numNodes = 0;
    unsigned long long numEdges = 0;

    // Read field.dat to get u and i nodes
    map<int, string> metisNodes;
    map<int, vector<int>> metisEdges;
    unordered_set<string> u_nodes, i_nodes;
    ifstream field_file("../../dataset/field.dat");
    string fline;
    int nodeIndex = 1;

    while (getline(field_file, fline))
    {
        istringstream iss(fline);
        int id;
        string set_str;
        if (iss >> id >> set_str)
        {
            string nodeName = set_str + to_string(id);
            metisNodes[nodeIndex++] = nodeName;
            numNodes++;
            if (set_str == "u")
                uCount++;
            else if (set_str == "i")
                iCount++;
        }
    }
    field_file.close();

    // Reverse map for lookup
    unordered_map<string, int> reverseMap;
    for (const auto &p : metisNodes)
    {
        reverseMap[p.second] = p.first;
    }

    // Read edges.dat and build adjacency
    ifstream edges_file("../../dataset/edges.dat");
    string eline;
    while (getline(edges_file, eline))
    {
        istringstream iss(eline);
        string node1, node2, weight;
        if (iss >> node1 >> node2 >> weight)
        {
            int u = reverseMap[node1];
            int v = reverseMap[node2];
            metisEdges[u].push_back(v);
            metisEdges[v].push_back(u);
            numEdges++;
        }
    }
    edges_file.close();

    long long unsigned realedgeCount = 0;
    // Remove duplicate neighbors and ensure sorted order
    for (auto &entry : metisEdges)
    {
        auto &nbrs = entry.second;
        sort(nbrs.begin(), nbrs.end());
        nbrs.erase(unique(nbrs.begin(), nbrs.end()), nbrs.end());
        realedgeCount += nbrs.size();
    }

    cout << numEdges << endl;
    cout << realedgeCount << endl;

    // Write graph.metis with exactly numNodes lines
    ofstream outfile("graph.metis");
    // METIS expects number of vertices and edges (undirected edge count)
    outfile << numNodes << " " << realedgeCount / 2 << "\n";

    // For each node index from 1..numNodes, write its adjacency list or blank line
    for (int idx = 1; idx <= (int)numNodes; ++idx)
    {
        auto it = metisEdges.find(idx);
        if (it != metisEdges.end())
        {
            for (int nbr : it->second)
            {
                outfile << nbr << " ";
            }
        }
        // even if no neighbors, this will output an empty line
        outfile << "\n";
    }
    outfile.close();

    // Summary
    cout << "TOTAL NODES: " << numNodes << endl;
    cout << "TOTAL EDGES: " << numEdges << " (undirected)" << endl;
    cout << "U NODES: " << uCount << endl;
    cout << "I NODES: " << iCount << endl;

    return 0;
}
