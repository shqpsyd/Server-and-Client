#pragma once
#include <mutex>
#include<iostream>

/// TODO: complete this implementation of a thread-safe (concurrent) sorted
/// linked list of integers
class clist
{
	/// a Node consists of a value and a pointer to another node
	struct Node
	{
		char* filename;
		char* file;
		Node* next;
		Node(char* v): value(v),next(nullptr){}
	};

	/// The head of the list is referenced by this pointer
	Node* sentinel;
	Node* head;
	//mutex *mtx = new (mutex);
	mutable std::mutex list_lock;
	int lru_size;

public:
	int k = 0;
	clist(int size)
	: sentinel(new Node(0)),lru_size(size)
	{}	
	
		
	/// insert *key* into the linked list if it doesn't already exist; return
	/// true if the key was added successfully.
	bool insert(char* file)
	{		
		std::lock_guard<std::mutex> lck(list_lock);		
		if(this->getSize() == lru_size){
			return false;
		}
		Node * new_node = new Node(file);
		Node* p = sentinel;
		while(p->next && p->next->value < key){
			p = p->next;
		}
		if(p->next && p->next->value == key){
			return false;
		}
		new_node->next = p->next;
		p->next = new_node;
		return true;
	}
	/// remove *key* from the list if it was present; return true if the key
	/// was removed successfully.
	bool remove(int key)
	{
		std::lock_guard<std::mutex> lck (list_lock);
		Node* p = sentinel;
		while(p->next && p->next->value < key){
			p = p->next;
		}
		if(p->next && p->next->value == key){
			Node *temp = p->next;
			p->next = p->next->next;
			delete temp;
			return true;
		}
		return false;
	}
	/// return true if *key* is present in the list, false otherwise
	bool lookup(int key) const
	{
		std::lock_guard<std::mutex> lck (list_lock);
		Node* p = sentinel;		
		while(p->next && p->next->value < key){
			p = p->next;					
		}
		return p->next && p->next->value == key;
	}


	//The following are not tested by the given tester but are required for grading
	//No locks are required for these.
	size_t getSize() const
	{
		//cout<<"head now"<<head;
		size_t size = 0;
		Node * p = sentinel;
			while(p->next){
				size++;
				p = p->next;
			}
			return size;
			
		
	}
	int getElement(size_t idx) const
	{		
		if(idx >= this->getSize()){
			return -1;
		}
		Node*p = sentinel->next;
		while(idx-- > 0){
			p = p->next;
		}
		return p->value;
	}


	//These functions just need to exist, they do not need to do anything
	size_t getBucketSize(size_t bucket) const
	{
		return 0;
	}
	int getElement(size_t bucket, size_t idx) const
	{
		return 0;
	}
};
