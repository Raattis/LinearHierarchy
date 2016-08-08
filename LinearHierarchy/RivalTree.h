#ifndef FLAT_RIVALTREE_H
#define FLAT_RIVALTREE_H

#include "FlatAssert.h"

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

struct RivalTreeNodeBase
{
	typedef FLAT_VECTOR<RivalTreeNodeBase*> ChildVector;
	ChildVector children;
	union
	{
		RivalTreeNodeBase* parent;
		RivalTreeNodeBase* nextUnusedNode; // When node is unused, this will point to next unused node in linked list fashion.
	};
	RivalTreeNodeBase* leftSibling;
	RivalTreeNodeBase* rightSibling;

	RivalTreeNodeBase* clear(RivalTreeNodeBase* currentUnusedNode)
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
};


template<typename ValueType>
struct RivalTreeNode : public RivalTreeNodeBase
{
	RivalTreeNode()
		: value()
	{
		parent = NULL;
		leftSibling = NULL;
		rightSibling = NULL;
	}

	void sanityCheck()
	{
		FLAT_ASSERT(!rightSibling || rightSibling->leftSibling == this);
		FLAT_ASSERT(!leftSibling || leftSibling->rightSibling == this);

		for (SizeType i = 0; i < children.getSize(); i++)
		{
			FLAT_ASSERT(children[i]->parent == this);

			FLAT_ASSERTF(i <= 0 || children[i - 1]->rightSibling == children[i], "This: %s,  Left: %s", toString(children[i]).ptr, toString(children[i - 1]).ptr);
			FLAT_ASSERTF(i + 1 >= children.getSize() || children[i + 1]->leftSibling == children[i], "This: %s, Right: %s", toString(children[i]).ptr, toString(children[i + 1]).ptr);

			((RivalTreeNode<ValueType>*)children[i])->sanityCheck();
		}
	}

	String getValue() const
	{
		if (!this)
			return String("NULL", 4);
		return String(value.toString(), 4);
	}
	static String toString(const RivalTreeNodeBase* node)
	{
		return ((const RivalTreeNode<ValueType>*)node)->toString();
	}
	String toString() const
	{
		if (!this)
			return String("NULL", 4);
		String result(getValue());
		result += " (L: ";
		result += ((const RivalTreeNode<ValueType>*)leftSibling)->getValue();
		result += ", R: ";
		result += ((const RivalTreeNode<ValueType>*)rightSibling)->getValue();
		result += ")";
		return result;
	}

	ValueType value;
};




struct RivalDefaultSorter
{
	static const bool UseSorting = true;

	template<typename T>
	inline static bool isFirst(const T& a, const T& b) { return a < b; }
};

void disconnectFromParentAndSiblings(RivalTreeNodeBase* node)
{
	FLAT_ASSERT(node);
	if (node->parent)
	{
		RivalTreeNodeBase::ChildVector& children = node->parent->children;
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

bool isChildOf(RivalTreeNodeBase* child, RivalTreeNodeBase* parent)
{
	FLAT_ASSERT(child);
	FLAT_ASSERT(parent);

	RivalTreeNodeBase* current = child->parent;
	while (current != NULL)
	{
		if (current == parent)
			return true;
		current = current->parent;
		FLAT_ASSERT(current != child && "Child re-encountered");
	}
	return false;
}

template<typename Sorter, typename Node>
void connectToParent(RivalTreeNodeBase* node, RivalTreeNodeBase* parent)
{
	RivalTreeNodeBase* left = NULL;
	RivalTreeNodeBase* right = NULL;

	if (parent)
	{
		FLAT_ASSERT(!isChildOf(parent, node));

		RivalTreeNodeBase::ChildVector& children = parent->children;
		if (Sorter::UseSorting == true)
		{
			bool inserted = false;
			for (SizeType i = 0; i < children.getSize(); i++)
			{
				if (Sorter::isFirst(((Node*)node)->value, ((Node*)children[i])->value))
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
				left = children[0];
				left->rightSibling = node;
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

template<typename Sorter, typename Node>
void makeChildOfImpl(Node* node, Node* parent)
{
	FLAT_ASSERT(parent);
	FLAT_ASSERT(!isChildOf(parent, node));

	disconnectFromParentAndSiblings(node);
	connectToParent<Sorter, Node>(node, parent);
}

static SizeType findCount(const RivalTreeNodeBase* node)
{
	SizeType result = 1;
	for (SizeType i = 0; i < node->children.getSize(); i++)
	{
		result += findCount(node->children[i]);
	}
	return result;
}

static SizeType findDepth(const RivalTreeNodeBase* node)
{
	SizeType maxDepth = 0;
	for (SizeType i = 0; i < node->children.getSize(); i++)
	{
		SizeType d = findDepth(node->children[i]) + 1;
		if (d > maxDepth)
			maxDepth = d;
	}
	return maxDepth;
}

//Node* find(const ValueType& value)
//{
//	struct Lolmbda
//	{
//		Node* recurse(const Node* n, const ValueType& v)
//		{
//			if (n->value == v)
//			{
//				return n;
//			}
//
//			for (SizeType i = 0; i < n->children.getSize(); i++)
//			{
//				if (Node* n = recurse(n->children[i], v))
//					return n;
//			}
//			return NULL;
//		}
//	};
//
//	return Lolmbda::recurse(root, value);
//}

template<typename ValueType, typename Sorter = RivalDefaultSorter>
struct RivalTree
{
	typedef RivalTreeNode<ValueType> Node;

	Node* root;
	RivalTreeNodeBase* firstUnusedNode;

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
			const SizeType BufferSize = (treeNodeBuffers.getSize() <= 25) ? (64 << treeNodeBuffers.getSize()) : (0xFFFFFFFF);

			treeNodeBuffers.pushBack(new FLAT_VECTOR<Node>());

			treeNodeBuffers.getBack()->resize(BufferSize);

			for (SizeType i = treeNodeBuffers.getBack()->getSize(); i-- > 0; )
			{
				Node& nnn = treeNodeBuffers.getBack()->operator[](i);
				nnn.nextUnusedNode = firstUnusedNode;
				firstUnusedNode = &nnn;

				// Constructor for child vector doesn't get called automatically, so set its valus to 0
				nnn.children.size = 0;
				nnn.children.capacity = 0;
				nnn.children.buffer = 0;
			}
		}

		Node* node = (Node*)firstUnusedNode;
		firstUnusedNode = firstUnusedNode->nextUnusedNode;

		node->value = value;

		connectToParent<Sorter, Node>(node, parent);
		return node;
	}

	void eraseNode(RivalTreeNodeBase* node)
	{
		FLAT_ASSERT(node);
		disconnectFromParentAndSiblings(node);

		RivalTreeNodeBase* oldFirstUnusedNode = firstUnusedNode;

		FLAT_ASSERT(node != root);
		FLAT_ASSERT(node->parent == NULL || !isChildOf(node->parent, node));

		FLAT_ASSERT(recursionCounter == 0);

		firstUnusedNode = node->clear(firstUnusedNode);

		FLAT_ASSERT(recursionCounter == 0);
	}

	void makeChildOf(Node* node, Node* parent)
	{
		makeChildOfImpl<Sorter>(node, parent);
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
		if (root)
			eraseNode(root);
		root = NULL;
	}

	Node* createNode(const ValueType& value, RivalTreeNodeBase* parent)
	{
		Node* node = new Node();
		node->value = value;
		connectToParent<Sorter, Node>(node, parent);
		return node;
	}
	void eraseNode(RivalTreeNodeBase* node)
	{
		FLAT_ASSERT(node);
		disconnectFromParentAndSiblings(node);

		struct Lolmbda
		{
			static void recurse(const RivalTreeNodeBase* n)
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
		makeChildOfImpl<Sorter>(node, parent);
	}
};





#endif