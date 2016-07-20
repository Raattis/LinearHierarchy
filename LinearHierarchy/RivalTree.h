#ifndef FLAT_RIVALTREE_H
#define FLAT_RIVALTREE_H

#ifndef FLAT_ASSERT
	#if FLAT_ASSERTS_ENABLED == true
		#include <assert.h>
		#define FLAT_ASSERT(expr) assert(expr)
	#else
		#define FLAT_ASSERT(expr) do{}while(false)
	#endif
#endif

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

template<typename ValueType>
struct RivalTreeNode
{
	RivalTreeNode()
		: value()
		, parent(NULL)
		, leftSibling(NULL)
		, rightSibling(NULL)
		, isInUse(false)
	{
	}

	RivalTreeNode* clear(RivalTreeNode* currentUnusedNode)
	{
		parent = NULL;
		leftSibling = NULL;
		rightSibling = NULL;
		nextUnusedNode = currentUnusedNode;
		currentUnusedNode = this;
		for (SizeType i = 0; i < children.getSize(); i++)
		{
			currentUnusedNode = children[i]->clear(currentUnusedNode);
		}
		children.clear();
		isInUse = false;
		return currentUnusedNode;
	}

	void init()
	{
		isInUse = true;
		for (SizeType i = 0; i < children.getSize(); i++)
		{
			children[i]->init();
		}
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
		RivalTreeNode* nextUnusedNode;
	};
	RivalTreeNode* leftSibling;
	RivalTreeNode* rightSibling;
	bool isInUse;
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

	FLAT_VECTOR<FLAT_VECTOR<Node>*> treeNodeBuffers;
	Node* firstUnusedNode;

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
		node->init();

		connectToParent(node, parent);

		return node;
	}
	void eraseNode(Node* node)
	{
		FLAT_ASSERT(node);
		disconnectFromParentAndSiblings(node);

		firstUnusedNode = node->clear(firstUnusedNode);
	}

	void makeChildOf(Node* node, Node* parent)
	{
		FLAT_ASSERT(parent);
		FLAT_ASSERT(!isChildOf(parent, node));

		if (node->parent != parent)
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
				children.pushBack(node);
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
};




#endif