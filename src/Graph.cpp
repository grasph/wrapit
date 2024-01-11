#include <bits/stdc++.h>
using namespace std;
#include "Graph.h"
//class Graph {
//public:
//  Graph(): cyclic_(false), sorted_(false){}
//
//  /** Detects if the index ordering graph contains a cycle.
//   * Returns true if a cycle was found, false otherwise.
//   */
//  bool isCyclic();
//
//  /** Specify ordering of two indices i and j,
//   * more precisely that i preceeds j, i ≺ j.
//   */
//  void preceeds(unsigned i, unsigned j);
//
//  /** Returns the index in an order respecting the contraints
//   * added with calls to the preceeds() function:
//   *
//   *  x_0 ≺ x_1 ≺ x_2 ≺ …
//   *
//   * cycleDetected is set to true if the dependency graph
//   * contains a cycle e.g. 1 ≺ 2 ≺ 3 ≺ 5 …, 3 ≺ 1.
//   */
//  std::vector<unsigned> sortedIndices();
//
//private:
//  void initScan();
//
//  void localSort(unsigned vertex);
//
//  void sort();
//
//  /* Edges of the graph: vector element at index i contains
//   * the list of nodes that preceeds i
//   */
//  std::vector<list<unsigned>> edges_;
//
//  std::vector<bool> visited_;
//  std::vector<bool> cyclicmarks_;
//  std::vector<unsigned> sortednodes_;
//  bool cyclic_;
//  bool sorted_;
//};

void Graph::extend(unsigned n){
  if(n > edges_.size()){
    edges_.resize(n);
    sorted_ = false;
  }
}

bool Graph::isCyclic(){
  if(!sorted_) sort();
  return cyclic_;
}

void Graph::preceeds(unsigned v, unsigned w){
  //invalidate the sorting:
  sorted_ = false;

  unsigned min_nodes = std::max(v, w) + 1;
  if(min_nodes > edges_.size()){
    edges_.resize(min_nodes);
  }

  edges_[w].push_back(v);
}

std::vector<unsigned> Graph::sortedIndices(){
  if(!sorted_) sort();
  return sortednodes_;
}

void Graph::initScan(){
  sortednodes_.clear();
  sortednodes_.reserve(edges_.size());

  visited_.resize(edges_.size());
  fill(visited_.begin(), visited_.end(), false);

  cyclicmarks_.resize(edges_.size());
  fill(cyclicmarks_.begin(), cyclicmarks_.end(), false);

  cyclic_ = false;
}


/*function used to sort the indices*/
void Graph::localSort(unsigned vertex){

  visited_[vertex] = true;

  //mark that we are starting the following
  //recursive loops from this vertex:
  cyclicmarks_[vertex] = true;

  /*check for all indices preceeding vertex*/
  for (const auto& node: edges_[vertex]){
    //    std::cerr<< node << " (" << cyclicmarks_[node]
    //             << ", " << cyclicmarks_[node] << ") "
    //             << " ≺ "
    //             << vertex << " (" << visited_[vertex]
    //             << ", " << cyclicmarks_[vertex] << ") "
    //             << "\n";
    if (!visited_[node]){
      localSort(node);
    }
    cyclic_ |= cyclicmarks_[node];
    //    std::cerr << "cyclic_: " << cyclic_ << "\n";
  }

  //remove the cycling detection mark:
  cyclicmarks_[vertex] = false;

  //The node vertex has no not-yet-visited preceeding
  //node, add it to the sorted list:
  sortednodes_.push_back(vertex);
}

void Graph::sort(){
  initScan();

  for (unsigned i = 0; i < visited_.size(); i++){
    if (visited_[i] == false){ //<-- can be removed...
      localSort(i);
    }
  }
  sorted_ = true;
}

#if 0 //to test
int main(){
  // Create a graph given in the above diagram
  Graph g;
  g.preceeds(1, 2);
  g.preceeds(2, 3);

  g.preceeds(1, 4);
  g.preceeds(4, 5);

  g.preceeds(4, 6);
  g.preceeds(6, 7);
  g.preceeds(7, 8);
  g.preceeds(7,8);
  g.preceeds(8,9);
  //at this point the graph is completly oriented

  //extra contraints that do not
  //change the ordering:
  g.preceeds(2, 7);
  g.preceeds(4, 9);
  g.preceeds(5, 8);
  g.preceeds(5, 9);

  cout << "Ordered indices: ";
  auto sorted_nodes = g.sortedIndices();


  for(const auto& e: sorted_nodes){
    std::cout << e << " ";
  }
  cout << "\n";

  std::cout << "Is-cyclic flag: " << g.isCyclic() << "\n";

  std::cout << "\nAdding 3 ≺ 1\n";
  g.preceeds(3, 1);

  sorted_nodes = g.sortedIndices();
  std::cout << "New order: ";
  for(const auto& e: sorted_nodes){
    std::cout << e << " ";
  }
  std::cout << "\nNew is-cyclic flag: " << g.isCyclic() << "\n";

  return 0;
}
#endif
