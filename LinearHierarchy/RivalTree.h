#ifndef FLAT_RIVALTREE_H
#define FLAT_RIVALTREE_H

#include "FlatAssert.h"

#include <inttypes.h>
typedef uint32_t SizeType;

#ifndef FLAT_VECTOR
	#include <vector>
	template<typename ValueType>
	class FLAT_Vector_Type : public std::vector<ValueType alignas(16)>
	{
	public:
		void pushBack(const ValueType& v) { push_back(v); }
		size_t getSize() const { return size(); }
		void insert(size_t index, const ValueType& v) { std::vector<ValueType>::insert(begin() + index, v); }
		const ValueType& getBack() const { return back(); }
		ValueType& getBack() { return back(); }
		size_t getCapacity() const { return capacity(); }
		void zero(size_t start, size_t end) { return std::fill(begin() + start, begin() + end, 0); }
		ValueType* getPointer() { return &*begin(); }
		void erase(size_t index) { erase(begin() + index); }
	};
	
	#define FLAT_VECTOR FLAT_Vector_Type
#endif

static SizeType recursionCounter = 0;

template<typename ValueType>
struct RivalTreeNode
{
	RivalTreeNode()
		: value()
		, parent(NULL)
		, leftSibling(NULL)
		, rightSibling(NULL)
	{
	}

	RivalTreeNode* clear(RivalTreeNode* currentUnusedNode)
	{
		FLAT_ASSERT(++recursionCounter < 1024);

		parent = NULL;
		leftSibling = NULL;
		rightSibling = NULL;
		for (SizeType i = 0; i < children.getSize(); i++)
		{
			FLAT_ASSERT(children[i]->parent = this);
			currentUnusedNode = children[i]->clear(currentUnusedNode);
		}
		children.clear();

		nextUnusedNode = currentUnusedNode;
		currentUnusedNode = this;

		FLAT_ASSERT(recursionCounter-- > 0);
		return currentUnusedNode;
	}

	void sanityCheck()
	{
		FLAT_ASSERT(!rightSibling || rightSibling->leftSibling == this);
		FLAT_ASSERT(!leftSibling  || leftSibling->rightSibling == this);

		for (SizeType i = 0; i < children.getSize(); i++)
		{
			FLAT_ASSERT(children[i]->parent == this);

			FLAT_ASSERT(i <= 0                      || children[i - 1]->rightSibling  == children[i]);
			FLAT_ASSERT(i + 1 >= children.getSize() || children[i + 1]->leftSibling == children[i]);

			children[i]->sanityCheck();
		}
	}

	ValueType value;
	typedef FLAT_VECTOR<RivalTreeNode*> ChildVector;
	ChildVector children;

	union
	{
		RivalTreeNode* parent;
		RivalTreeNode* nextUnusedNode; // When node is unused, this will point to next unused node in linked list fashion.
	};
	RivalTreeNode* leftSibling;
	RivalTreeNode* rightSibling;
};



struct RivalDefaultSorter
{
	static const bool UseSorting = true;

	template<typename T>
	inline static bool isFirst(const T& a, const T& b) { return a < b; }
};



template<typename ValueType, typename Sorter = RivalDefaultSorter>
struct RivalTree
{
	typedef RivalTreeNode<ValueType> Node;

	Node* root;
	Node* firstUnusedNode;

	FLAT_VECTOR<Node*> allNodes;
	FLAT_VECTOR<FLAT_VECTOR<Node>*> treeNodeBuffers;

	RivalTree()
		: root(NULL)
		, firstUnusedNode(NULL)
	{
	}
	~RivalTree()
	{
		for (SizeType i = 0; i < treeNodeBuffers.getSize(); i++)
		{
			delete treeNodeBuffers[i];
		}
	}

	Node* createNode(const ValueType& value, Node* parent)
	{
		if (!firstUnusedNode)
		{
			treeNodeBuffers.pushBack(new FLAT_VECTOR<Node>());

			static const SizeType BufferSize = 1024 / sizeof(Node) < 4 ? 4 : 1024 / sizeof(Node);

			treeNodeBuffers.getBack()->resize(BufferSize);
			for (SizeType i = treeNodeBuffers.getBack()->getSize(); i-- > 0; )
			{
				Node& nnn = treeNodeBuffers.getBack()->operator[](i);
				nnn.nextUnusedNode = firstUnusedNode;
				firstUnusedNode = &nnn;
			}
		}

		Node* node = firstUnusedNode;
		firstUnusedNode = firstUnusedNode->nextUnusedNode;

		node->value = value;

		connectToParent(node, parent);

		allNodes.pushBack(node);

		return node;
	}
	void eraseNode(Node* node)
	{
		FLAT_ASSERT(node);
		disconnectFromParentAndSiblings(node);

		recursionCounter = 0;

		Node* oldFirstUnusedNode = firstUnusedNode;

		FLAT_ASSERT(node != root);
		FLAT_ASSERT(node->parent == NULL || !isChildOf(node->parent, node));
		firstUnusedNode = node->clear(firstUnusedNode);

		FLAT_ASSERT(recursionCounter == 0);

		Node* current = firstUnusedNode;
		while (current != oldFirstUnusedNode)
		{
			FLAT_ASSERT(current != NULL);
			FLAT_ASSERT(current != root);

			for (SizeType i = 0, end = allNodes.getSize(); i < end; i++)
			{
				if (allNodes[i] == current)
				{
					allNodes[i] = allNodes.getBack();
					allNodes.resize(allNodes.getSize() - 1U);
					break;
				}
			}
			current = current->nextUnusedNode;
		}
	}

	void makeChildOf(Node* node, Node* parent)
	{
		FLAT_ASSERT(parent);
		FLAT_ASSERT(!isChildOf(parent, node));

		disconnectFromParentAndSiblings(node);
		connectToParent(node, parent);
	}


	void disconnectFromParentAndSiblings(Node* node)
	{
		FLAT_ASSERT(node);
		if (node->parent)
		{
			Node::ChildVector& children = node->parent->children;
			for (SizeType i = 0; i < children.getSize(); i++)
			{
				if (children[i] == node)
				{
					children.erase(i);
					break;
				}
			}
			node->parent = NULL;
		}
		if (node->leftSibling)
		{
			node->leftSibling->rightSibling = node->rightSibling;
		}
		if (node->rightSibling)
		{
			node->rightSibling->leftSibling = node->leftSibling;
			node->rightSibling = NULL;
		}
		node->leftSibling = NULL;
	}

	void connectToParent(Node* node, Node* parent)
	{
		Node* left = NULL;
		Node* right = NULL;

		if (parent)
		{
			FLAT_ASSERT(!isChildOf(parent, node));

			Node::ChildVector& children = parent->children;
			if (Sorter::UseSorting == true)
			{
				bool inserted = false;
				for (SizeType i = 0; i < children.getSize(); i++)
				{
					if (Sorter::isFirst(node->value, children[i]->value))
					{
						if (i > 0)
						{
							left = children[i - 1];
							left->rightSibling = node;
						}

						right = children[i];
						right->leftSibling = node;

						children.insert(i, node);

						inserted = true;
						break;
					}
				}

				if (!inserted)
				{
					if (children.getSize() > 0)
					{
						left = children.getBack();
						left->rightSibling = node;
					}

					children.pushBack(node);
				}
			}
			else
			{
				if (children.getSize() > 0)
				{
					left = children.getBack();
					left->rightSibling = node;
				}
				children.insert(0, node);
			}
		}

		node->parent = parent;
		node->leftSibling = left;
		node->rightSibling = right;
		FLAT_ASSERT(!left  || left->rightSibling == node);
		FLAT_ASSERT(!right || right->leftSibling == node);
	}

	bool isChildOf(Node* child, Node* parent)
	{
		FLAT_ASSERT(child);
		FLAT_ASSERT(parent);
		
		SizeType emergenceEscape = 1024 * 1024 * 16;

		Node* current = child->parent;
		while (current != NULL)
		{
			if (current == parent)
				return true;
			current = current->parent;
			FLAT_ASSERT(current != child && "Child re-encountered");

			FLAT_ASSERT(emergenceEscape > 2);
			if (--emergenceEscape == 0)
			{
				break;
			}
		}
		return false;
	}

	Node* find(const ValueType& value)
	{
		struct Lolmbda
		{
			Node* recurse(const Node* n, const ValueType& v)
			{
				if (n->value == v)
				{
					return n;
				}

				for (SizeType i = 0; i < n->children.getSize(); i++)
				{
					if (Node* n = recurse(n->children[i], v))
						return n;
				}
				return NULL;
			}
		};

		return Lolmbda::recurse(root, value);
	}
};


template<typename ValueType, typename Sorter = RivalDefaultSorter>
struct NaiveTree
{
	typedef RivalTreeNode<ValueType> Node;
	Node* root;

	NaiveTree()
		: root(NULL)
	{
	}
	~NaiveTree()
	{
		if(root)
			eraseNode(root);
	}

	Node* createNode(const ValueType& value, Node* parent)
	{
		Node* node = new Node();
		node->value = value;
		connectToParent(node, parent);
		return node;
	}
	void eraseNode(Node* node)
	{
		FLAT_ASSERT(node);
		disconnectFromParentAndSiblings(node);

		struct Lolmbda
		{
			static void recurse(const Node* n)
			{
				for (SizeType i = 0; i < n->children.getSize(); i++)
				{
					recurse(n->children[i]);
				}
				delete n;
			}
		};

		Lolmbda::recurse(node);
	}

	void makeChildOf(Node* node, Node* parent)
	{
		FLAT_ASSERT(parent);
		FLAT_ASSERT(!isChildOf(parent, node));

		disconnectFromParentAndSiblings(node);
		connectToParent(node, parent);
	}


	void disconnectFromParentAndSiblings(Node* node)
	{
		FLAT_ASSERT(node);
		if (node->parent)
		{
			Node::ChildVector& children = node->parent->children;
			for (SizeType i = 0; i < children.getSize(); i++)
			{
				if (children[i] == node)
				{
					children.erase(i);
					break;
				}
			}
			node->parent = NULL;
		}
		if (node->leftSibling)
		{
			node->leftSibling->rightSibling = node->rightSibling;
		}
		if (node->rightSibling)
		{
			node->rightSibling->leftSibling = node->leftSibling;
			node->rightSibling = NULL;
		}
		node->leftSibling = NULL;
	}

	void connectToParent(Node* node, Node* parent)
	{
		Node* left = NULL;
		Node* right = NULL;

		if (parent)
		{
			FLAT_ASSERT(!isChildOf(parent, node));

			Node::ChildVector& children = parent->children;
			if (Sorter::UseSorting == true)
			{
				bool inserted = false;
				for (SizeType i = 0; i < children.getSize(); i++)
				{
					if (Sorter::isFirst(node->value, children[i]->value))
					{
						if (i > 0)
						{
							left = children[i - 1];
							left->rightSibling = node;
						}

						right = children[i];
						right->leftSibling = node;

						children.insert(i, node);

						inserted = true;
						break;
					}
				}

				if (!inserted)
				{
					if (children.getSize() > 0)
					{
						left = children.getBack();
						left->rightSibling = node;
					}

					children.pushBack(node);
				}
			}
			else
			{
				if (children.getSize() > 0)
				{
					right = children.getBack();
					right->leftSibling = node;
				}
				children.insert(0, node);
			}
		}

		node->parent = parent;
		node->leftSibling = left;
		node->rightSibling = right;
		FLAT_ASSERT(!left || left->rightSibling == node);
		FLAT_ASSERT(!right || right->leftSibling == node);
	}

	bool isChildOf(Node* child, Node* parent)
	{
		FLAT_ASSERT(child);
		FLAT_ASSERT(parent);
		Node* current = child;
		while (current != NULL)
		{
			if (current == parent)
				return true;
			current = current->parent;
			FLAT_ASSERT(current != child && "Child re-encountered");
		}
		return false;
	}

	Node* find(const ValueType& value)
	{
		struct Lolmbda
		{
			Node* recurse(const Node* n, const ValueType& v)
			{
				if (n->value == v)
				{
					return n;
				}

				for (SizeType i = 0; i < n->children.getSize(); i++)
				{
					if (Node* n = recurse(n->children[i], v))
						return n;
				}
				return NULL;
			}
		};

		return Lolmbda::recurse(root, value);
	}
};



#endif