This is a real world performance test program for three different tree data structures.

The tree types are
* Left-child right-sibling binary tree
 * (https://en.wikipedia.org/wiki/Left_child_right_sibling)
* Child array tree
 * naive but intuitive
* Flat array tree
 * new

The test executes following operations on all of the trees.
* add node
* traverse from root to a random leaf
* find maximum depth of the tree
* read every node of the tree in depth search order
* move subtree to a different parent node
* delete subtree

Additionally following operations are executed only on the pointer based trees. Flat array is excluded these are just simple look-up operations on it.
* count nodes
* travel to the tree's Nth node in depth search order

The test are run using multiple different tree sizes. The sizes span from 10 nodes to 25,000 nodes. Additionally the test are repeated multiple times to mitigate random variance between executions.

The CPU cache is flushed before every measurement to ensure a clean execution state. The flushing is achieved by moving around several megabytes worth of heap memory via the CPU's memory pipeline. The majority of the programs execution time is infact spent doing this.


# Data structures

## Left-child right-sibling binary tree
This tree unlike its name suggests isn't really a binary tree as its nodes can have an unlimited number of child nodes. It is a minimal data structure whose node consists of one child and one sibling pointer.

```
struct Node
{
    Node* child;
    Node* sibling;
    Value value;
};
struct Tree
{
    Node* root;
};
```

While its nodes are small it suffers from linked-list like cache performance. This can to some extent be alleviated by allocating the nodes from a pool as that way they are be more likely in the same cache line as related nodes.

In this tree nodes have no knowledge of their parent nodes. Finding a parent of a node requires recursing through the whole tree. This greatly affects the trees node deletion performance in particular as the node pointing to the one being deleted must be searched and its pointers be manipulated.

Additions are fast in this tree. That is as long as the future parent of the node is already fetched.

More info here: https://en.wikipedia.org/wiki/Left-node_right-sibling_binary_tree

## Child array tree
Child array tree is a somewhat naïve but still viable data structure - it's the first thing that usually comes to mind when thinking of trees with multiple child nodes. Every node consists of a parent pointer and an array of child pointers. Additionally the node can also have left and right sibling pointers which I have decided to include as several implementations I happened to come across had them.

```
struct Node
{
    Node* parent;
    Array<Node*> children;
    Node* leftSibling;
    Node* rightSibling;
    Value value;
};
struct Tree
{
    Node* root;
};
```

Child array trees child nodes are, as the name suggests, in an array which enables random order accessing. Though, this is somewhat undermined by the fact that the nodes still have to be referenced by pointer. 

While compared to left-child right-sibling binary tree, child array tree is much less like linked-list, its cache performance can still be greatly improved by pooling the nodes.



## Flat array tree
Flat array tree is (as far as I know) a new data structure to contain tree structures. It doesn't have explicit node structures, instead it consists of a meaningfully ordered array of values paired with a same length array of depth values.

```
struct Tree
{
    Array<Value> values;
    Array<int> depths;
};
```

The tree is formed by ordering the values in depth search order. Every node is assigned a corresponding integer value that tells the node's depth in the tree.

### Sample tree
```
                 root
             /    |     \
            /     |      \
           /      |       \
        node_1  node_2  node_3
        /   \       \
       /     \       \
      /       \       \
 node_1_1  node_1_2  node_2_1
              |
              | 
              |
          node_1_2_1

```
turns into two arrays 
```
values:    root, node_1, node_1_1, node_1_2, node_1_2_1, node_2, node_2_1, node_3
depths:    0,    1,      2,        2,        3,          1,      2,        1
```
which can be visualized like this:
```
           root---v----------------------------------------v-----------------v
                  |                                        |                 |
                 node_1---v----------v                   node_2----v        node_3
                          |          |                             |
                         node_1_1  node_1_2---v                  node_2_1
                                              |
                                             node_1_2_1
```

Packing the data thightly is ideal for cache performance as any related nodes are likely to be close by in memory. The topology of the tree is completely conveyed by the integer array alone making it fast to process.

Subtrees in this tree are contiguous blocks of memory, which makes shifting and removing them surprisingly efficient.

The performance of this tree can be further improved by introducing cached look-up tables. These look-up tables can hold any value related to every node such as parent, sibling or last descendant of the node. Since the data is in an array, it is simple and efficient to gather the necessary information for any of the mentioned look-up table types.

Operations that benefit from having a cache table include
* finding an Nth child of a node
 * cache type: next sibling cache
* deleting or moving a subtree
 * cache type: last descendant cache
* finding any of the node's ancestors in a single step
 * cache type: depth parent cache, requires 2D array, depth * count


# Test tree topology

The topology of the tree that is created on every test execution is deterministic and always the same.

The tree is built in a growing saw-tooth pattern:
* 1. root's first child has two children
* 1.1. first of these children has one child
* 1.1.1. it has no children
* 1.2. second has none
* 2. root's second child has three children
* 2.1. first of them has two children
* 2.1.1. first of those has one child
* 2.1.1.1. which has none
* 2.1.2. second has none
* 2.2. second has one child
* 2.2.1. which has none
* 2.3. third has none
* 3. root's third child has four children
* ...

The tree's topology can be described recursively as follows:
* Child count for root's direct children:
 * ```childCount = 2 + childIndex```
* Child count for every other node
 * ```childCount = parent's childCount - childIndex - 1```
* New nodes are added until target node count is reached.

Although the tree's topology is deterministic the ordering of child nodes is randomized. This varies the shape of the tree somewhat. The shape ARE identical between all tree types.

This topology was chosen as it roughly resembles real world tree structures. It is complicated enough to make branch prediction difficulty and automatic compiler optimizations impossible, while still being simple to construct iteratively.
