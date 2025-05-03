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
    unsigned long long numNodes = 0;
    unsigned long long uCount = 0;
    unsigned long long iCount = 0;
    unsigned long long nodeCount = 1;
    unsigned long long numEdges = 0;

    // Read field.dat to get u and i nodes
    map<int, string> metisNodes;
    map<int, vector<int>> metisEdges;
    unordered_set<string> u_nodes, i_nodes;
    ifstream field_file("../../dataset/field.dat");
    string fline;

    while (getline(field_file, fline))
    {
        istringstream iss(fline);
        int id;
        string set_str;
        if (iss >> id >> set_str)
        {
            if (set_str == "u")
            {
                string nodeName = set_str + to_string(id);
                u_nodes.insert(nodeName);
                metisNodes[nodeCount++] = nodeName;
                uCount++;
                numNodes++;
            }
            else if (set_str == "i")
            {
                string nodeName = set_str + to_string(id);
                i_nodes.insert(nodeName);
                metisNodes[nodeCount++] = nodeName;
                iCount++;
                numNodes++;
            }
        }
    }
    field_file.close();

    // Read edges file
    unordered_map<string, vector<string>> edges;
    ifstream edges_file("../../dataset/edges.dat");
    string eline;

    while (getline(edges_file, eline))
    {
        istringstream iss(eline);
        string node1, node2, weight;

        if (iss >> node1 >> node2 >> weight)
            numEdges++;

        edges[node1].push_back(node2);
        edges[node2].push_back(node1);
    }
    edges_file.close();

    /*
    // Example output to verify the adjacency list
    for (const auto& entry : edges) {
        cout << entry.first << " -> ";
        for (const string& neighbor : entry.second) {
            cout << neighbor << " ";
        }
        cout << endl;
    }
    */

    // Reverse map of metis nodes for easy lookup
    map<string, int> reverseMap;
    for (const auto &pair : metisNodes)
    {
        reverseMap[pair.second] = pair.first;
    }

    /*
    // METIS FORMAT OUTPUT: NODES ONLY
    cout << "METIS node set size: " << metisNodes.size() << endl << endl;
    for (const auto& entry : metisNodes) {
        cout << entry.first << ": " << entry.second << endl;
    }

    */

    // Writing the graph in METIS format
    for (const auto &entry : edges)
    {
        string node = entry.first;
        int n1 = reverseMap[node];
        for (const string &neighbor : entry.second)
        {
            int n2 = reverseMap[neighbor];
            metisEdges[n1].push_back(n2);
            metisEdges[n2].push_back(n1);
        }
    }

    // verify edges in METIS format
    for (const auto &entry : metisEdges)
    {
        cout << entry.first << ": ";
        for (const int &neighbor : entry.second)
        {
            cout << neighbor << " ";
        }
        cout << endl;
    }

    // Save only the values (neighbors) from metisEdges to a file
    ofstream outfile("graph.metis");
    for (const auto &entry : metisEdges)
    {
        for (const int &neighbor : entry.second)
        {
            outfile << neighbor << " ";
        }
        outfile << endl;
    }
    outfile.close();

    cout << "U NODES: " << uCount << endl;
    cout << "I NODES: " << iCount << endl;
    cout << "TOTAL NODES: " << numNodes << endl;
    cout << "TOTAL EDGES: " << numEdges << endl;

    return 0;
}