This is a real world performance test program for three different tree data structures.

The tree types are
* Left-child right-sibling binary tree
 * (https://en.wikipedia.org/wiki/Left_child_right_sibling)
* Child array tree
 * naive and intuitive
* Flat array tree
 * new

The test executes following operations on every tree type.
* add node
* traverse from root to a random leaf
* find maximum depth of the tree
* read every node of the tree in depth search order
* move subtree to a different parent node
* delete subtree

Additionally the following operations are executed only on the pointer based trees. Flat array is excluded from these tests as they are just simple look-up operations on it.
* count nodes
* travel to the tree's Nth node in depth search order

The tests are run using multiple different tree sizes ranging from 10 nodes to 25,000 nodes. Every test is repeated several times as to mitigate random variance caused by the testing environment.

Tests are timed one operation at a time to ensure that CPU cache is in a clean state during the test. The CPU cache is flushed before every measurement by moving around several megabytes worth of heap memory via the CPU's memory pipeline. The vast majority of the program's execution time is spent doing this (around 99.75 %). From the perspective of the CPU this program mostly shuffles memory around and occasionally does some calculations.

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

While its nodes are small it suffers from linked-list like cache performance. This can to some extent be alleviated by allocating the nodes from a pool. This way it is more likely that related nodes are in a same cache line.

In this tree nodes have no knowledge of their parent. Finding a parent of a node requires recursing through the whole tree. This greatly affects the trees node deletion performance in particular as the node pointing to the one being deleted must be searched and its pointers be manipulated.

Additions are fast in this tree as they only require manipulation of a single pointer. That being said, searching and fetching the node that will point to the created node is slow.

More info here: https://en.wikipedia.org/wiki/Left-node_right-sibling_binary_tree

## Child array tree
Child array tree is a naïve but still viable data structure in most cases - it's the first thing that usually comes to mind when thinking of trees with multiple child nodes. Every node consists of a parent pointer and an array of child pointers. Additionally the node can also have left and right sibling pointers, which I have decided to include as several implementations I happened to come across had them.

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

Child nodes in the child array tree are, as the name suggests, in an array which enables random order accessing. Though, this is somewhat undermined by the fact that the nodes still have to be referenced by pointer, and any information about them can cause a possible cache miss.

While, compared to left-child right-sibling binary tree, child array tree is much less like linked-list its cache performance can still be greatly improved by allocating the nodes from a pool.

## Flat array tree
Flat array tree is (as far as I know) a new data structure to describe a tree. It doesn't have explicit node objects, instead it consists of a meaningfully ordered array of values paired with a same length array of depth values.

```
struct Tree
{
    Array<Value> values;
    Array<int> depths;
};
```

The tree is formed by ordering the values in depth search order. Every node is assigned a corresponding integer value that tells the node's depth in the tree.

### Sample tree
This sample tree
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
corresponds to this block of memory
```
values:    root, node_1, node_1_1, node_1_2, node_1_2_1, node_2, node_2_1, node_3
depths:    0,    1,      2,        2,        3,          1,      2,        1
```
which can be more clearly visualized like this:
```
           root---v----------------------------------------v-----------------v
                  |                                        |                 |
                 node_1---v----------v                   node_2----v        node_3
                          |          |                             |
                         node_1_1  node_1_2---v                  node_2_1
                                              |
                                             node_1_2_1
```

Packing the data thightly is ideal for cache performance as any related nodes are likely to be close by in memory. The topology of the tree is completely conveyed by the depth array alone making it fast to seek through.

Subtrees in this tree are contiguous blocks of memory, which makes shifting and removing them rather efficient.

The performance of this tree can be further improved by introducing cached look-up tables. These look-up tables usually hold index values related to the corresponding node such as parent, sibling or last descendant of the node. Since the data is a simple array, it is efficient to gather usually in a single pass.

Operations that benefit from having a cache table include
* finding an Nth child of a node
 * cache type: next sibling cache
* deleting or moving a subtree
 * cache type: last descendant cache
* finding any one of the node's ancestors in a single step
 * cache type: deep parent cache, depth * count, 2D array


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
* Child count for all of the other nodes
 * ```childCount = parent's childCount - childIndex - 1```
* New nodes are added until target node count is reached.

Although the tree's topology is deterministic the ordering of child nodes is randomized. This varies the shape of the tree somewhat. Both the topology and the shape of the built tree is identical for every tested tree structure type to ensure fair evaluation.

This topology was chosen as it roughly resembles real world tree structures. It is complicated enough to make branch prediction difficulty and automatic compiler optimizations impossible, while still being simple to construct iteratively.

# Test execution

## Validity
The test data is checksummed in every stage of testing for every tree. This is done to ensure that every tree type is doing the same work and arriving at the same results.

## Timing
The timing is done using Window's QueryPerformanceCounter. It's maximum precision on the test machine is 0.3198 µs. Due to this the results on the lower tree sizes tend to cluster on the multiples of this number. To combat this the tests are run an excessive number of times to achieve as noise free results as possible, but due to cache flushing done before every test this is very time consuming.

A full run of the tests takes over 7 hours on an Intel i7 5820k processor.

## Cache flushing
Cache flushing is done before every timed operation to ensure that as little of the test data is prefetched to CPU's cache. The cache flushing is achieved via brute force means by pumping several megabytes of heap memory through the CPU in unpredictable order. Unfortunately this takes wastly more time than the actual tests.

## Repetitions
Every combination of tree type and treesize is tested multiple times to reduce noise. On the lower tree sizes the repetition count is higher than on larger tree sizes. The numbers are chosen to balance noise and execution time.

## The function
The test function can be abreviated like so:
```
void test()
{
    for (tree type count)
    {
        initialize_random();
        
        for (tree size count)
        {
            Tree tree;
            for (test repetition count for current size)
            {
                while(tree's node count < tree size)
                    add_node(tree);
                
                // N is just an arbitary number. The size of the tree is used for this in the tests, but it doesn't have to be
                for (N)
                    travel_to_random_leaf(tree);
                
                for (i = 0 .. N)
                    find_node(tree, get_node(tree, i));
                
                for (N / 10)
                    find_max_depth_and_count(tree);
                
                for (20)
                    // The content of the test trees' nodes is a transform struct resembling a matrix
                    // In this test the whole tree is traversed and the matrices are multiplied from parent to children
                    do_transform_multiplication(tree);
                
                for (N / 10)
                    // Moves a subtree starting from one node to a new parent
                    move_node(tree, get_node(random()), get_node(random()));
                
                while (tree's node count > 0.8 * tree size)
                    erase_node(tree, get_node(random()));
            }
        }
    }
}
```
