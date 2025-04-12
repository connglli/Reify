#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <set>
#include <queue>
class Graph {
public:
    int nodes;
    std::vector<std::set<int>> adj; // Use set to avoid duplicate edges

    Graph(int n) : nodes(n), adj(n) {}

    void add_edge(int u, int v) {
        if (u != v && adj[u].size() < 2) { // Ensure outdegree <= 2
            adj[u].insert(v);
        }
    }

    // Function to check if a node has a path to a given target
    bool has_path(int start, int target) {
        std::vector<bool> visited(nodes, false);
        std::queue<int> q;
        q.push(start);
        visited[start] = true;

        while (!q.empty()) {
            int curr = q.front();
            q.pop();
            if (curr == target) return true;
            for (int neighbor : adj[curr]) {
                if (!visited[neighbor]) {
                    visited[neighbor] = true;
                    q.push(neighbor);
                }
            }
        }
        return false;
    }

    // Ensure every node has a path to the last node while keeping outdegree <= 2
    void enforce_reachability_to_last() {
        for (int i = nodes - 2; i >= 0; i--) {
            if (!has_path(i, nodes - 1)) {
                std::vector<int> candidates;
                for (int j = i + 1; j < nodes; j++) {
                    if (has_path(j, nodes - 1)) {
                        candidates.push_back(j);
                    }
                }
                bool found = false;
                for(auto candidate: candidates)
                {
                    if(candidate == nodes - 1)
                    {
                        continue;
                    }
                    if(adj[candidate].size() < 2)
                    {
                        add_edge(i, candidate);
                        found = true;
                    }
                }
                if(!found)
                {
                    add_edge(i, nodes - 1);
                }

            }
        }
    }

    // Ensure every node is reachable from 0 while keeping outdegree <= 2
    void enforce_reachability_from_start() {
        for (int i = 1; i < nodes; i++) {
            if (!has_path(0, i)) {
                std::vector<int> candidates;
                for (int j = 0; j < i; j++) {
                    if (has_path(0, j)) {
                        candidates.push_back(j);
                    }
                }
                for(auto candidate: candidates)
                {
                    if(candidate == 0)
                    {
                        continue;
                    }
                    if(adj[candidate].size() < 2)
                    {
                        add_edge(candidate, i);
                    }
                }
                // if (!candidates.empty() && adj[candidates[0]].size() < 2) {
                //     int target = candidates[rand() % candidates.size()];
                //     add_edge(target, i);
                // }
            }
        }
    }

    void generate_graph() {
        srand(time(0));

        for (int i = 0; i < nodes - 1; i++) {
            if (rand() % 2 == 0 || i == nodes - 2) {
                int target = rand() % nodes;
                while (target == 0 || target == i) target = rand() % nodes;
                add_edge(i, target);
            } else {
                int target1 = rand() % nodes;
                int target2 = rand() % nodes;

                while (target1 == 0 || target1 == i) target1 = rand() % nodes;
                while (target2 == 0 || target2 == i || target2 == target1) target2 = rand() % nodes;

                add_edge(i, target1);
                add_edge(i, target2);
            }
        }

        // Ensure all nodes have a path to the last node while obeying outdegree <= 2
        

        // Ensure all nodes are reachable from 0 while obeying outdegree <= 2
        enforce_reachability_from_start();
        enforce_reachability_to_last();
    }
    std::vector<int> sample_walk(int start, int end, int max_steps = 100) {
        std::vector<int> walk;
        max_steps = std::max(max_steps, 50*nodes); // Ensure max_steps is at least 10 times the number of nodes
        int current_node = start;
        walk.push_back(current_node);

        int steps = 0;
        while (current_node != end && steps < max_steps) {
            const std::set<int>& neighbors = adj[current_node];
            if (neighbors.empty()) {
                break; // No more neighbors to move to
            }

            // Randomly choose a neighbor to walk to
            auto it = neighbors.begin();
            std::advance(it, rand() % neighbors.size()); // Random step
            current_node = *it;

            walk.push_back(current_node);
            steps++;
        }

        // If we did not reach the end, the walk is incomplete
        return walk;
    }
    std::vector<int> sample_consistent_walk(int start, int end, int max_steps = 100) {
        std::vector<int> walk;
        max_steps = std::max(max_steps, 50 * nodes); // Ensure enough steps
        int current_node = start;
        walk.push_back(current_node);
    
        std::map<int, std::set<int>> visited_neighbors; // Tracks visited neighbors for each node
        std::map<int, int> locked_neighbor; // Locks a node to one of its neighbors once both are visited
    
        int steps = 0;
        while (current_node != end && steps < max_steps) {
            const std::set<int>& neighbors = adj[current_node];
            if (neighbors.empty()) {
                break; // No more neighbors to move to
            }
    
            // Check if this node already has a locked neighbor
            if (locked_neighbor.find(current_node) != locked_neighbor.end()) {
                // If locked, always go to the locked neighbor
                current_node = locked_neighbor[current_node];
            } else {
                // Otherwise, randomly choose a neighbor
                auto it = neighbors.begin();
                std::advance(it, rand() % neighbors.size()); // Random step
                int next_node = *it;
    
                // Record the visit
                visited_neighbors[current_node].insert(next_node);
    
                // If the node had outdegree 2 and now has visited both, lock to the last one
                if (neighbors.size() == 2 && visited_neighbors[current_node].size() == 2) {
                    locked_neighbor[current_node] = next_node; // Lock to the first visited neighbor
                }
    
                current_node = next_node;
            }
    
            walk.push_back(current_node);
            steps++;
        }
    
        return walk;
    }
    void print_graph() {
        for (int i = 0; i < nodes; i++) {
            std::cout << "Node " << i << " -> ";
            for (int j : adj[i]) {
                std::cout << j << " ";
            }
            std::cout << std::endl;
        }
    }
};

