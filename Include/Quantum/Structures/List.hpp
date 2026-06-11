/**
 * @file Include/Quantum/Structures/List.hpp
 * @brief Declaration of different list structures.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "Node.hpp"

/**
 * @brief List structures.
 */
namespace Quantum::Structures::Lists {
  /**
   * @brief Template class representing a linked list structure. Provides basic
   *        functionality to manage a list of nodes containing values of type
   *        `ValueType`.
   * @tparam ValueType The type of values stored in the linked list nodes.
   */
  template <typename ValueType>
  class LinkedList {
    public:
      /**
       * @brief Constructs an empty linked list.
       */
      explicit LinkedList() = default;

      /**
       * @brief Constructs a linked list with the given head node.
       * @param head Pointer to the head node of the linked list.
       */
      explicit LinkedList(Nodes::LinkedNode<ValueType>* head) : _head(head) {}

      /**
       * @brief Destructor for the linked list. Frees all nodes in the list.
       */
      ~LinkedList() {
        auto* current = _head;

        while (current) {
          auto* next = current->GetNext();

          delete current;

          current = next;
        }
      }

      /**
       * @brief Gets the head node of the linked list.
       * @return Pointer to the head node of the linked list.
       */
      Nodes::LinkedNode<ValueType>* GetHead() const { return _head; }

      /**
       * @brief Sets the head node of the linked list.
       * @param head Pointer to the new head node of the linked list.
       */
      void SetHead(Nodes::LinkedNode<ValueType>* head) { _head = head; }

      /**
       * @brief Gets the tail node of the linked list.
       * @return Pointer to the tail node of the linked list.
       */
      Nodes::LinkedNode<ValueType>* GetTail() const {
        if (!_head) return nullptr;

        auto* current = _head;

        while (current && current->GetNext()) current = current->GetNext();

        return current;
      }

      /**
       * @brief Gets the number of nodes in the linked list.
       * @return The count of nodes in the linked list.
       */
      Size GetCount() const {
        Size count = 0;
        auto* current = _head;

        while (current) {
          ++count;
          current = current->GetNext();
        }

        return count;
      }

      /**
       * @brief Pops the head node from the linked list and returns it.
       *        The caller is responsible for deleting the returned node when
       *        done.
       * @return Pointer to the popped head node, or `nullptr` if the list is
       *         empty.
       */
      Nodes::LinkedNode<ValueType>* Pop() {
        if (!_head) return nullptr;

        Nodes::LinkedNode<ValueType>* node = _head;

        _head = _head->GetNext();

        node->SetNext(nullptr);

        return node;
      }

      /**
       * @brief Removes the specified node from the linked list and returns it.
       *        The caller is responsible for deleting the returned node when
       *        done.
       * @param node Pointer to the node to remove from the linked list.
       * @return Pointer to the removed node, or `nullptr` if the node was not
       *         found in the list.
       */
      Nodes::LinkedNode<ValueType>* Remove(Nodes::LinkedNode<ValueType>* node) {
        if (!node) return nullptr;

        if (node == _head) {
          _head = _head->GetNext();

          node->SetNext(nullptr);

          return node;
        }

        auto* current = _head;

        while (current && current->GetNext() != node)
          current = current->GetNext();

        if (!current) return nullptr;

        current->SetNext(node->GetNext());
        node->SetNext(nullptr);

        return node;
      }

      /**
       * @brief Appends a node to the end of the linked list.
       * @param node Pointer to the node to append to the linked list.
       */
      void Append(Nodes::LinkedNode<ValueType>* node) {
        if (!node) return;

        Nodes::LinkedNode<ValueType>* tail = GetTail();

        if (tail)
          tail->SetNext(node);
        else
          _head = node;
      }

    private:
      /**
       * @brief Pointer to the head node of the linked list.
       */
      Nodes::LinkedNode<ValueType>* _head = nullptr;
  };

  /**
   * @brief Doubly-linked list with O(1) head/tail access and tracked count.
   * @tparam ValueType The type of values stored in the list nodes.
   *
   * Uses `PathNode<ValueType>` as the underlying node type. The list owns
   * all nodes added to it; the destructor deletes every node. Methods that
   * remove nodes (`Pop`, `PopBack`, `Remove`) detach the node and return it
   * to the caller, who becomes responsible for its lifetime.
   */
  template <typename ValueType>
  class List {
    public:
      /**
       * @brief Constructs an empty list.
       */
      explicit List() = default;

      /**
       * @brief Destroys the list and deletes all nodes.
       */
      ~List() { Clear(); }

      /**
       * @brief Gets the head node.
       * @return Pointer to the head node, or `nullptr` if empty.
       */
      Nodes::PathNode<ValueType>* GetHead() const { return _head; }

      /**
       * @brief Gets the tail node.
       * @return Pointer to the tail node, or `nullptr` if empty.
       */
      Nodes::PathNode<ValueType>* GetTail() const { return _tail; }

      /**
       * @brief Gets the number of nodes in the list.
       * @return The node count.
       */
      Size GetCount() const { return _count; }

      /**
       * @brief Checks whether the list is empty.
       * @return `true` if the list contains no nodes.
       */
      bool IsEmpty() const { return _count == 0; }

      /**
       * @brief Inserts a node at the front of the list.
       * @param node Pointer to the node to prepend.
       */
      void Prepend(Nodes::PathNode<ValueType>* node) {
        if (!node) return;

        node->SetPrevious(nullptr);
        node->SetNext(_head);

        if (_head)
          _head->SetPrevious(node);
        else
          _tail = node;

        _head = node;
        ++_count;
      }

      /**
       * @brief Inserts a node at the end of the list.
       * @param node Pointer to the node to append.
       */
      void Append(Nodes::PathNode<ValueType>* node) {
        if (!node) return;

        node->SetNext(nullptr);
        node->SetPrevious(_tail);

        if (_tail)
          _tail->SetNext(node);
        else
          _head = node;

        _tail = node;
        ++_count;
      }

      /**
       * @brief Inserts a node after a target node already in the list.
       * @param target The existing node to insert after.
       * @param node The new node to insert.
       */
      void InsertAfter(
        Nodes::PathNode<ValueType>* target,
        Nodes::PathNode<ValueType>* node
      ) {
        if (!target || !node) return;

        node->SetPrevious(target);
        node->SetNext(target->GetNext());

        if (target->GetNext())
          target->GetNext()->SetPrevious(node);
        else
          _tail = node;

        target->SetNext(node);
        ++_count;
      }

      /**
       * @brief Inserts a node before a target node already in the list.
       * @param target The existing node to insert before.
       * @param node The new node to insert.
       */
      void InsertBefore(
        Nodes::PathNode<ValueType>* target,
        Nodes::PathNode<ValueType>* node
      ) {
        if (!target || !node) return;

        node->SetNext(target);
        node->SetPrevious(target->GetPrevious());

        if (target->GetPrevious())
          target->GetPrevious()->SetNext(node);
        else
          _head = node;

        target->SetPrevious(node);
        ++_count;
      }

      /**
       * @brief Removes a node from the list and returns it. The caller is
       *        responsible for deleting the returned node.
       * @param node Pointer to the node to remove.
       * @return The removed node with links zeroed, or `nullptr` if `node`
       *         is null.
       */
      Nodes::PathNode<ValueType>* Remove(Nodes::PathNode<ValueType>* node) {
        if (!node) return nullptr;

        if (node->GetPrevious())
          node->GetPrevious()->SetNext(node->GetNext());
        else
          _head = node->GetNext();

        if (node->GetNext())
          node->GetNext()->SetPrevious(node->GetPrevious());
        else
          _tail = node->GetPrevious();

        node->SetNext(nullptr);
        node->SetPrevious(nullptr);
        --_count;

        return node;
      }

      /**
       * @brief Removes and returns the head node. The caller is responsible
       *        for deleting the returned node.
       * @return Pointer to the former head node, or `nullptr` if empty.
       */
      Nodes::PathNode<ValueType>* Pop() {
        return Remove(_head);
      }

      /**
       * @brief Removes and returns the tail node. The caller is responsible
       *        for deleting the returned node.
       * @return Pointer to the former tail node, or `nullptr` if empty.
       */
      Nodes::PathNode<ValueType>* PopBack() {
        return Remove(_tail);
      }

      /**
       * @brief Finds the first node whose value equals the given value.
       * @param value The value to search for.
       * @return Pointer to the matching node, or `nullptr` if not found.
       */
      Nodes::PathNode<ValueType>* Find(const ValueType& value) const {
        auto* current = _head;

        while (current) {
          if (current->GetValue() == value) return current;

          current = current->GetNext();
        }

        return nullptr;
      }

      /**
       * @brief Checks whether the list contains a node with the given value.
       * @param value The value to search for.
       * @return `true` if a matching node exists.
       */
      bool Contains(const ValueType& value) const {
        return Find(value) != nullptr;
      }

      /**
       * @brief Deletes all nodes in the list.
       */
      void Clear() {
        auto* current = _head;

        while (current) {
          auto* next = current->GetNext();

          delete current;

          current = next;
        }

        _head = nullptr;
        _tail = nullptr;
        _count = 0;
      }

    private:
      /**
       * @brief Pointer to the head node.
       */
      Nodes::PathNode<ValueType>* _head = nullptr;

      /**
       * @brief Pointer to the tail node.
       */
      Nodes::PathNode<ValueType>* _tail = nullptr;

      /**
       * @brief Number of nodes in the list.
       */
      Size _count = 0;
  };

  /**
   * @brief Simple struct representing a pointer to a list of values, along with
   *        the count of values. This is not a full list structure and does not
   *        manage memory; it is simply a convenient way to pass around an array
   *        of values with its size.
   * @tparam ValueType The type of values in the list.
   */
  template <typename ValueType>
  class PointerList {
    public:
      /**
       * @brief Constructs a pointer list with the given base pointer and count.
       * @param base Pointer to an array of values of type `ValueType`. The
       *             caller is responsible for managing the memory of this
       *             array.
       * @param count The number of values in the array pointed to by `base`.
       */
      explicit PointerList(ValueType* base, Size count)
        : _base(base), _count(count) {}

      /**
       * @brief Constructs an empty pointer list with a null base pointer and
       *        zero count.
       */
      explicit PointerList() : _base(nullptr), _count(0) {}

      /**
       * @brief Gets the pointer to the array of values.
       * @return Pointer to the array of values.
       */
      virtual ~PointerList() = default;

      /**
       * @brief Gets the pointer to the array of values.
       * @return Pointer to the array of values.
       */
      Size GetCount() const { return _count; }

      /**
       * @brief Accesses the element at the given index without bounds checking.
       * @param index The index of the element to access.
       * @return Reference to the element at the given index.
       */
      ValueType& operator[](Size index) { return _base[index]; }

      /**
       * @brief Accesses the element at the given index without bounds checking.
       * @param index The index of the element to access.
       * @return Const reference to the element at the given index.
       */
      const ValueType& operator[](Size index) const { return _base[index]; }

      /**
       * @brief Accesses the element at the given index with bounds checking.
       * @param index The index of the element to access.
       * @return Pointer to the element, or `nullptr` if the index is out of
       *         bounds.
       */
      ValueType* At(Size index) {
        if (index >= _count) return nullptr;

        return &_base[index];
      }

      /**
       * @brief Accesses the element at the given index with bounds checking.
       * @param index The index of the element to access.
       * @return Const pointer to the element, or `nullptr` if the index is out
       *         of bounds.
       */
      const ValueType* At(Size index) const {
        if (index >= _count) return nullptr;

        return &_base[index];
      }

    private:
      /**
       * @brief Pointer to an array of values of type `ValueType`. The caller is
       *        responsible for managing the memory of this array.
       */
      ValueType* _base;

      /**
       * @brief The number of values in the array pointed to by `Pointer`.
       */
      Size _count;
  };
}
