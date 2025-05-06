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

using namespace std;

void readPaperGraph(const string& filename, map<int, string>& nodes, map<int, vector<int>>& edges, unordered_set<string>& u_nodes, unordered_set<string>& i_nodes) {
    ifstream file(filename);
    if (!file.is_open()) throw runtime_error("Could not open file");

    string line;

    // Skip header line
    getline(file, line);

    // Read partition sizes
    getline(file, line);
    int u_size = stoi(line);
    getline(file, line);
    int v_size = stoi(line);

    // Initialize adjacency lists for all nodes
    edges.clear();

    // Process U partition nodes
    for (int u_id = 0; u_id < u_size; u_id++) {
        getline(file, line);
        istringstream iss(line);
        int v_local;

        string u_name = "u" + to_string(u_id);
        nodes[u_id] = u_name;
        u_nodes.insert(u_name);

        // Read all V-local neighbors for this U node
        while (iss >> v_local) {
            int v_global = v_local + u_size;
            edges[u_id].push_back(v_global); // Add U → V edge
            edges[v_global].push_back(u_id); // Add V → U edge
        }
    }

    // Process V partition nodes (ensure existing edges are preserved)
    for (int v_id = 0; v_id < v_size; v_id++) {
        getline(file, line);
        istringstream iss(line);
        int u_local;
        int v_global = u_size + v_id;

        string v_name = "i" + to_string(v_id);
        nodes[v_global] = v_name;
        i_nodes.insert(v_name);

        // Read all U-local neighbors for this V node
        while (iss >> u_local) {
            // Only add if not already present (to avoid duplicates)
            if (find(edges[v_global].begin(), edges[v_global].end(), u_local) == edges[v_global].end()) {
                edges[v_global].push_back(u_local); // Add V → U edge
            }
            // Ensure U node also points back to V
            if (find(edges[u_local].begin(), edges[u_local].end(), v_global) == edges[u_local].end()) {
                edges[u_local].push_back(v_global); // Add U → V edge
            }
        }
    }

    // Write field.dat
    ofstream fieldOut("paperField.dat");
    for (int u_id = 0; u_id < u_size; ++u_id) {
        fieldOut << u_id << " u\n";
    }
    for (int v_id = u_size; v_id < u_size+v_size; ++v_id) {
        fieldOut << v_id << " i\n";
    }
    fieldOut.close();

    // Write edges.dat
    ofstream edgesOut("paperEdges.dat");
    for (int u_id = 0; u_id < u_size; ++u_id) {
        for (int v_global : edges[u_id]) {
            // Use the global ID of the I node (already starts from u_size)
            edgesOut << "u" << u_id << " i" << v_global << " 1\n";
        }
    }
    edgesOut.close();
}

int main453() {
    string paperFile = "paperGraph.txt";

    map<int, string> nodes;
    map<int, vector<int>> edges;
    unordered_set<string> u_nodes, i_nodes;
    unordered_map<string, int> reverseMap;

    // Read the paper's graph format
    readPaperGraph(paperFile, nodes, edges, u_nodes, i_nodes);

    return 0;
}