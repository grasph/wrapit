#ifndef GRAPH_H
#define GRAPH_H

#include <list>
#include <vector>

class Graph {
public:
  Graph(): cyclic_(false), sorted_(false){}

  /** Detects if the index ordering graph contains a cycle.
   * Returns true if a cycle was found, false otherwise.
   */
  bool isCyclic();

  /** Specify ordering of two indices i and j, more precisely that i preceeds j,
   * i ≺ j.
   */
  void preceeds(unsigned i, unsigned j);

  /**
   * Extend index list to include indices up to maxIndex (excluded). By default,
   * the list goes up to the maximum index passed to the preceeds method (included).
   */
  void extend(unsigned maxIndex);
  
  /** Returns the index in an order respecting the contraints
   * added with calls to the preceeds() function:
   *
   *  x_0 ≺ x_1 ≺ x_2 ≺ …
   *
   * cycleDetected is set to true if the dependency graph
   * contains a cycle e.g. 1 ≻ 2 ≻ 3 ≻ 5 …, 3 ≻ 1.
   */
  std::vector<unsigned> sortedIndices();

private:
  void initScan();

  void localSort(unsigned vertex);

  void sort();

  /* Edges of the graph: vector element at index i contains
   * the list of nodes that preceeds i
   */
  std::vector<std::list<unsigned>> edges_;

  std::vector<bool> visited_;
  std::vector<bool> cyclicmarks_;
  std::vector<unsigned> sortednodes_;
  bool cyclic_;
  bool sorted_;
};

#endif //GRAPH_H not defined
