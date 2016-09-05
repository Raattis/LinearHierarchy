#ifndef FLAT_MULTIWAYTREE_H
#define FLAT_MULTIWAYTREE_H

#include "FlatAssert.h"

typedef uint32_t SizeType;

#ifndef FLAT_VECTOR
	#error "FLAT_VECTOR not defined"
#endif

struct MultiwayTreeNodeBase
{
	MultiwayTreeNodeBase* child;
	MultiwayTreeNodeBase* sibling;
};


template<typename ValueType>
struct MultiwayTreeNode : public MultiwayTreeNodeBase
{
	MultiwayTreeNode()
		: value()
	{
		child = NULL;
		sibling = NULL;
	}

	void sanityCheck()
	{
		//FLAT_ASSERT(!rightSibling || rightSibling->leftSibling == this);
		//FLAT_ASSERT(!leftSibling || leftSibling->rightSibling == this);
		//
		//for (SizeType i = 0; i < children.getSize(); i++)
		//{
		//	FLAT_ASSERT(children[i]->parent == this);
		//
		//	FLAT_ASSERTF(i <= 0 || children[i - 1]->rightSibling == children[i], "This: %s,  Left: %s", toString(children[i]).ptr, toString(children[i - 1]).ptr);
		//	FLAT_ASSERTF(i + 1 >= children.getSize() || children[i + 1]->leftSibling == children[i], "This: %s, Right: %s", toString(children[i]).ptr, toString(children[i + 1]).ptr);
		//
		//	((MultiwayTreeNode<ValueType>*)children[i])->sanityCheck();
		//}
	}
	String getValue() const
	{
		if (!this)
			return String("NULL", 4);
		return String(value.toString(), 4);
	}
	static String toString(const MultiwayTreeNodeBase* node)
	{
		return ((const MultiwayTreeNode<ValueType>*)node)->toString();
	}
	String toString() const
	{
		if (!this)
			return String("NULL", 4);
		String result(getValue());
		result += " (L: ";
		result += ((const MultiwayTreeNode<ValueType>*)leftSibling)->getValue();
		result += ", R: ";
		result += ((const MultiwayTreeNode<ValueType>*)rightSibling)->getValue();
		result += ")";
		return result;
	}

	ValueType value;
};




struct MultiwayDefaultSorter
{
	static const bool UseSorting = true;

	template<typename T>
	inline static bool isFirst(const T& a, const T& b) { return a < b; }
};

// Find parent or left sibling
MultiwayTreeNodeBase* findPointingNode(MultiwayTreeNodeBase* root, MultiwayTreeNodeBase* node)
{
	FLAT_ASSERTF(root != NULL, "Root is NULL.");
	FLAT_ASSERTF(node != NULL, "Node is NULL.");

	if (root->child == node || root->sibling == node)
	{
		return root;
	}
	if (root->child != NULL)
	{
		if (MultiwayTreeNodeBase* temp = findPointingNode(root->child, node))
		{
			return temp;
		}
	}
	if (root->sibling != NULL)
	{
		if (MultiwayTreeNodeBase* temp = findPointingNode(root->sibling, node))
		{
			return temp;
		}
	}

	return NULL;
}

void disconnectFromParentAndSiblings(MultiwayTreeNodeBase* pointingNode, MultiwayTreeNodeBase* node)
{
	FLAT_ASSERT(pointingNode != NULL);
	FLAT_ASSERT(node != NULL);
	FLAT_ASSERTF(pointingNode->child == node || pointingNode->sibling == node, "Not the node you are looking for.");

	if (pointingNode->child == node)
		pointingNode->child = node->sibling;
	else
		pointingNode->sibling = node->sibling;
	node->sibling = NULL;
}

bool isChildOf(MultiwayTreeNodeBase* child, MultiwayTreeNodeBase* parent)
{
	return parent->child == child || (parent->child != NULL && findPointingNode(parent->child, child) != NULL);
}

template<typename Sorter, typename Node>
void connectToParent(MultiwayTreeNodeBase* node, MultiwayTreeNodeBase* parent)
{
	if (parent)
	{
		FLAT_ASSERT(!isChildOf(parent, node));

		if (Sorter::UseSorting == true)
		{
			if (parent->child == NULL)
			{
				parent->child = node;
			}
			else
			{
				MultiwayTreeNodeBase* child = parent->child;

				bool inserted = false;
				while (child->sibling != NULL && !Sorter::isFirst(((Node*)node)->value, ((Node*)child->sibling)->value))
				{
					child = child->sibling;
				}

				node->sibling = child->sibling;
				child->sibling = node;
			}
		}
		else
		{
			node->sibling = parent->child;
			parent->child = node;
		}
	}
}

template<typename Sorter, typename Node>
void makeChildOfImpl(MultiwayTreeNodeBase* node, MultiwayTreeNodeBase* parent, MultiwayTreeNodeBase* pointingNode)
{
	FLAT_ASSERT(parent);
	FLAT_ASSERT(!isChildOf(parent, node));

	disconnectFromParentAndSiblings(pointingNode, node);
	connectToParent<Sorter, Node>(node, parent);
}

static SizeType getChildCount(MultiwayTreeNodeBase* parent)
{
	SizeType result = 0;
	MultiwayTreeNodeBase* node = parent->child;
	while (node != NULL)
	{
		node = node->sibling;
		++result;
	}
	return result;
}

static MultiwayTreeNodeBase* getNthChild(MultiwayTreeNodeBase* parent, SizeType n)
{
	MultiwayTreeNodeBase* result = parent->child;
	while (n > 0 && result != NULL)
	{
		result = result->sibling;
		--n;
	}
	return result;
}

static SizeType findCount(const MultiwayTreeNodeBase* node)
{
	if (node == NULL)
		return 0;
	return 1 + findCount(node->child) + findCount(node->sibling);
}

static SizeType findDepth(const MultiwayTreeNodeBase* node)
{
	if (node == NULL)
		return 0;
	SizeType ownDepth = 1 + findDepth(node->child);
	SizeType siblingDepth = findDepth(node->sibling);
	return ownDepth > siblingDepth ? ownDepth : siblingDepth;
}

template<typename ValueType, typename Sorter = MultiwayDefaultSorter>
struct MultiwayTree
{
	typedef MultiwayTreeNode<ValueType> Node;

	Node* root;
	MultiwayTreeNodeBase* firstUnusedNode;

	FLAT_VECTOR<FLAT_VECTOR<Node>*> treeNodeBuffers;

	MultiwayTree()
		: root(NULL)
		, firstUnusedNode(NULL)
	{
	}
	~MultiwayTree()
	{
		for (SizeType i = 0; i < treeNodeBuffers.getSize(); i++)
		{
			delete treeNodeBuffers[i];
		}
	}

	void removeRoot()
	{
		struct LOLMBDA
		{
			static MultiwayTreeNodeBase* gather(MultiwayTreeNodeBase* node, MultiwayTreeNodeBase* unusedNode)
			{
				FLAT_ASSERT(node != NULL);

				if (node->child)
				{
					unusedNode = gather(node->child, unusedNode);
				}
				if (node->sibling)
				{
					unusedNode = gather(node->sibling, unusedNode);
				}

				node->child = NULL;
				node->sibling = unusedNode;
				return node;
			}
		};

		firstUnusedNode = LOLMBDA::gather(root, firstUnusedNode);
		root = NULL;
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
				nnn.sibling = firstUnusedNode;
				firstUnusedNode = &nnn;
			}
		}

		Node* node = (Node*)firstUnusedNode;
		firstUnusedNode = firstUnusedNode->sibling;

		node->child = NULL;
		node->sibling = NULL;
		node->value = value;

		connectToParent<Sorter, Node>(node, parent);
		return node;
	}

	void eraseNode(MultiwayTreeNodeBase* node)
	{
		FLAT_ASSERT(node);
		FLAT_ASSERT(node != root);
		
		disconnectFromParentAndSiblings(findPointingNode(root, node), node);

		FLAT_ASSERT(recursionCounter == 0);

		struct LOLMBDA
		{
			static MultiwayTreeNodeBase* gather(MultiwayTreeNodeBase* node, MultiwayTreeNodeBase* unusedNode)
			{
				FLAT_ASSERT(node != NULL);

				if(node->child)
				{
					unusedNode = gather(node->child, unusedNode);
				}
				if (node->sibling)
				{
					unusedNode = gather(node->sibling, unusedNode);
				}

				node->child = NULL;
				node->sibling = unusedNode;
				return node;
			}
		};

		firstUnusedNode = LOLMBDA::gather(node, firstUnusedNode);

		FLAT_ASSERT(recursionCounter == 0);
	}

	void makeChildOf(MultiwayTreeNodeBase* node, MultiwayTreeNodeBase* parent)
	{
		makeChildOfImpl<Sorter, Node>(node, parent, findPointingNode(root, node));
	}
};

template<typename ValueType, typename Sorter = MultiwayDefaultSorter>
struct NaiveMultiwayTree
{
	typedef MultiwayTreeNode<ValueType> Node;
	Node* root;

	NaiveMultiwayTree()
		: root(NULL)
	{
	}
	~NaiveMultiwayTree()
	{
		removeRoot();
	}
	void removeRoot()
	{
		struct LOLMBDA
		{
			static void eraseNode(MultiwayTreeNodeBase* node)
			{
				if (node == NULL)
					return;

				MultiwayTreeNodeBase* child = node->child;
				MultiwayTreeNodeBase* sibling = node->sibling;
				delete node;

				eraseNode(child);
				eraseNode(sibling);
			}
		};

		LOLMBDA::eraseNode(root);
		root = NULL;
	}
	Node* createNode(const ValueType& value, MultiwayTreeNodeBase* parent)
	{
		Node* node = new Node();
		node->value = value;
		connectToParent<Sorter, Node>(node, parent);
		return node;
	}
	void eraseNode(MultiwayTreeNodeBase* node)
	{
		FLAT_ASSERT(node);
		disconnectFromParentAndSiblings(findPointingNode(root, node), node);

		struct LOLMBDA
		{
			static void eraseNode(MultiwayTreeNodeBase* node)
			{
				if (node == NULL)
					return;

				MultiwayTreeNodeBase* child = node->child;
				MultiwayTreeNodeBase* sibling = node->sibling;
				delete node;

				eraseNode(child);
				eraseNode(sibling);
			}
		};

		LOLMBDA::eraseNode(node);
	}

	void makeChildOf(MultiwayTreeNodeBase* node, MultiwayTreeNodeBase* parent)
	{
		makeChildOfImpl<Sorter, Node>(node, parent, findPointingNode(root, node));
	}
};





#endif