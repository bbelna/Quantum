/**
 * @file Include/Quantum/Structures/Node.hpp
 * @brief Declaration of different node structures.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>
#include <Quantum/Core/Value.hpp>

/**
 * @brief Node structures.
 */
namespace Quantum::Structures::Nodes {
  /**
   * @brief Node in a doubly-linked structure.
   * @tparam ValueType
   *   The type of value stored in the node.
   */
  template <typename ValueType>
  struct PathNode : public HasMutableValue<ValueType> {
    /**
     * @brief Constructs a node with the given value.
     * @param value The value to store in the node.
     */
    explicit PathNode(const ValueType& value)
      : HasMutableValue<ValueType>(value) {}

    /**
     * @brief Constructs a node with the given value and next node.
     * @param value The value to store in the node.
     * @param next Pointer to the next node.
     */
    explicit PathNode(const ValueType& value, PathNode<ValueType>* next)
      : HasMutableValue<ValueType>(value), _next(next) {}

    /**
     * @brief Constructs a node with the given value, next node, and previous
     *        node.
     * @param value The value to store in the node.
     * @param next Pointer to the next node.
     * @param previous Pointer to the previous node.
     */
    explicit PathNode(
      const ValueType& value,
      PathNode<ValueType>* next,
      PathNode<ValueType>* previous
    ) : HasMutableValue<ValueType>(value), _next(next), _previous(previous) {}

    /**
     * @brief Gets the next node in the structure.
     * @return Pointer to the next node.
     */
    PathNode<ValueType>* GetNext() const { return this->_next; }

    /**
     * @brief Sets the next node in the structure.
     * @param next Pointer to the next node.
     */
    void SetNext(PathNode<ValueType>* next) { this->_next = next; }

    /**
     * @brief Gets the previous node in the structure.
     * @return Pointer to the previous node.
     */
    PathNode<ValueType>* GetPrevious() const { return this->_previous; }

    /**
     * @brief Sets the previous node in the structure.
     * @param previous Pointer to the previous node.
     */
    void SetPrevious(PathNode<ValueType>* previous) {
      this->_previous = previous;
    }

    private:
      /**
       * @brief Pointer to the next node in the structure.
       */
      PathNode<ValueType>* _next = nullptr;

      /**
       * @brief Pointer to the previous node in the structure.
       */
      PathNode<ValueType>* _previous = nullptr;
  };

  /**
   * @brief Node in a singly-linked structure.
   * @tparam ValueType
   *   The type of value stored in the node.
   */
  template <typename ValueType>
  struct LinkedNode : public PathNode<ValueType> {
    /**
     * @brief Constructs a linked node with the given value.
     * @param value The value to store in the node.
     */
    explicit LinkedNode(const ValueType& value)
      : PathNode<ValueType>(value) {}

    /**
     * @brief Constructs a linked node with the given value and next node.
     * @param value The value to store in the node.
     * @param next Pointer to the next node.
     */
    explicit LinkedNode(
      const ValueType& value,
      LinkedNode<ValueType>* next
    ) : PathNode<ValueType>(value, next) {}

    /**
     * @brief Gets the next node in the structure.
     * @return Pointer to the next node.
     */
    LinkedNode<ValueType>* GetNext() const {
      return static_cast<LinkedNode<ValueType>*>(PathNode<ValueType>::GetNext());
    }

    /**
     * @brief Sets the next node in the structure.
     * @param next Pointer to the next node.
     */
    using PathNode<ValueType>::SetNext;
  };

  /**
   * @brief Node in a binary tree structure.
   * @tparam ValueType
   *   The type of value stored in the node.
   */
  template <typename ValueType>
  struct BinaryTreeNode : public HasMutableValue<ValueType> {
    /**
     * @brief Constructs a tree node with the given value.
     * @param value The value to store in the node.
     */
    explicit BinaryTreeNode(const ValueType& value)
      : HasMutableValue<ValueType>(value) {}

    /**
     * @brief Gets the left child of the node.
     * @return Pointer to the left child node.
     */
    BinaryTreeNode<ValueType>* GetLeft() const { return _left; }

    /**
     * @brief Sets the left child of the node.
     * @param left Pointer to the left child node.
     */
    void SetLeft(BinaryTreeNode<ValueType>* left) { _left = left; }

    /**
     * @brief Gets the right child of the node.
     * @return Pointer to the right child node.
     */
    BinaryTreeNode<ValueType>* GetRight() const { return _right; }

    /**
     * @brief Sets the right child of the node.
     * @param right Pointer to the right child node.
     */
    void SetRight(BinaryTreeNode<ValueType>* right) { _right = right; }

    /**
     * @brief Gets the parent of the node.
     * @return Pointer to the parent node.
     */
    BinaryTreeNode<ValueType>* GetParent() const { return _parent; }

    /**
     * @brief Sets the parent of the node.
     * @param parent Pointer to the parent node.
     */
    void SetParent(BinaryTreeNode<ValueType>* parent) { _parent = parent; }

    private:
      /**
       * @brief Pointer to the left child node.
       */
      BinaryTreeNode<ValueType>* _left = nullptr;

      /**
       * @brief Pointer to the right child node.
       */
      BinaryTreeNode<ValueType>* _right = nullptr;

      /**
       * @brief Pointer to the parent node.
       */
      BinaryTreeNode<ValueType>* _parent = nullptr;
  };

  /**
   * @brief Color of a node in a red-black tree.
   */
  enum class RedBlackColor {
    /**
     * @brief Red color for a red-black tree node.
     */
    Red,

    /**
     * @brief Black color for a red-black tree node.
     */
    Black
  };

  /**
   * @brief Node in a red-black tree structure.
   * @tparam ValueType The type of value stored in the node.
   * 
   * Extends `BinaryTreeNode` with a color property for maintaining red-black tree
   * invariants.
   */
  template <typename ValueType>
  struct RedBlackNode : public BinaryTreeNode<ValueType> {
    /**
     * @brief Constructs a red-black node with the given value. The node is
     *        colored red by default.
     * @param value The value to store in the node.
     */
    explicit RedBlackNode(const ValueType& value)
      : BinaryTreeNode<ValueType>(value) {}

    /**
     * @brief Constructs a red-black node with the given value and color.
     * @param value The value to store in the node.
     * @param color The color of the node.
     */
    explicit RedBlackNode(const ValueType& value, RedBlackColor color)
      : BinaryTreeNode<ValueType>(value), _color(color) {}

    /**
     * @brief Gets the color of the node.
     * @return The color of the node.
     */
    RedBlackColor GetColor() const { return _color; }

    /**
     * @brief Sets the color of the node.
     * @param color The new color of the node.
     */
    void SetColor(RedBlackColor color) { _color = color; }

    /**
     * @brief Gets the left child of the node.
     * @return Pointer to the left child node.
     */
    RedBlackNode<ValueType>* GetLeft() const {
      return static_cast<RedBlackNode<ValueType>*>(
        BinaryTreeNode<ValueType>::GetLeft()
      );
    }

    /**
     * @brief Gets the right child of the node.
     * @return Pointer to the right child node.
     */
    RedBlackNode<ValueType>* GetRight() const {
      return static_cast<RedBlackNode<ValueType>*>(
        BinaryTreeNode<ValueType>::GetRight()
      );
    }

    /**
     * @brief Gets the parent of the node.
     * @return Pointer to the parent node.
     */
    RedBlackNode<ValueType>* GetParent() const {
      return static_cast<RedBlackNode<ValueType>*>(
        BinaryTreeNode<ValueType>::GetParent()
      );
    }

    /**
     * @brief Sets the left child of the node.
     * @param left Pointer to the left child node.
     */
    using BinaryTreeNode<ValueType>::SetLeft;

    /**
     * @brief Sets the right child of the node.
     * @param right Pointer to the right child node.
     */
    using BinaryTreeNode<ValueType>::SetRight;

    /**
     * @brief Sets the parent of the node.
     * @param parent Pointer to the parent node.
     */
    using BinaryTreeNode<ValueType>::SetParent;

    private:
      /**
       * @brief The color of the node.
       */
      RedBlackColor _color = RedBlackColor::Red;
  };

  /**
   * @brief Node in an N-ary tree structure using the first-child/next-sibling
   *        representation.
   * @tparam ValueType The type of value stored in the node.
   *
   * Each node has a parent pointer, a pointer to its first child, and a
   * pointer to its next sibling. This allows an arbitrary number of children
   * per node without dynamic arrays.
   */
  template <typename ValueType>
  struct TreeNode : public HasMutableValue<ValueType> {
    /**
     * @brief Constructs a tree node with the given value.
     * @param value The value to store in the node.
     */
    explicit TreeNode(const ValueType& value)
      : HasMutableValue<ValueType>(value) {}

    /**
     * @brief Gets the parent node.
     * @return Pointer to the parent node, or `nullptr` if this is a root.
     */
    TreeNode<ValueType>* GetParent() const { return _parent; }

    /**
     * @brief Sets the parent node.
     * @param parent Pointer to the parent node.
     */
    void SetParent(TreeNode<ValueType>* parent) { _parent = parent; }

    /**
     * @brief Gets the first child node.
     * @return Pointer to the first child, or `nullptr` if this node has no
     *         children.
     */
    TreeNode<ValueType>* GetFirstChild() const { return _firstChild; }

    /**
     * @brief Sets the first child node.
     * @param child Pointer to the first child node.
     */
    void SetFirstChild(TreeNode<ValueType>* child) { _firstChild = child; }

    /**
     * @brief Gets the next sibling node.
     * @return Pointer to the next sibling, or `nullptr` if this is the last
     *         sibling.
     */
    TreeNode<ValueType>* GetNextSibling() const { return _nextSibling; }

    /**
     * @brief Sets the next sibling node.
     * @param sibling Pointer to the next sibling node.
     */
    void SetNextSibling(TreeNode<ValueType>* sibling) {
      _nextSibling = sibling;
    }

    /**
     * @brief Gets a reference to the stored value.
     * @return Reference to the value.
     */
    ValueType& GetValueRef() { return this->_value; }

    /**
     * @brief Gets a const reference to the stored value.
     * @return Const reference to the value.
     */
    const ValueType& GetValueRef() const { return this->_value; }

    private:
      /**
       * @brief Pointer to the parent node.
       */
      TreeNode<ValueType>* _parent = nullptr;

      /**
       * @brief Pointer to the first child node.
       */
      TreeNode<ValueType>* _firstChild = nullptr;

      /**
       * @brief Pointer to the next sibling node.
       */
      TreeNode<ValueType>* _nextSibling = nullptr;
  };

  /**
   * @brief Maximum length for directory node names, including the null
   *        terminator.
   */
  inline constexpr Size DirectoryNodeNameMaxLength = 64;

  /**
   * @brief Node in an N-ary tree structure with an associated name. Extends
   *        `TreeNode` with a fixed-size name for use in directory-like trees.
   * @tparam ValueType The type of value stored in the node.
   */
  template <typename ValueType>
  struct DirectoryNode : public TreeNode<ValueType> {
    /**
     * @brief Constructs a directory node with the given name and value.
     * @param name The name for this node (will be truncated to
     *        `DirectoryNodeNameMaxLength - 1` characters).
     * @param value The value to store in the node.
     */
    explicit DirectoryNode(
      const char* name,
      const ValueType& value
    ) : TreeNode<ValueType>(value) {
      Size i = 0;

      if (name)
        for (; name[i] && i < DirectoryNodeNameMaxLength - 1; ++i)
          _name[i] = name[i];

      _name[i] = '\0';
    }

    /**
     * @brief Gets the name of this node.
     * @return Pointer to the null-terminated name string.
     */
    const char* GetName() const { return _name; }

    /**
     * @brief Gets the parent node.
     * @return Pointer to the parent node.
     */
    DirectoryNode<ValueType>* GetParent() const {
      return static_cast<DirectoryNode<ValueType>*>(
        TreeNode<ValueType>::GetParent()
      );
    }

    /**
     * @brief Gets the first child node.
     * @return Pointer to the first child node.
     */
    DirectoryNode<ValueType>* GetFirstChild() const {
      return static_cast<DirectoryNode<ValueType>*>(
        TreeNode<ValueType>::GetFirstChild()
      );
    }

    /**
     * @brief Gets the next sibling node.
     * @return Pointer to the next sibling node.
     */
    DirectoryNode<ValueType>* GetNextSibling() const {
      return static_cast<DirectoryNode<ValueType>*>(
        TreeNode<ValueType>::GetNextSibling()
      );
    }

    /**
     * @brief Sets the parent node.
     * @param parent Pointer to the parent node.
     */
    using TreeNode<ValueType>::SetParent;

    /**
     * @brief Sets the first child node.
     * @param child Pointer to the first child node.
     */
    using TreeNode<ValueType>::SetFirstChild;

    /**
     * @brief Sets the next sibling node.
     * @param sibling Pointer to the next sibling node.
     */
    using TreeNode<ValueType>::SetNextSibling;

    private:
      /**
       * @brief The name of this node (null-terminated).
       */
      char _name[DirectoryNodeNameMaxLength] = {};
  };
}
