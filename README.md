This is a test program for three different tree data structures.

The tree types are
* Left-child right-sibling binary tree
* Child array tree
* Flat array tree

The test executes following operations on all of the trees
* add node
* move subtree to a different parent node
* traverse from root to a random leaf
* find maximum depth of the tree
* read every node of the tree in depth search order
* delete subtree

Additionally following operations are executed only on the pointer based trees as they are just simple look-up operations on the flat tree.
* count nodes
* travel to the tree's Nth node in depth search order

The purpose of this program is to precisely measure the execution times of the afore mentioned operations on all of the trees with varying node counts.

The CPU cache gets flushed before every measurement to ensure a clean execution state. This cache flushing is achieved by moving around several megabytes worth of heap memory via the CPU's memory pipeline. The majority of the programs execution time spent doing this.





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

While its nodes are small it suffers from linked-list like cache performance. This can to some extent be alleviated by allocating the Nodes from a pool as that way they will be more likely in local cache.

Nodes have no knowledge of their parent nodes. Finding a Node's parent requires recursing through whole the tree. This greatly affects the trees node deletion performance in particular as the node pointing to the one being deleted must be searched and manipulated.

Additions are fast in this tree 

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
turns into two blocks of 
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

Arranging the data thightly like this is ideal for cache performance. Any related nodes are likely to be close by in memory. The trees whole structure is conveyed by the integer array making it fast to search. The cache performance can be further improved by 



