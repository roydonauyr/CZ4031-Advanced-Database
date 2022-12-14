#ifndef BPLUSTREE_HEADER
#define BPLUSTREE_HEADER

#include <algorithm>
#include <array>
#include <iterator>
#include <vector>
#include <utility>
#include <iostream>

#include "Database_Storage.hpp"

class BPlusTree
{
public:
	BlockManager * blkManager;
	
	unsigned int rootNode = 0; //Stores index of root node

	BPlusTree(BlockManager *mgr){
		this->blkManager = mgr;
	}

	// Print out a node block for BPlusTree
	void printTreeNode(unsigned int nodeIndex){
		treeNodeBlock* curNode = (treeNodeBlock*) blkManager->accessBlock(nodeIndex);
		printf("\n------------------------------------------\n");
		std::cout<<"Block id is: "<<nodeIndex<<std::endl;
		std::cout<<"Parent id is: "<<curNode->getParentBlock()<<std::endl;
		std::cout<<"Type is: "<<(unsigned int)curNode->type<<std::endl;
		std::cout<<"Keys: "<<std::endl;
		for (unsigned int key: curNode->key){
			std::cout<<key<<" ";
		}
		// printf('\n');
		std::cout<<"\nPtrs: "<<std::endl;
		for (Pointer ptr: curNode->ptrs){
			std::cout<<ptr.getBlock()<<" ";
		}
		std::cout<<std::endl;
	}

	//Print linkedList Block
	void PrintLinkedListBlock(unsigned int LinkedListIndex){
		printf("\n------------------------------------------\n");
		linkedListNodeBlock* curNode = (linkedListNodeBlock*) blkManager->accessBlock(LinkedListIndex);
		std::cout<<"Block id is: "<<(unsigned int)LinkedListIndex<<std::endl;
		std::cout<<"Type is: "<<(unsigned int)curNode->type<<std::endl;
		printf("Record addresses: \n");
		for (Pointer ptr: curNode->pointers){
			std::cout<<ptr.getBlock()<<" ";
		}
		std::cout<<"\n"<<"Next linkedList Block Pointer: "<<curNode->nextBlock.getBlock()<<std::endl;
	}

	//Search function
	std::vector<Record> searchKeys(unsigned int key){
		std::vector<Record> results;
		unsigned int curIndex = rootNode;
		treeNodeBlock* curBlock = (treeNodeBlock*) blkManager->accessBlock(rootNode);

		// iterating through internal nodes to get to leaf nodes
		while (curBlock -> type != 2) {  // while it is not a leaf node
			unsigned int i = 0;
			while (i<curBlock->getLength() && key>curBlock->key[i]) {
				i++;
			}
			curIndex = curBlock->ptrs[i].getBlock();
			curBlock = (treeNodeBlock*) blkManager->accessBlock(curIndex);
		}
		
		//Now curIndex and curBlock should be leaf node
		unsigned int j = 0;

		// search through the leaf node for the key 
		while (j < curBlock->getLength() && key != curBlock->key[j]) {
			j++;
		}

		// means that we have iterated through the whole leaf node already == key does not exist
		if (j == curBlock->getLength()) {
			std::cout<<"Key does not exist"<<std::endl;
			return results;
		}

		// accessing Record Blocks
		unsigned int curBlockIndex = curBlock->ptrs[j].getBlock();

		// directly accessing record blocks, no duplicates
		if (blkManager->accessBlock(curBlockIndex) ->type == 0) {
			RecordBlock* curRecordBlock = (RecordBlock*)blkManager->accessBlock(curBlockIndex); 
			unsigned int accessIndex = curBlock->ptrs[j].entry;
			results.push_back(curRecordBlock->records[accessIndex]);
		}

		// accessing linked list for duplicates
		else if (blkManager -> accessBlock(curBlockIndex) ->type == 3){
			linkedListNodeBlock* linkedList = (linkedListNodeBlock*) blkManager -> accessBlock(curBlockIndex);

			// for first linkedlist block
			for(Pointer record_pointer: linkedList->pointers){
				if(record_pointer.getBlock() == 0){
					break;
				}
				// exact index of the record in the record block
				unsigned int accessIndex = record_pointer.entry;
				// index of the record block
				unsigned int blockIndex = record_pointer.getBlock();
				RecordBlock* curRecordBlock = (RecordBlock*)blkManager->accessBlock(blockIndex);
				results.push_back(curRecordBlock->records[accessIndex]);
			}
			
			// if there are more than one linkedlist block
			while(linkedList->nextBlock.getBlock() != 0){
				//Switch to next linkedlist block
				unsigned int nextLinkedListIndex = linkedList->nextBlock.getBlock();
				linkedList = (linkedListNodeBlock*) blkManager -> accessBlock(nextLinkedListIndex);
				for(Pointer record_pointer: linkedList->pointers){
					if(record_pointer.getBlock() == 0){
						break;
					}
					unsigned int accessIndex = record_pointer.entry;
					unsigned int blockIndex = record_pointer.getBlock();
					RecordBlock* curRecordBlock = (RecordBlock*)blkManager->accessBlock(blockIndex);
					results.push_back(curRecordBlock->records[accessIndex]);
				}
			}
		}
		return results;
	}

	//Search range function
	std::vector<Record> searchRangeOfKeys(int LowerBound, int UpperBound) {
		std::vector<Record> results;
		unsigned int curIndex = rootNode;
		treeNodeBlock* curBlock = (treeNodeBlock*) blkManager->accessBlock(rootNode);

		while (curBlock -> type != 2) {  // while it is not a leaf node
			unsigned int i = 0;

			// search until the point where the lower bound of the range is found, if not keep iterating
			while (i < curBlock->getLength() && curBlock->key[i] < LowerBound) {  
				i++;
			}
			curIndex = curBlock->ptrs[i].getBlock();
			curBlock = (treeNodeBlock*) blkManager->accessBlock(curIndex);
		}
		

		// Now curIndex and curBlock should be leaf node
		// iterate through leaf node
		unsigned int j = 0;
		if(curBlock->key[0]>UpperBound){
			std::cout<<"Keys in range do not exist"<<std::endl;
			return results;
		}
		while (curBlock->key[0] <= UpperBound) {
			j = 0;
			while(curBlock->key[j]< LowerBound){
				j++;
			}

			while(LowerBound<curBlock->key[j]){
				LowerBound+= 1;
			}
			//20000 20001 30001//30003 30005 //30006 40001
			while (LowerBound <= curBlock->key[curBlock->getLength()-1] && LowerBound <= UpperBound) {
				while(j < curBlock->getLength()){
					if(LowerBound == curBlock->key[j]){
						unsigned int curBlockIndex = curBlock->ptrs[j].getBlock();
						// directly accessing record blocks, no duplicates
						if (blkManager->accessBlock(curBlockIndex) ->type == 0) {
							RecordBlock* curRecordBlock = (RecordBlock*)blkManager->accessBlock(curBlockIndex); 
							unsigned int accessIndex = curBlock->ptrs[j].entry;
							results.push_back(curRecordBlock->records[accessIndex]);
						}

						// accessing linked list for duplicates
						else if (blkManager -> accessBlock(curBlockIndex) ->type == 3){
							linkedListNodeBlock* linkedList = (linkedListNodeBlock*) blkManager -> accessBlock(curBlockIndex);

							// for first linkedlist block
							for(Pointer record_pointer: linkedList->pointers){
								if(record_pointer.getBlock() == 0){
									break;
								}
								// exact index of the record in the record block
								unsigned int accessIndex = record_pointer.entry;
								// index of the record block
								unsigned int blockIndex = record_pointer.getBlock();
								RecordBlock* curRecordBlock = (RecordBlock*)blkManager->accessBlock(blockIndex);
								results.push_back(curRecordBlock->records[accessIndex]);
							}
							
							// if there are more than one linkedlist block
							while(linkedList->nextBlock.getBlock() != 0){
								//Switch to next linkedlist block
								unsigned int nextLinkedListIndex = linkedList->nextBlock.getBlock();
								linkedList = (linkedListNodeBlock*) blkManager -> accessBlock(nextLinkedListIndex);
								for(Pointer record_pointer: linkedList->pointers){
									if(record_pointer.getBlock() == 0){
										break;
									}
									unsigned int accessIndex = record_pointer.entry;
									unsigned int blockIndex = record_pointer.getBlock();
									RecordBlock* curRecordBlock = (RecordBlock*)blkManager->accessBlock(blockIndex);
									results.push_back(curRecordBlock->records[accessIndex]);
								}
							}
						}
						j+=1;
					}
					LowerBound+=1;
					break;
				}
			};
			j = 0;
			curIndex = curBlock->ptrs[(curBlock->ptrs.size())-1].getBlock();
			curBlock = (treeNodeBlock*) blkManager->accessBlock(curIndex);

		}
		return results;
	}

	void deleteKey(unsigned int key) {
			unsigned int curIndex = rootNode;
			treeNodeBlock* curBlock = (treeNodeBlock*) blkManager->accessBlock(rootNode);

			unsigned leftIndex;
			unsigned rightIndex;
			// iterating through internal nodes to get to leaf nodes
			while (curBlock -> type != 2) {  // while it is not a leaf node
				unsigned int i = 0;
				//A 1 C 2 D
				while (i<curBlock->getLength() && key>=curBlock->key[i]) {
					i++;
				}
				curIndex = curBlock->ptrs[i].getBlock();
				curBlock = (treeNodeBlock*) blkManager->accessBlock(curIndex);
				if(i != 0){
					leftIndex = curBlock->ptrs[i-1].getBlock();
				}
			}

			treeNodeBlock* cursor = curBlock;

			unsigned int parentIndex = cursor->getParentBlock();
			treeNodeBlock* parent = (treeNodeBlock*)(blkManager->accessBlock(parentIndex));		// Parent of currNode

			bool isDeleted = false;
			int index = 0;

			// Try to find key in currNode
			for (int i = 0; i < cursor->getLength(); i++) {
				if (cursor->key[i] == key) {
					isDeleted = true;
					index = i;
					std::cout << "Key found, deleting key..." << std::endl;
					break;
				}
			}

			// Key not in B+ tree
			if (!isDeleted) {
				std::cout << "Key not found." << std::endl;
				return;
			}

			// Delete linked list
			if (blkManager->accessBlock(cursor->ptrs[index].getBlock())->type == 3) {
				linkedListNodeBlock *linkedList = (linkedListNodeBlock*) blkManager->accessBlock(cursor->ptrs[index].getBlock());
				for(Pointer record_pointer: linkedList->pointers){
					if(record_pointer.getBlock() == 0){
						break;
					}
					// exact index of the record in the record block
					unsigned int accessIndex = record_pointer.entry;
					// index of the record block
					unsigned int blockIndex = record_pointer.getBlock();
					RecordBlock* curRecordBlock = (RecordBlock*)blkManager->accessBlock(blockIndex);
					//Soft delete record, num votes will be zero
					curRecordBlock->records[accessIndex].numVotes = 0;
					curRecordBlock->records[accessIndex].avgRating.ones = 0;
					curRecordBlock->records[accessIndex].avgRating.decimal = 0;
					curRecordBlock->records[accessIndex].setTconst("");
				}
				unsigned int linkedListIndex = cursor->ptrs[index].getBlock();
				// if there are more than one linkedlist block
				while(linkedList->nextBlock.getBlock() != 0){
					//Switch to next linkedlist block
					unsigned int nextLinkedListIndex = linkedList->nextBlock.getBlock();
					blkManager->deleteBlock(linkedListIndex);
					linkedListIndex = nextLinkedListIndex;
					linkedList = (linkedListNodeBlock*) blkManager -> accessBlock(nextLinkedListIndex);
					for(Pointer record_pointer: linkedList->pointers){
						if(record_pointer.getBlock() == 0){
							break;
						}
						unsigned int accessIndex = record_pointer.entry;
						unsigned int blockIndex = record_pointer.getBlock();
						RecordBlock* curRecordBlock = (RecordBlock*)blkManager->accessBlock(blockIndex);
						curRecordBlock->records[accessIndex].numVotes = 0;
						curRecordBlock->records[accessIndex].avgRating.ones = 0;
						curRecordBlock->records[accessIndex].avgRating.decimal = 0;
						curRecordBlock->records[accessIndex].setTconst("");
					}
				}
				// Last linkedList block to delete
				blkManager->deleteBlock(linkedListIndex);
			}
			// Delete record
			else if ((blkManager->accessBlock(cursor->ptrs[index].getBlock()))->type == 0) {
				unsigned int accessIndex = cursor->ptrs[index].entry;
				unsigned int blockIndex = cursor->ptrs[index].getBlock();
				RecordBlock *curRecordBlock = (RecordBlock*)(blkManager->accessBlock(blockIndex));
				curRecordBlock->records[accessIndex].numVotes = 0;
				curRecordBlock->records[accessIndex].avgRating.ones = 0;
				curRecordBlock->records[accessIndex].avgRating.decimal = 0;
				curRecordBlock->records[accessIndex].setTconst("");
			}

			// Delete key
			cursor->key[index] = 0;
			cursor->ptrs[index].entry = -1;

			// LB changed hence must update parent
			if(index == 0){
				//update
				updateParent(cursor->getParentBlock());
			}

			// Move subsequent keys and pointers 1 position forward
			for (int i = index; i < cursor->getLength(); i++) {
				// currNode->key[i] = currNode->key[i + 1];
				cursor->ptrs[i] = cursor->ptrs[i + 1]; // Not sure how this will go but should be correct
				cursor->key[i] = cursor->key[i+1];
			}

			
			//Check if current leaf less than minimum key
			unsigned int minimum = (NUM_KEY_INDEX+1)/2;

			//Means that im the root (Tree is now empty)
			if(parentIndex == 0 && cursor->getLength() < minimum){
				blkManager->deleteBlock(rootNode);
				rootNode = 0;
				std::cout << "No keys left in the tree. Killing tree..." << std::endl;
				return;
			}

			if(cursor->getLength() < minimum){
				mergeLeafNodes(leftIndex, curIndex);
			}

	}

	void updateParent(unsigned int currNodeIndex){
		treeNodeBlock *currNode = (treeNodeBlock*) blkManager->accessBlock(currNodeIndex);
		int i = 1;
		while(currNode->ptrs[i].getBlock() != 0 ){
			treeNodeBlock* temp = (treeNodeBlock*)blkManager->accessBlock(currNode->ptrs[i].getBlock());
			currNode->key[i-1] = lowestBound(temp);
			i++;
		}
		if(currNodeIndex != rootNode){
			updateParent(currNode->getParentBlock());
		}
	}

	void mergeLeafNodes(unsigned int leftIndex, unsigned int currNodeIndex){
			unsigned int minimum = (NUM_KEY_INDEX+1)/2;
			treeNodeBlock *leftNode;
			treeNodeBlock *rightNode;
			unsigned int rightNodeIndex;
			treeNodeBlock *currNode = (treeNodeBlock*) blkManager->accessBlock(currNodeIndex);

			// Check if left sibling exists
			if (leftIndex != 0) {
				leftNode = (treeNodeBlock*)blkManager->accessBlock(leftIndex);
			}

			// Check if right sibling exists
			if (currNode->ptrs[currNode->ptrs.size()-1].getBlock() != 0) {
				rightNodeIndex = currNode->ptrs[currNode->ptrs.size()-1].getBlock();
				rightNode = (treeNodeBlock*)blkManager->accessBlock(rightNodeIndex);
			}

			// Left sibling can make a transfer
			if (leftIndex != 0 && leftNode->getLength() > minimum) {
				//1 2 3
				//A B C next leaf node
				// Make space for the transfer
				for (int i = currNode->getLength()-1; i > 0; i--) {
					currNode->key[i] = currNode->key[i - 1];
					currNode->ptrs[i] = currNode->ptrs[i-1];
				}

				

				// Transfer key from leftNode to currNode
				currNode->key[0] = leftNode->key[leftNode->getLength() - 1];
				currNode->ptrs[0] = leftNode->ptrs[leftNode->getLength() - 1];


				// Update left leaf
				leftNode->key[leftNode->getLength() - 1] = 0;
				leftNode->ptrs[leftNode->getLength() - 1].setBlock(0);
				leftNode->ptrs[leftNode->getLength() - 1].entry = -1;

				//update parent
				updateParent(currNode->getParentBlock());
				return;
			}
			// Right sibling can make a transfer
			else if (rightNodeIndex != 0 && rightNode->getLength() >minimum) {			
				// Transfer key from rightNode to currNode
				currNode->key[currNode->getLength() - 1] = rightNode->key[0];
				currNode->ptrs[currNode->getLength() - 1] = rightNode->ptrs[0];

				// Remove rightNode's first key and update subsequent pointers and keys
				for (int i = 0; i < rightNode->getLength(); i++) {
					rightNode->key[i] = rightNode->key[i + 1];
					rightNode->ptrs[i] = rightNode->ptrs[i + 1];
				}

				//update parent
				updateParent(rightNode->getParentBlock());
				return;
			}

			// Merge leftNode and currNode
			else if (leftIndex != 0 && leftNode->getLength() <= minimum)
			{
				// Transfer all keys and pointers from currNode to leftNode
				for (int i = leftNode->getLength()+1, j = 0; j < currNode->getLength(); i++, j++)
				{
					leftNode->key[i] = currNode->key[j];
					leftNode->ptrs[i] = currNode->ptrs[j];
				}
				leftNode->ptrs[leftNode->ptrs.size()-1] = currNode->ptrs[leftNode->ptrs.size()-1];

				
				// Delete currNode from parent
				unsigned int parentBlockIndex = currNode->getParentBlock();
				blkManager->deleteBlock(currNodeIndex);

				//recursion for parent
				deleteKeyInternal(currNodeIndex, parentBlockIndex);
				
			}
			// Merge with rightNode
			else if (rightNodeIndex != 0 && rightNode->getLength() <= minimum) {
				// Transfer all keys and pointers from rightNode to currNode
				for (int i = currNode->getLength()+1, j = 0; j < rightNode->getLength(); i++, j++)
				{
					currNode->key[i] = rightNode->key[j];
					currNode->ptrs[i] = rightNode->ptrs[j];
				}
				currNode->ptrs[currNode->ptrs.size()-1] = rightNode->ptrs[rightNode->ptrs.size()-1];

				// Delete rightNode from parent
				unsigned int parentBlockIndex = rightNode->getParentBlock();
				blkManager->deleteBlock(rightNodeIndex);

				//recursion for parent
				deleteKeyInternal(rightNodeIndex, parentBlockIndex);
			}
	}

	void mergeParentNodes(unsigned int leftIndex, unsigned int rightIndex, unsigned int currNodeIndex){
			unsigned int minimum = (NUM_KEY_INDEX+1)/2;
			treeNodeBlock *leftNode;
			treeNodeBlock *rightNode;
			treeNodeBlock *currNode = (treeNodeBlock*) blkManager->accessBlock(currNodeIndex);

			// Check if left sibling exists
			if (leftIndex != 0) {
				leftNode = (treeNodeBlock*)blkManager->accessBlock(leftIndex);
			}

			// Check if right sibling exists
			if (rightIndex != 0) {
				rightNode = (treeNodeBlock*)blkManager->accessBlock(rightIndex);
			}

			// Left sibling can make a transfer
			if (leftIndex != 0 && leftNode->getLength() > minimum) {
			
				// Make space for the transfer
				for (int i = currNode->getLength(); i > 0; i--) {
					currNode->ptrs[i] = currNode->ptrs[i-1];
				}


				// Transfer key from leftNode to currNode
				currNode->ptrs[0] = leftNode->ptrs[leftNode->getLength()];
				leftNode->key[leftNode->getLength()-1] = 0;

				// Update leftNode
				int i = 1;
				while(i<leftNode->getLength()-1){
					treeNodeBlock* temp = (treeNodeBlock*) blkManager->accessBlock(leftNode->ptrs[i].getBlock());
					leftNode->key[i-1] = lowestBound(temp);
				}

				//Update currNode
				i = 1;
				while(i<leftNode->getLength()){
					treeNodeBlock* temp = (treeNodeBlock*) blkManager->accessBlock(leftNode->ptrs[i].getBlock());
					leftNode->key[i-1] = lowestBound(temp);
				}
				
				//update parent
				updateParent(currNode->getParentBlock());
				return;
			}
			// Right sibling can make a transfer
			else if (rightIndex != 0 && rightNode->getLength() >minimum) {			
				// Transfer key from rightNode to currNode
				currNode->ptrs[currNode->getLength() - 1] = rightNode->ptrs[0];

				//Update currNode
				int i = 1;
				while(i<currNode->getLength()){
					treeNodeBlock* temp = (treeNodeBlock*) blkManager->accessBlock(currNode->ptrs[i].getBlock());
					currNode->key[i-1] = lowestBound(temp);
				}

				// Remove rightNode's first key and update subsequent pointers and keys
				for (int i = 0; i < rightNode->getLength(); i++) {
					rightNode->ptrs[i] = rightNode->ptrs[i + 1];
				}
				rightNode->key[rightNode->getLength()] = 0;

				//Update rightNode
				i = 1;
				while(i<rightNode->getLength()+1){
					treeNodeBlock* temp = (treeNodeBlock*) blkManager->accessBlock(rightNode->ptrs[i].getBlock());
					currNode->key[i-1] = lowestBound(temp);
				}

				//update parent
				updateParent(rightNode->getParentBlock());
				return;
			}

			// Merge leftNode and currNode
			else if (leftIndex != 0 && leftNode->getLength() <= minimum)
			{
				// Transfer all keys and pointers from currNode to leftNode
				for (int i = leftNode->getLength()+2, j = 0; j < currNode->getLength()+1; i++, j++)
				{
					leftNode->ptrs[i] = currNode->ptrs[j];
				}

				//Update leftNode
				int i = 1;
				while(i<leftNode->getLength()+1){
					treeNodeBlock* temp = (treeNodeBlock*) blkManager->accessBlock(leftNode->ptrs[i].getBlock());
					currNode->key[i-1] = lowestBound(temp);
				}

				// Delete currNode from parent
				unsigned int parentBlockIndex = currNode->getParentBlock();
				blkManager->deleteBlock(currNodeIndex);

				//recursion for parent
				deleteKeyInternal(currNodeIndex, parentBlockIndex);
				
			}
			// Merge with rightNode
			else if (rightIndex != 0 && rightNode->getLength() <= minimum) {
				// Transfer all keys and pointers from rightNode to currNode
				for (int i = currNode->getLength()+2, j = 0; j < rightNode->getLength()+1; i++, j++)
				{
					currNode->ptrs[i] = rightNode->ptrs[j];
				}
				currNode->ptrs[currNode->ptrs.size()-1] = rightNode->ptrs[rightNode->ptrs.size()-1];

				//Update currNode
				int i = 1;
				while(i<leftNode->getLength()+1){
					treeNodeBlock* temp = (treeNodeBlock*) blkManager->accessBlock(currNode->ptrs[i].getBlock());
					currNode->key[i-1] = lowestBound(temp);
				}

				// Delete rightNode from parent
				unsigned int parentBlockIndex = rightNode->getParentBlock();
				blkManager->deleteBlock(rightIndex);

				//recursion for parent
				deleteKeyInternal(rightIndex, parentBlockIndex);
			}
	}

	unsigned int findLeftIndex(unsigned int blockNumber){
		unsigned int level;
		if(blockNumber == rootNode){
			return 0;
		}

		treeNodeBlock* block = (treeNodeBlock*) blkManager->accessBlock(blockNumber);
		unsigned int curParentBlockIndex = block->getParentBlock();
		treeNodeBlock* curParentBlock = (treeNodeBlock*) blkManager->accessBlock(curParentBlockIndex);
		unsigned int foundParentIndex = 0;
		
		while(foundParentIndex<curParentBlock->ptrs.size() && curParentBlock->ptrs[foundParentIndex].getBlock() != blockNumber){
			foundParentIndex ++;
		}

		unsigned int prevNodeIndex;
		unsigned int prevParentIndex = curParentBlockIndex;
		curParentBlockIndex = curParentBlock->getParentBlock();


		int i = 0;
		while(foundParentIndex!= 0 && curParentBlockIndex!= rootNode){
			foundParentIndex = 0;
			while(foundParentIndex<curParentBlock->ptrs.size() && curParentBlock->ptrs[foundParentIndex].getBlock() != prevParentIndex){
				foundParentIndex++;
			}
			prevParentIndex = curParentBlockIndex;
			curParentBlockIndex = curParentBlock->getParentBlock();
			level++;
		}
		
		return foundParentIndex;
		
	}

	unsigned int findRightIndex(unsigned int blockNumber){
		unsigned int level;
		if(blockNumber == rootNode){
			return 0;
		}

		treeNodeBlock* block = (treeNodeBlock*) blkManager->accessBlock(blockNumber);
		unsigned int curParentBlockIndex = block->getParentBlock();
		treeNodeBlock* curParentBlock = (treeNodeBlock*) blkManager->accessBlock(curParentBlockIndex);
		unsigned int foundParentIndex = 0;
		
		while(foundParentIndex<curParentBlock->ptrs.size() && curParentBlock->ptrs[foundParentIndex].getBlock() != blockNumber){
			foundParentIndex ++;
		}

		unsigned int prevNodeIndex;
		unsigned int prevParentIndex = curParentBlockIndex;
		curParentBlockIndex = curParentBlock->getParentBlock();


		int i = 0;
		while(foundParentIndex!= 0 && curParentBlockIndex!= rootNode){
			foundParentIndex = 0;
			while(foundParentIndex<curParentBlock->ptrs.size() && curParentBlock->ptrs[foundParentIndex].getBlock() != prevParentIndex){
				foundParentIndex++;
			}
			prevParentIndex = curParentBlockIndex;
			curParentBlockIndex = curParentBlock->getParentBlock();
			level++;
		}

		return foundParentIndex;
		
		
	}

	void deleteKeyInternal(unsigned int deletedNodeIndex, unsigned int currentNodeIndex){
		unsigned int minimum = (NUM_KEY_INDEX+1)/2;
		treeNodeBlock* currentNode = (treeNodeBlock*) blkManager->accessBlock(currentNodeIndex);
		treeNodeBlock* deletedNode = (treeNodeBlock*) blkManager->accessBlock(deletedNodeIndex);
		std::vector<Pointer> temp;
		int i = 0;
		while(currentNode->ptrs[i].getBlock()!=0){
			if(currentNode->ptrs[i].getBlock() != deletedNodeIndex){
				temp.push_back(currentNode->ptrs[i]);
			}
			i++;
		}
		i = 0;
		while(i< currentNode->key.size()){
			currentNode->ptrs[i].setBlock(0);
			currentNode->ptrs[i].entry = 0;
			currentNode->key[i] = 0;
			i++;
		}
		currentNode->ptrs[i].setBlock(0);
		currentNode->ptrs[i].entry = 0;
		i = 0;
		currentNode->ptrs[i] = temp[i];
		i++;
		while(i<temp.size()){
			currentNode->ptrs[i] = temp[i];
			unsigned int childNodeIndex = temp[i].getBlock();
			treeNodeBlock* childNode = (treeNodeBlock*) blkManager->accessBlock(childNodeIndex);
			currentNode->key[i-1] = lowestBound(childNode);
		}
		if(currentNode->getLength() <= minimum){
			unsigned int leftIndex = findLeftIndex(currentNodeIndex);
			unsigned int rightIndex = findRightIndex(currentNodeIndex);
			mergeParentNodes(leftIndex, rightIndex,currentNodeIndex);
		}
		return;
		
	}

	// Assuming that we have a root node, finds the block which should contain a particular key.
	// Block need not have space to contain this key
	unsigned int searchBlockToContain(unsigned int key) const {
		unsigned int curIndex = rootNode;
		treeNodeBlock* curBlock = (treeNodeBlock*) blkManager->accessBlock(rootNode);
		if(curBlock -> type == 2){
			return curIndex;
		}
		while(curBlock -> type != 2){
			unsigned int i = 0;
			// C 1 A 4 B
			while(i<curBlock->getLength() && key>=curBlock->key[i]){
				i++;
			}
			curIndex = curBlock->ptrs[i].getBlock();
			curBlock = (treeNodeBlock*) blkManager->accessBlock(curIndex);
		}
		return curIndex;
	}

	void insertKeyToLinkedList(Pointer ptr, linkedListNodeBlock* firstLinkedListNode){
		linkedListNodeBlock* curLinkedList = firstLinkedListNode;
		// Linked list block exist, insert new entry
		unsigned int i = 0;
		while(curLinkedList->pointers[i].getBlock() != 0){
			//This pointer points to a block already
			if(i==(curLinkedList->pointers.size()-1)){
				// This block is full
				if(curLinkedList->nextBlock.getBlock() == 0){
					// No next block, so need to create new block and add into it
					unsigned int newLinkedListIndex = blkManager->createLinkedListBlock();
					linkedListNodeBlock* newLinkedList = (linkedListNodeBlock*)blkManager->accessBlock(newLinkedListIndex);
					newLinkedList->type = 3;
					newLinkedList->pointers[0];
					Pointer newPtr;
					newPtr.setBlock(newLinkedListIndex);
					curLinkedList->nextBlock = newPtr;
					newLinkedList->pointers[0] = ptr;
					return;
				}
				else{
					// Continue searching next block
					i = 0;
					curLinkedList = (linkedListNodeBlock *)blkManager->accessBlock(curLinkedList->nextBlock.getBlock());
				}
			}
			else{
				i++;
			}
		}
		curLinkedList->pointers[i] = ptr;
	}
	//Find lowest bound of a treeNodeBlock
	unsigned int lowestBound(treeNodeBlock* block) const {
		treeNodeBlock* curBlock = block;
		while(curBlock->type != 2){
			curBlock = (treeNodeBlock *)blkManager->accessBlock(curBlock->ptrs[0].getBlock());
		}
		return curBlock->key[0];
	}

	void insertInternal(unsigned int curBlockIndex , const Pointer ptr){
		//printf("---------INSERT INTERNAL----------\n");
		treeNodeBlock* curBlock = (treeNodeBlock*)blkManager->accessBlock(curBlockIndex);
		//unsigned int key = lowestBound((treeNodeBlock*)blkManager->accessBlock(ptr.getBlock()));
		if(curBlock->getLength() < curBlock->key.size()){
			// Has space, just insert
			/*unsigned int locationToInsert = 0;
			while(curBlock->key[locationToInsert] < key && locationToInsert < curBlock->getLength()){
				locationToInsert++;
			}
			if(locationToInsert == (curBlock->key.size() - 1) ){
				curBlock->key[locationToInsert] = key;
				curBlock->ptrs[locationToInsert+ 1] = ptr;
			}
			else{
				unsigned int i = 0;
				std::vector<Pointer> ptrTemp;
				std::vector<unsigned int> keyTemp;
				while(i<curBlock->key.size()){
					i++;
				}
				i=0;
				while(i<curBlock->ptrs.size()){
					i++;
				}
				
				
				//std::move_backward(curBlock->ptrs.begin() + locationToInsert + 1, curBlock->ptrs.end() - 1, curBlock->ptrs.end());
				//std::move_backward(curBlock->key.begin() + locationToInsert, curBlock->key.end() - 1, curBlock->key.end());
				//curBlock->ptrs[locationToInsert+ 1] = ptr;
				//curBlock->key[locationToInsert] = key;
			}*/

			std::vector<Pointer> ptrTemp;
			unsigned int i=0;
			while(i<(curBlock->ptrs.size()-1) && curBlock->ptrs[i].getBlock() != 0){
				ptrTemp.push_back(curBlock->ptrs[i]);
				i++;
			}
			ptrTemp.push_back(ptr);
			std::sort(ptrTemp.begin(), ptrTemp.end(), [&](Pointer a, Pointer b){
				unsigned int aKey = lowestBound((treeNodeBlock*)blkManager->accessBlock(a.getBlock()));
				unsigned int bKey = lowestBound((treeNodeBlock*)blkManager->accessBlock(b.getBlock()));
				return aKey < bKey;
			});
			curBlock->ptrs[0] = ptrTemp[0];
			i=1;
			while(i<ptrTemp.size()){
				curBlock->ptrs[i] = ptrTemp[i];
				curBlock->key[i-1] = lowestBound((treeNodeBlock*)blkManager->accessBlock(ptrTemp[i].getBlock()));
				i++;
			}

			treeNodeBlock* childBlock = (treeNodeBlock*)blkManager->accessBlock(ptr.getBlock());
			childBlock->setParentBlock(curBlockIndex);
			//printTreeNode(curBlockIndex);
		}
		else{
			// No space, need to split
			std::vector<Pointer> pointers;
			for(Pointer p: curBlock->ptrs){
				pointers.push_back(p);
			}
			//Add in new pointer as well
			pointers.push_back(ptr);
			unsigned int i = 0;
			while(i<curBlock->key.size()){
				curBlock->ptrs[i].setBlock(0);
				curBlock->ptrs[i].entry=-1;
				curBlock->key[i] = 0;
				i++;
			}
			curBlock->ptrs[curBlock->ptrs.size()-1].setBlock(0);
			curBlock->ptrs[curBlock->ptrs.size()-1].entry=-1;
			std::sort(pointers.begin(), pointers.end(), [&](Pointer a, Pointer b){
				unsigned int aKey = lowestBound((treeNodeBlock*)blkManager->accessBlock(a.getBlock()));
				unsigned int bKey = lowestBound((treeNodeBlock*)blkManager->accessBlock(b.getBlock()));
				return aKey < bKey;
			});
			unsigned int ptrInNewNode = (curBlock->key.size() / 2)+1;
			unsigned int ptrInCurNode = pointers.size() - ptrInNewNode;

			unsigned int newTreeNodeIndex = blkManager->createIndexBlock();
			treeNodeBlock* newTreeNode = (treeNodeBlock*)blkManager->accessBlock(newTreeNodeIndex);
			newTreeNode->type = 1;
			curBlock->ptrs[0] = pointers[0];

			i = 1;
			while(i<ptrInCurNode){
				curBlock->ptrs[i] = pointers[i];
				curBlock->key[i-1] = lowestBound((treeNodeBlock*)blkManager->accessBlock(pointers[i].getBlock()));
				i++;
			}
			newTreeNode->ptrs[0] = pointers[i];
			i++;
			unsigned int j=1;
			while(j<ptrInNewNode){
				newTreeNode->ptrs[j] = pointers[i];
				newTreeNode->key[j-1] = lowestBound((treeNodeBlock*)blkManager->accessBlock(pointers[i].getBlock()));
				j++;
				i++;
			}
			i = 0;
			while(i<ptrInCurNode){
				treeNodeBlock* updateParent = (treeNodeBlock*)blkManager->accessBlock(curBlock->ptrs[i].getBlock());
				updateParent->setParentBlock(curBlockIndex);
				i++;
			}
			i=0;
			while(i<ptrInNewNode){
				treeNodeBlock* updateParent = (treeNodeBlock*)blkManager->accessBlock(newTreeNode->ptrs[i].getBlock());
				updateParent->setParentBlock(newTreeNodeIndex);
				i++;
			}
			if(curBlockIndex == rootNode){
				// is root, make new root
				unsigned int newRootIndex = blkManager->createIndexBlock();
				treeNodeBlock* newRoot = (treeNodeBlock*)blkManager->accessBlock(newRootIndex);
				newRoot->type = 1;
				rootNode = newRootIndex;

				Pointer firstPtr, secondPtr;
				firstPtr.setBlock(curBlockIndex);
				secondPtr.setBlock(newTreeNodeIndex);
				newRoot->ptrs[0] = firstPtr;
				newRoot->ptrs[1] = secondPtr;
				newRoot->key[0] = lowestBound(newTreeNode);

				// Update parents of 2 blocks
				curBlock->setParentBlock(newRootIndex);
				newTreeNode->setParentBlock(newRootIndex);
			}
			else{
				// Update parent
				Pointer toAddPtr;
				toAddPtr.setBlock(newTreeNodeIndex);
				insertInternal(curBlock->getParentBlock(), toAddPtr);
			}
			//printTreeNode(curBlockIndex);
			//printTreeNode(newTreeNodeIndex);

		}
	}

	void insert(unsigned int key, Pointer ptr){
		//printf("----------INSERT-----------\n");
		if(rootNode == 0){
			//Empty tree, create new tree
			rootNode = blkManager->createIndexBlock();
			treeNodeBlock* rootBlock = (treeNodeBlock*)blkManager->accessBlock(rootNode);
			rootBlock->type = 2;
			rootBlock->key[0] = key;
			rootBlock->ptrs[0] = ptr;
			//printTreeNode(rootNode);
		}
		else{
			const unsigned int blockToInsert = searchBlockToContain(key);
			//printf("BLOCK TO INSERT: %d\n", blockToInsert);
			treeNodeBlock* insertBlock = (treeNodeBlock*)blkManager->accessBlock(blockToInsert);
			//if(insertBlock->getLength() < insertBlock->key.size()){
			if(insertBlock->getLength() < insertBlock->key.size()){
				// Have space, insert into block and done
				unsigned int locationToInsert = 0;
				while(insertBlock->key[locationToInsert] < key && locationToInsert < insertBlock->getLength()){
					locationToInsert++;
				}
				if(insertBlock->key[locationToInsert] == key){
					// Duplicate found
					const Pointer pointed = insertBlock->ptrs[locationToInsert];
					block * pointedBlock = blkManager->accessBlock(pointed.getBlock());
					if(pointedBlock->type == 3){
						// Linked list block exist, insert new entry
						insertKeyToLinkedList(ptr, (linkedListNodeBlock*)pointedBlock);
					}
					else{
						// No linked list block, create new linked list block
						unsigned int newLinkedListIndex = blkManager->createLinkedListBlock();
						linkedListNodeBlock* newLinkedList = (linkedListNodeBlock*)blkManager->accessBlock(newLinkedListIndex);
						newLinkedList->type = 3;
						newLinkedList->pointers[0] = pointed;
						newLinkedList->pointers[1] = ptr;
						Pointer newLinkedListPointer;
						newLinkedListPointer.setBlock(newLinkedListIndex);
						insertBlock->ptrs[locationToInsert] = newLinkedListPointer;
					}
				}
				else{
					// No duplicate, shift all other keys and pointers back
					// Move Nth - getLength()th key and ptr back by 1
					// Insert to Nth index
					//const Pointer IAmNotArsedToDoAWhileLoop = insertBlock->ptrs[insertBlock->ptrs.size() -1 ];
					
					std::vector<unsigned int> keyTemp;
					std::vector<Pointer> ptrTemp;

					unsigned int i=0;
					while(i<(insertBlock->key.size()-1)){
						keyTemp.push_back(insertBlock->key[i]);
						ptrTemp.push_back(insertBlock->ptrs[i]);
						i++;
					}
					ptrTemp.insert(ptrTemp.begin() + locationToInsert, ptr );
					keyTemp.insert(keyTemp.begin() + locationToInsert, key );
					//std::move_backward(insertBlock->ptrs.begin() + locationToInsert, insertBlock->ptrs.end() - 1, insertBlock->ptrs.end());
					//std::copy(insertBlock->ptrs.begin() + locationToInsert, insertBlock->ptrs.end(), std::back_inserter(ptrTemp));
					
					//std::copy(insertBlock->key.begin() + locationToInsert, insertBlock->key.end(), std::back_inserter(keyTemp));
					//std::move_backward(insertBlock->key.begin() + locationToInsert, insertBlock->key.end() - 1, insertBlock->key.end());
					i=0;
					while(i<keyTemp.size()){
						//std::cout<<"Temp Key: "<<keyTemp[i]<<" ptr: "<<ptrTemp[i].getBlock();
						//insertBlock->ptrs[i].setBlock(ptrTemp[i].getBlock());
						insertBlock->ptrs[i] = ptrTemp[i];
						insertBlock->key[i] = keyTemp[i];
						i++;
					}
					insertBlock->ptrs[locationToInsert] = ptr;
					insertBlock->key[locationToInsert] = key;
					//std::cout<<std::endl;
					//std::copy(ptrTemp.begin(), ptrTemp.end(), insertBlock->ptrs.begin() + locationToInsert);
					//std::copy(keyTemp.begin(), keyTemp.end(), insertBlock->key.begin() + locationToInsert);

					//insertBlock->ptrs[insertBlock->ptrs.size() -1 ] = IAmNotArsedToDoAWhileLoop;
					//printTreeNode(blockToInsert);
					
				}
			}
			else{
				// No space, need to check if duplicate exists
				unsigned int locationToInsert = 0;
				while(insertBlock->key[locationToInsert] < key && locationToInsert < insertBlock->getLength()){
					locationToInsert++;
				}
				if(insertBlock->key[locationToInsert] == key){
					// Duplicate found
					Pointer pointed = insertBlock->ptrs[locationToInsert];
					block * pointedBlock = blkManager->accessBlock(pointed.getBlock());
					if(pointedBlock->type == 3){
						// Linked list block exist, insert new entry
						insertKeyToLinkedList(ptr, (linkedListNodeBlock*)pointedBlock);
					}
					else{
						// No linked list block, create new linked list block
						unsigned int newLinkedListIndex = blkManager->createLinkedListBlock();
						linkedListNodeBlock* newLinkedList = (linkedListNodeBlock*)blkManager->accessBlock(newLinkedListIndex);
						newLinkedList->type = 3;
						newLinkedList->pointers[0] = pointed;
						newLinkedList->pointers[1] = ptr;
						Pointer newLinkedListPointer;
						newLinkedListPointer.setBlock(newLinkedListIndex);
						insertBlock->ptrs[locationToInsert] = newLinkedListPointer;
					}
					//printTreeNode(blockToInsert);
				}
				else{
					// No duplicates exists, need to split
					// Gather all keys and ptr
					std::vector<std::pair<Pointer, unsigned int>> pointers;
					unsigned int i = 0;
					while(i<insertBlock->key.size()){
						pointers.push_back(std::make_pair(insertBlock->ptrs[i], insertBlock->key[i]));
						i++;
					}
					pointers.push_back(std::make_pair(ptr, key));
					i=0;
					while(i<insertBlock->key.size()){
						insertBlock->ptrs[i].setBlock(0);
						insertBlock->ptrs[i].entry=0;
						insertBlock->key[i]=0;
						i++;
					}
					std::sort(pointers.begin(), pointers.end(), [](std::pair<Pointer, unsigned int> a, std::pair<Pointer, unsigned int> b){
						return a.second < b.second;
					});
					const unsigned int recordInNewBlock = (insertBlock->key.size() + 1) / 2;
					const unsigned int recordInInsertBlock = pointers.size() - recordInNewBlock;
					i=0;
					// Fill up old block
					while(i<recordInInsertBlock){
						insertBlock->ptrs[i] = pointers[i].first;
						insertBlock->key[i] = pointers[i].second;
						i++;
					}

					// Create new block and fill up
					const unsigned int newTreeBlockIndex = blkManager->createIndexBlock();
					treeNodeBlock *newTreeBlock = (treeNodeBlock *)blkManager->accessBlock(newTreeBlockIndex);
					newTreeBlock->type = 2;
					unsigned int j=0;
					while(i<pointers.size()){
						newTreeBlock->ptrs[j] = pointers[i].first;
						newTreeBlock->key[j] = pointers[i].second;
						j++;
						i++;
					}
					// Copy over what the current node points to as next block to the new block
					newTreeBlock->ptrs[newTreeBlock->ptrs.size()-1] = insertBlock->ptrs[insertBlock->ptrs.size()-1];

					// Set next block for current block to be new block
					insertBlock->ptrs[insertBlock->ptrs.size()-1].setBlock(newTreeBlockIndex);

					
					if(blockToInsert == rootNode){
						unsigned int newParentIndex = blkManager->createIndexBlock();

						//Set parent block index
						newTreeBlock->setParentBlock(newParentIndex);
						insertBlock->setParentBlock(newParentIndex);
						treeNodeBlock *newParent = (treeNodeBlock *)blkManager->accessBlock(newParentIndex);
						newParent->type = 1;
						// Root node is current only leaf node. The new parent will be the root.
						rootNode = newParentIndex;

						newParent->ptrs[0].setBlock(blockToInsert);
						newParent->ptrs[1].setBlock(newTreeBlockIndex);

						newParent->key[0] = newTreeBlock->key[0];
					}
					
					else{
						Pointer toAddPtr;
						toAddPtr.setBlock(newTreeBlockIndex);
						insertInternal(insertBlock->getParentBlock(), toAddPtr);
					}
					//printTreeNode(blockToInsert);
					//printTreeNode(newTreeBlockIndex);
				}
			}
		}
	}
};

#endif