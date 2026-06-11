/**
 * @package Quantum.Core
 * @file Include/Quantum/Structures/Tree.hpp
 * @brief Declaration of tree structures.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

#include "Node.hpp"

/**
 * @brief Tree structures.
 */
namespace Quantum::Structures::Trees {
  /**
   * @brief Base class for binary tree structures.
   * @tparam ValueType The type of values stored in the tree.
   * @tparam NodeType The type of node used in the tree.
   *
   * Provides common functionality for managing a tree of nodes containing
   * values of type `ValueType`.
   */
  template <typename ValueType, typename NodeType>
  class Tree {
    public:
      /**
       * @brief Constructs an empty tree.
       */
      explicit Tree() = default;

      /**
       * @brief Destructor for the tree. Frees all nodes.
       */
      ~Tree() { DestroySubtree(_root); }

      /**
       * @brief Gets the root node of the tree.
       * @return Pointer to the root node of the tree.
       */
      NodeType* GetRoot() const { return _root; }

      /**
       * @brief Gets the number of nodes in the tree.
       * @return The count of nodes in the tree.
       */
      Size GetCount() const { return _count; }

      /**
       * @brief Checks whether the tree is empty.
       * @return `true` if the tree has no nodes, false otherwise.
       */
      bool IsEmpty() const { return _root == nullptr; }

    protected:
      /**
       * @brief Pointer to the root node of the tree.
       */
      NodeType* _root = nullptr;

      /**
       * @brief The number of nodes in the tree.
       */
      Size _count = 0;

      /**
       * @brief Recursively destroys all nodes in a subtree.
       * @param node The root of the subtree to destroy.
       */
      void DestroySubtree(NodeType* node) {
        if (!node) return;

        DestroySubtree(static_cast<NodeType*>(node->GetLeft()));
        DestroySubtree(static_cast<NodeType*>(node->GetRight()));

        delete node;
      }

      /**
       * @brief Finds the node with the minimum value in a subtree.
       * @param node The root of the subtree to search.
       * @return Pointer to the node with the minimum value, or `nullptr` if
       *         the subtree is empty.
       */
      NodeType* FindMinimum(NodeType* node) const {
        if (!node) return nullptr;

        while (node->GetLeft()) node = static_cast<NodeType*>(node->GetLeft());

        return node;
      }

      /**
       * @brief Finds the node with the maximum value in a subtree.
       * @param node The root of the subtree to search.
       * @return Pointer to the node with the maximum value, or `nullptr` if
       *         the subtree is empty.
       */
      NodeType* FindMaximum(NodeType* node) const {
        if (!node) return nullptr;

        while (node->GetRight())
          node = static_cast<NodeType*>(node->GetRight());

        return node;
      }

      /**
       * @brief Replaces one subtree as a child of its parent with another
       *        subtree.
       * @param currentNode The node to be replaced.
       * @param newNode The replacement node (may be `nullptr`).
       */
      void Transplant(NodeType* currentNode, NodeType* newNode) {
        if (!currentNode->GetParent()) _root = newNode;
        else if (
          currentNode == static_cast<NodeType*>(
            currentNode->GetParent()->GetLeft()
          )
        ) currentNode->GetParent()->SetLeft(newNode);
        else currentNode->GetParent()->SetRight(newNode);

        if (newNode) newNode->SetParent(currentNode->GetParent());
      }
  };

  /**
   * @brief Template class representing a binary search tree.
   * @tparam ValueType
   *   The type of values stored in the tree. Must support `operator<`.
   * @note
   *   Duplicate values are not inserted; inserting an existing value returns
   *   the existing node.
   */
  template <typename ValueType>
  class BinarySearchTree : public Tree<ValueType, Nodes::BinaryTreeNode<ValueType>> {
    public:
      /**
       * @brief Constructs an empty binary search tree.
       */
      explicit BinarySearchTree() = default;

      /**
       * @brief Inserts a value into the tree. If the value already exists,
       *        the existing node is returned without modification.
       * @param value The value to insert.
       * @return Pointer to the inserted or existing node.
       */
      Nodes::BinaryTreeNode<ValueType>* Insert(const ValueType& value) {
        auto* node = new Nodes::BinaryTreeNode<ValueType>(value);

        if (!this->_root) {
          this->_root = node;
          this->_count++;

          return node;
        }

        Nodes::BinaryTreeNode<ValueType>* current = this->_root;
        Nodes::BinaryTreeNode<ValueType>* parent = nullptr;

        while (current) {
          parent = current;

          if (value < current->GetValue()) current = current->GetLeft();
          else if (current->GetValue() < value) current = current->GetRight();
          else {
            delete node;

            return current;
          }
        }

        node->SetParent(parent);

        if (value < parent->GetValue()) parent->SetLeft(node);
        else parent->SetRight(node);

        this->_count++;

        return node;
      }

      /**
       * @brief Searches for a value in the tree.
       * @param value The value to search for.
       * @return Pointer to the node containing the value, or `nullptr` if
       *         not found.
       */
      Nodes::BinaryTreeNode<ValueType>* Search(const ValueType& value) const {
        auto* current = this->_root;

        while (current) {
          if (value < current->GetValue()) current = current->GetLeft();
          else if (current->GetValue() < value) current = current->GetRight();
          else return current;
        }

        return nullptr;
      }

      /**
       * @brief Checks whether the tree contains a value.
       * @param value The value to search for.
       * @return `true` if the value exists in the tree, false otherwise.
       */
      bool Contains(const ValueType& value) const {
        return Search(value) != nullptr;
      }

      /**
       * @brief Removes a value from the tree.
       * @param value The value to remove.
       * @return `true` if the value was found and removed, false otherwise.
       */
      bool Remove(const ValueType& value) {
        auto* node = Search(value);

        if (!node) return false;

        RemoveNode(node);

        return true;
      }

      /**
       * @brief Gets the node with the minimum value in the tree.
       * @return Pointer to the node with the minimum value, or `nullptr` if
       *         the tree is empty.
       */
      Nodes::BinaryTreeNode<ValueType>* GetMinimum() const {
        return this->FindMinimum(this->_root);
      }

      /**
       * @brief Gets the node with the maximum value in the tree.
       * @return Pointer to the node with the maximum value, or `nullptr` if
       *         the tree is empty.
       */
      Nodes::BinaryTreeNode<ValueType>* GetMaximum() const {
        return this->FindMaximum(this->_root);
      }

    private:
      /**
       * @brief Removes a node from the tree and frees it.
       * @param node The node to remove.
       */
      void RemoveNode(Nodes::BinaryTreeNode<ValueType>* node) {
        if (!node->GetLeft()) {
          this->Transplant(node, node->GetRight());
        } else if (!node->GetRight()) {
          this->Transplant(node, node->GetLeft());
        } else {
          auto* successor = this->FindMinimum(node->GetRight());

          if (successor->GetParent() != node) {
            this->Transplant(successor, successor->GetRight());
            successor->SetRight(node->GetRight());
            successor->GetRight()->SetParent(successor);
          }

          this->Transplant(node, successor);
          successor->SetLeft(node->GetLeft());
          successor->GetLeft()->SetParent(successor);
        }

        delete node;
        this->_count--;
      }
  };

  /**
   * @brief Template class representing a red-black tree. A self-balancing
   *        binary search tree that guarantees O(log n) time for insertion,
   *        deletion, and search operations.
   * @tparam ValueType
   *   The type of values stored in the tree. Must support `operator<`.
   * @note
   *   Duplicate values are not inserted; inserting an existing value returns
   *   the existing node.
   */
  template <typename ValueType>
  class RedBlackTree : public Tree<ValueType, Nodes::RedBlackNode<ValueType>> {
    public:
      /**
       * @brief Constructs an empty red-black tree.
       */
      explicit RedBlackTree() = default;

      /**
       * @brief Inserts a value into the tree. If the value already exists,
       *        the existing node is returned without modification.
       * @param value The value to insert.
       * @return Pointer to the inserted or existing node.
       */
      Nodes::RedBlackNode<ValueType>* Insert(const ValueType& value) {
        auto* node = new Nodes::RedBlackNode<ValueType>(value);

        if (!this->_root) {
          node->SetColor(Nodes::RedBlackColor::Black);

          this->_root = node;
          this->_count++;

          return node;
        }

        auto* current = this->_root;
        Nodes::RedBlackNode<ValueType>* parent = nullptr;

        while (current) {
          parent = current;

          if (value < current->GetValue()) current = current->GetLeft();
          else if (current->GetValue() < value) current = current->GetRight();
          else {
            delete node;

            return current;
          }
        }

        node->SetParent(parent);

        if (value < parent->GetValue()) parent->SetLeft(node);
        else parent->SetRight(node);

        this->_count++;

        InsertFixup(node);

        return node;
      }

      /**
       * @brief Searches for a value in the tree.
       * @param value The value to search for.
       * @return Pointer to the node containing the value, or `nullptr` if
       *         not found.
       */
      Nodes::RedBlackNode<ValueType>* Search(const ValueType& value) const {
        auto* current = this->_root;

        while (current) {
          if (value < current->GetValue()) current = current->GetLeft();
          else if (current->GetValue() < value) current = current->GetRight();
          else return current;
        }

        return nullptr;
      }

      /**
       * @brief Checks whether the tree contains a value.
       * @param value The value to search for.
       * @return `true` if the value exists in the tree, false otherwise.
       */
      bool Contains(const ValueType& value) const {
        return Search(value) != nullptr;
      }

      /**
       * @brief Removes a value from the tree.
       * @param value The value to remove.
       * @return `true` if the value was found and removed, false otherwise.
       */
      bool Remove(const ValueType& value) {
        auto* node = Search(value);

        if (!node) return false;

        RemoveNode(node);

        return true;
      }

      /**
       * @brief Gets the node with the minimum value in the tree.
       * @return Pointer to the node with the minimum value, or `nullptr` if
       *         the tree is empty.
       */
      Nodes::RedBlackNode<ValueType>* GetMinimum() const {
        return this->FindMinimum(this->_root);
      }

      /**
       * @brief Gets the node with the maximum value in the tree.
       * @return Pointer to the node with the maximum value, or `nullptr` if
       *         the tree is empty.
       */
      Nodes::RedBlackNode<ValueType>* GetMaximum() const {
        return this->FindMaximum(this->_root);
      }

    private:
      /**
       * @brief Gets the color of a node, treating `nullptr` as black.
       * @param node The node to get the color of.
       * @return The color of the node, or `Nodes::RedBlackColor::Black` if the node
       *         is `nullptr`.
       */
      static Nodes::RedBlackColor GetNodeColor(Nodes::RedBlackNode<ValueType>* node) {
        return node ? node->GetColor() : Nodes::RedBlackColor::Black;
      }

      /**
       * @brief Performs a left rotation around the given node.
       * @param x The node to rotate around.
       */
      void RotateLeft(Nodes::RedBlackNode<ValueType>* x) {
        auto* y = x->GetRight();

        x->SetRight(y->GetLeft());

        if (y->GetLeft()) y->GetLeft()->SetParent(x);

        y->SetParent(x->GetParent());

        if (!x->GetParent()) this->_root = y;
        else if (x == x->GetParent()->GetLeft()) x->GetParent()->SetLeft(y);
        else x->GetParent()->SetRight(y);

        y->SetLeft(x);
        x->SetParent(y);
      }

      /**
       * @brief Performs a right rotation around the given node.
       * @param x The node to rotate around.
       */
      void RotateRight(Nodes::RedBlackNode<ValueType>* x) {
        auto* y = x->GetLeft();

        x->SetLeft(y->GetRight());

        if (y->GetRight()) y->GetRight()->SetParent(x);

        y->SetParent(x->GetParent());

        if (!x->GetParent()) this->_root = y;
        else if (x == x->GetParent()->GetRight()) x->GetParent()->SetRight(y);
        else x->GetParent()->SetLeft(y);

        y->SetRight(x);
        x->SetParent(y);
      }

      /**
       * @brief Restores red-black properties after an insertion.
       * @param z The newly inserted node.
       */
      void InsertFixup(Nodes::RedBlackNode<ValueType>* z) {
        while (
          z->GetParent() &&
          z->GetParent()->GetColor() == Nodes::RedBlackColor::Red
        ) {
          auto* parent = z->GetParent();
          auto* grandparent = parent->GetParent();

          if (!grandparent) break;

          if (parent == grandparent->GetLeft()) {
            auto* uncle = grandparent->GetRight();

            if (GetNodeColor(uncle) == Nodes::RedBlackColor::Red) {
              parent->SetColor(Nodes::RedBlackColor::Black);
              uncle->SetColor(Nodes::RedBlackColor::Black);
              grandparent->SetColor(Nodes::RedBlackColor::Red);

              z = grandparent;
            } else {
              if (z == parent->GetRight()) {
                z = parent;

                RotateLeft(z);

                parent = z->GetParent();
                grandparent = parent->GetParent();
              }

              parent->SetColor(Nodes::RedBlackColor::Black);
              grandparent->SetColor(Nodes::RedBlackColor::Red);
              RotateRight(grandparent);
            }
          } else {
            auto* uncle = grandparent->GetLeft();

            if (GetNodeColor(uncle) == Nodes::RedBlackColor::Red) {
              parent->SetColor(Nodes::RedBlackColor::Black);
              uncle->SetColor(Nodes::RedBlackColor::Black);
              grandparent->SetColor(Nodes::RedBlackColor::Red);

              z = grandparent;
            } else {
              if (z == parent->GetLeft()) {
                z = parent;

                RotateRight(z);

                parent = z->GetParent();
                grandparent = parent->GetParent();
              }

              parent->SetColor(Nodes::RedBlackColor::Black);
              grandparent->SetColor(Nodes::RedBlackColor::Red);
              RotateLeft(grandparent);
            }
          }
        }

        this->_root->SetColor(Nodes::RedBlackColor::Black);
      }

      /**
       * @brief Removes a node from the tree and frees it, then restores
       *        red-black properties if necessary.
       * @param z The node to remove.
       */
      void RemoveNode(Nodes::RedBlackNode<ValueType>* z) {
        auto* y = z;
        Nodes::RedBlackColor originalColor = y->GetColor();
        Nodes::RedBlackNode<ValueType>* x;
        Nodes::RedBlackNode<ValueType>* xParent;

        if (!z->GetLeft()) {
          x = z->GetRight();
          xParent = z->GetParent();

          this->Transplant(z, x);
        } else if (!z->GetRight()) {
          x = z->GetLeft();
          xParent = z->GetParent();

          this->Transplant(z, x);
        } else {
          y = this->FindMinimum(z->GetRight());
          originalColor = y->GetColor();
          x = y->GetRight();

          if (y->GetParent() == z) xParent = y;
          else {
            xParent = y->GetParent();

            this->Transplant(y, x);
            y->SetRight(z->GetRight());
            z->GetRight()->SetParent(y);
          }

          this->Transplant(z, y);
          y->SetLeft(z->GetLeft());
          z->GetLeft()->SetParent(y);
          y->SetColor(z->GetColor());
        }

        delete z;

        this->_count--;

        if (originalColor == Nodes::RedBlackColor::Black) RemoveFixup(x, xParent);
      }

      /**
       * @brief Restores red-black properties after a deletion.
       * @param x The node that replaced the removed node (may be `nullptr`).
       * @param xParent The parent of `x` (used when `x` is `nullptr`).
       */
      void RemoveFixup(
        Nodes::RedBlackNode<ValueType>* x,
        Nodes::RedBlackNode<ValueType>* xParent
      ) {
        while (
          x != this->_root &&
          GetNodeColor(x) == Nodes::RedBlackColor::Black
        ) {
          if (x == (xParent ? xParent->GetLeft() : nullptr)) {
            auto* w = xParent->GetRight();

            if (w->GetColor() == Nodes::RedBlackColor::Red) {
              w->SetColor(Nodes::RedBlackColor::Black);
              xParent->SetColor(Nodes::RedBlackColor::Red);
              RotateLeft(xParent);

              w = xParent->GetRight();
            }

            if (
              GetNodeColor(w->GetLeft()) == Nodes::RedBlackColor::Black &&
              GetNodeColor(w->GetRight()) == Nodes::RedBlackColor::Black
            ) {
              w->SetColor(Nodes::RedBlackColor::Red);

              x = xParent;
              xParent = x->GetParent();
            } else {
              if (GetNodeColor(w->GetRight()) == Nodes::RedBlackColor::Black) {
                w->GetLeft()->SetColor(Nodes::RedBlackColor::Black);
                w->SetColor(Nodes::RedBlackColor::Red);
                RotateRight(w);

                w = xParent->GetRight();
              }

              w->SetColor(xParent->GetColor());
              xParent->SetColor(Nodes::RedBlackColor::Black);
              w->GetRight()->SetColor(Nodes::RedBlackColor::Black);
              RotateLeft(xParent);

              x = this->_root;
            }
          } else {
            auto* w = xParent->GetLeft();

            if (w->GetColor() == Nodes::RedBlackColor::Red) {
              w->SetColor(Nodes::RedBlackColor::Black);
              xParent->SetColor(Nodes::RedBlackColor::Red);
              RotateRight(xParent);

              w = xParent->GetLeft();
            }

            if (
              GetNodeColor(w->GetRight()) == Nodes::RedBlackColor::Black &&
              GetNodeColor(w->GetLeft()) == Nodes::RedBlackColor::Black
            ) {
              w->SetColor(Nodes::RedBlackColor::Red);

              x = xParent;
              xParent = x->GetParent();
            } else {
              if (GetNodeColor(w->GetLeft()) == Nodes::RedBlackColor::Black) {
                w->GetRight()->SetColor(Nodes::RedBlackColor::Black);
                w->SetColor(Nodes::RedBlackColor::Red);
                RotateLeft(w);

                w = xParent->GetLeft();
              }

              w->SetColor(xParent->GetColor());
              xParent->SetColor(Nodes::RedBlackColor::Black);
              w->GetLeft()->SetColor(Nodes::RedBlackColor::Black);
              RotateRight(xParent);

              x = this->_root;
            }
          }
        }

        if (x) x->SetColor(Nodes::RedBlackColor::Black);
      }
  };

  /**
   * @brief Template class representing a named, path-addressable N-ary tree.
   *        Each node carries a name and a value. Nodes can be queried, added,
   *        or removed using slash-separated paths (e.g. `"Devices/VGA"`).
   * @tparam ValueType The type of values stored in the tree nodes.
   */
  template <typename ValueType>
  class DirectoryTree {
    public:
      /**
       * @brief Constructs an empty directory tree.
       */
      explicit DirectoryTree() = default;

      /**
       * @brief Destructor. Frees all nodes in the tree.
       */
      ~DirectoryTree() {
        auto* current = _root;

        while (current) {
          auto* next = current->GetNextSibling();

          _destroySubtree(current);

          current = next;
        }
      }

      /**
       * @brief Gets the first top-level node.
       * @return Pointer to the first top-level node, or `nullptr` if empty.
       */
      Nodes::DirectoryNode<ValueType>* GetRoot() const { return _root; }

      /**
       * @brief Gets the number of nodes in the tree.
       * @return The total node count.
       */
      Size GetCount() const { return _count; }

      /**
       * @brief Checks whether the tree is empty.
       * @return `true` if the tree has no nodes, false otherwise.
       */
      bool IsEmpty() const { return _root == nullptr; }

      /**
       * @brief Finds the node at the given slash-separated path. The returned
       *        node can be used as the root of its subtree.
       * @param path Slash-separated path (e.g. `"A/B/C"`).
       * @return Pointer to the node, or `nullptr` if not found.
       */
      Nodes::DirectoryNode<ValueType>* Find(const char* path) const {
        return _walk(path);
      }

      /**
       * @brief Gets a pointer to the value stored at the given path.
       * @param path Slash-separated path.
       * @return Pointer to the value, or `nullptr` if the path does not exist.
       */
      ValueType* FindValue(const char* path) const {
        auto* node = _walk(path);

        return node ? &node->GetValueRef() : nullptr;
      }

      /**
       * @brief Checks whether a node exists at the given path.
       * @param path Slash-separated path.
       * @return `true` if the path exists, false otherwise.
       */
      bool Exists(const char* path) const {
        return _walk(path) != nullptr;
      }

      /**
       * @brief Adds a new node at the given path. All intermediate components
       *        of the path must already exist; only the final component is
       *        created.
       * @param path Slash-separated path for the new node.
       * @param value The value to store in the new node.
       * @return Pointer to the newly created node, or `nullptr` if the parent
       *         path does not exist or a node with the same name already exists
       *         at that level.
       */
      Nodes::DirectoryNode<ValueType>* Add(
        const char* path,
        const ValueType& value
      ) {
        if (!path || *path == '\0') return nullptr;

        const char* p = path;
        const char* leafStart = nullptr;
        Nodes::DirectoryNode<ValueType>* parent = nullptr;
        Size leafLength = 0;

        while (*p) {
          // skip slashes
          while (*p == '/') ++p;

          if (!*p) break;

          // extract component bounds
          const char* componentStart = p;

          while (*p && *p != '/') ++p;

          Size componentLength = static_cast<Size>(p - componentStart);

          // peek ahead: is there another component after this one?
          const char* peek = p;

          while (*peek == '/') ++peek;

          if (*peek == '\0') {
            // this is the last component, the leaf name
            leafStart = componentStart;
            leafLength = componentLength;

            break;
          }

          // intermediate component, must exist
          auto* candidate = parent ? parent->GetFirstChild() : _root;
          Nodes::DirectoryNode<ValueType>* found = nullptr;

          while (candidate) {
            if (
              _nameEquals(
                candidate->GetName(),
                componentStart,
                componentLength
              )
            ) {
              found = candidate;

              break;
            }

            candidate = candidate->GetNextSibling();
          }

          if (!found) return nullptr;

          parent = found;
        }

        if (!leafStart || leafLength == 0) return nullptr;

        // check for duplicate at this level
        auto* existing = parent ? parent->GetFirstChild() : _root;

        while (existing) {
          if (_nameEquals(existing->GetName(), leafStart, leafLength))
            return nullptr;

          existing = existing->GetNextSibling();
        }

        // build a null-terminated name for the node
        char leafName[Nodes::DirectoryNodeNameMaxLength] = {};
        Size copyLength = leafLength < Nodes::DirectoryNodeNameMaxLength - 1
          ? leafLength
          : Nodes::DirectoryNodeNameMaxLength - 1;

        for (Size i = 0; i < copyLength; ++i) leafName[i] = leafStart[i];

        // create and link the new node
        auto* node = new Nodes::DirectoryNode<ValueType>(leafName, value);

        if (parent) {
          node->SetNextSibling(parent->GetFirstChild());
          node->SetParent(parent);
          parent->SetFirstChild(node);
        } else {
          node->SetNextSibling(_root);

          _root = node;
        }

        ++_count;

        return node;
      }

      /**
       * @brief Removes the node at the given path and all of its descendants.
       * @param path Slash-separated path of the node to remove.
       * @return `true` if the node was found and removed, false otherwise.
       */
      bool Remove(const char* path) {
        auto* node = _walk(path);

        if (!node) return false;

        _detach(node);
        _destroySubtree(node);

        return true;
      }

    private:
      /**
       * @brief Pointer to the first top-level node. Top-level nodes are linked
       *        via their next-sibling pointers.
       */
      Nodes::DirectoryNode<ValueType>* _root = nullptr;

      /**
       * @brief The total number of nodes in the tree.
       */
      Size _count = 0;

      /**
       * @brief Compares a null-terminated node name against a path component
       *        specified by a pointer and length.
       * @param name Null-terminated node name.
       * @param component Pointer to the start of the path component.
       * @param length Length of the path component.
       * @return `true` if the name exactly matches the component.
       */
      static bool _nameEquals(
        const char* name,
        const char* component,
        Size length
      ) {
        Size i = 0;

        for (; i < length; ++i)
          if (name[i] != component[i] || name[i] == '\0') return false;

        return name[i] == '\0';
      }

      /**
       * @brief Walks a slash-separated path from the root, returning the node
       *        at the end.
       * @param path Slash-separated path.
       * @return Pointer to the node, or `nullptr` if any component is not
       *         found.
       */
      Nodes::DirectoryNode<ValueType>* _walk(const char* path) const {
        if (!path || *path == '\0') return nullptr;

        const char* p = path;
        Nodes::DirectoryNode<ValueType>* current = nullptr;

        while (*p) {
          // skip slashes
          while (*p == '/') ++p;
          if (!*p) break;

          // extract component bounds
          const char* componentStart = p;

          while (*p && *p != '/') ++p;

          Size componentLength = static_cast<Size>(p - componentStart);

          // search the appropriate child list
          auto* candidate = current ? current->GetFirstChild() : _root;
          Nodes::DirectoryNode<ValueType>* found = nullptr;

          while (candidate) {
            if (
              _nameEquals(
                candidate->GetName(),
                componentStart,
                componentLength
              )
            ) {
              found = candidate;

              break;
            }

            candidate = candidate->GetNextSibling();
          }

          if (!found) return nullptr;

          current = found;
        }

        return current;
      }

      /**
       * @brief Detaches a node from its parent's child list or the top-level
       *        list. Does not destroy the node or its descendants.
       * @param node The node to detach.
       */
      void _detach(Nodes::DirectoryNode<ValueType>* node) {
        auto* parent = node->GetParent();

        if (parent) {
          if (parent->GetFirstChild() == node) {
            parent->SetFirstChild(node->GetNextSibling());
          } else {
            auto* prev = parent->GetFirstChild();

            while (prev && prev->GetNextSibling() != node)
              prev = prev->GetNextSibling();

            if (prev)
              prev->SetNextSibling(node->GetNextSibling());
          }
        } else {
          if (_root == node) {
            _root = node->GetNextSibling();
          } else {
            auto* prev = _root;

            while (prev && prev->GetNextSibling() != node)
              prev = prev->GetNextSibling();

            if (prev)
              prev->SetNextSibling(node->GetNextSibling());
          }
        }

        node->SetParent(nullptr);
        node->SetNextSibling(nullptr);
      }

      /**
       * @brief Recursively destroys a node and all of its descendants,
       *        decrementing `_count` for each node destroyed.
       * @param node The root of the subtree to destroy.
       */
      void _destroySubtree(Nodes::DirectoryNode<ValueType>* node) {
        if (!node) return;

        auto* child = node->GetFirstChild();

        while (child) {
          auto* next = child->GetNextSibling();

          _destroySubtree(child);

          child = next;
        }

        --_count;

        delete node;
      }
  };
}
