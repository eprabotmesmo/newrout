#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "algorithm.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define DEBUG 1

#define DIAGONAL 14
#define ORTOGONAL 10

#ifdef WIN32
	#include <windows.h>
#else
	#include <sys/time.h>
	static unsigned long
	GetTickCount ()
	{
		struct timeval tv;
		gettimeofday (&tv, (struct timezone *) NULL);
		return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
	}
#endif /* WIN32 */


/*******************************************/


// Create a new, empty pathfinding session.
// You must initialize it with CalcPath_init()
CalcPath_session *
CalcPath_new ()
{
	CalcPath_session *session;

	session = (CalcPath_session*) malloc (sizeof (CalcPath_session));
	
	session->initialized = 0;
	session->run = 0;
	
	return session;
}

unsigned long*
calcKey (Node* node, unsigned int startX, unsigned int startY, bool avoidWalls, unsigned int k)
{
	static unsigned long key[2];
	
    key[1] = ((node->g > node->rhs) ? node->rhs : node->g);
	
	unsigned short h = heuristic_cost_estimate(node->x, node->y, startX, startY, avoidWalls);
	
	key[0] = key[1] + h + k;
	
    return key;
}


//
int 
heuristic_cost_estimate (int currentX, int currentY, int startX, int startY, int avoidWalls)
{
    int xDistance = abs(currentX - startX);
    int yDistance = abs(currentY - startY);
    
    int hScore = (ORTOGONAL * (xDistance + yDistance)) + ((DIAGONAL - (2 * ORTOGONAL)) * ((xDistance > yDistance) ? yDistance : xDistance));
    
    if (avoidWalls) {
        hScore += (((xDistance > yDistance) ? xDistance : yDistance) * 10);
    }
    
    return hScore;
}

int
recheck_all_nodes_in_binary_heap (CalcPath_session *session)
{
	long lastIndex = session->openListSize-1;
	
	long currentIndex = 0;
	long leftChildIndex;
	long rightChildIndex;
	
	unsigned long currentAdress;
	unsigned long leftChildAdress;
	unsigned long rightChildAdress;
	
	Node* currentNode;
	Node* rightChildNode;
	Node* leftChildNode;
	
	while (lastIndex >= currentIndex) {
		
		currentAdress = session->openList[currentIndex];
		currentNode = &session->currentMap[currentAdress];
		
		leftChildIndex = 2 * currentIndex + 1;
		rightChildIndex = 2 * currentIndex + 2;
		
		if (lastIndex >= leftChildIndex) {
			leftChildAdress = session->openList[leftChildIndex];
			leftChildNode = &session->currentMap[leftChildAdress];
			
			if (currentNode->key1 > leftChildNode->key1 || (currentNode->key1 == leftChildNode->key1 && currentNode->key2 > leftChildNode->key2)) {
				
				printf("[Pathfinding error] Current node %d %d, at index %ld, has keys %lu and %lu || left child node %d %d, at index %ld, has keys %lu and %lu\n", currentNode->x, currentNode->y, currentIndex, currentNode->key1, currentNode->key2, leftChildNode->x, leftChildNode->y, leftChildIndex, leftChildNode->key1, leftChildNode->key2);
				return 0;
			}
		}
		
		if (lastIndex >= rightChildIndex) {
			rightChildAdress = session->openList[rightChildIndex];
			rightChildNode = &session->currentMap[rightChildAdress];
			if (currentNode->key1 > rightChildNode->key1 || (currentNode->key1 == rightChildNode->key1 && currentNode->key2 > rightChildNode->key2)) {
				
				printf("[Pathfinding error] Current node %d %d, at index %ld, has keys %lu and %lu || right child node %d %d, at index %ld, has keys %lu and %lu\n", currentNode->x, currentNode->y, currentIndex, currentNode->key1, currentNode->key2, rightChildNode->x, rightChildNode->y, rightChildIndex, rightChildNode->key1, rightChildNode->key2);
				return 0;
			}
		}
		
		currentIndex++;
	}
	
	return 1;
}

int
recheck_openList_removed (CalcPath_session *session, Node* removedNode)
{
	long lastIndex = session->openListSize-1;
	
	long currentIndex = 0;
	
	unsigned long currentAdress;
	
	Node* currentNode;
	
	while (lastIndex >= currentIndex) {
		
		currentAdress = session->openList[currentIndex];
		currentNode = &session->currentMap[currentAdress];
		
		if (removedNode->nodeAdress == currentNode->nodeAdress) {
			printf("[Pathfinding error] Current node %d %d, at index %ld, was removed from openList but is still in it\n", currentNode->x, currentNode->y, currentIndex);
			return 0;
		}
		
		currentIndex++;
	}
	
	return 1;
}

// Openlist is a binary heap of min-heap type
// Each member in openList is the adress (nodeAdress) of a node in the map (session->currentMap)

// Add node 'currentNode' to openList
void 
openListAdd (CalcPath_session *session, Node* currentNode)
{
	// Index will be 1 + last index in openList, which is also its size
	// Save in currentNode its index in openList
    currentNode->openListIndex = session->openListSize;
	currentNode->isInOpenList = 1;
	
	// Defines openList[index] to currentNode adress
    session->openList[currentNode->openListIndex] = currentNode->nodeAdress;
	
	// Increses openListSize by 1, since we just added a new member
	session->openListSize++;
	
	long parentIndex = (long)floor((currentNode->openListIndex - 1) / 2);
	Node* parentNode;
	
	// Repeat while currentNode still has a parent node, otherwise currentNode is the top node in the heap
    while (parentIndex >= 0) {
		
		parentNode = &session->currentMap[session->openList[parentIndex]];
		
		// If parent node is bigger than currentNode, exchange their positions
		if (parentNode->key1 > currentNode->key1 || (parentNode->key1 == currentNode->key1 && parentNode->key2 > currentNode->key2)) {
			// Changes the node adress of openList[currentNode->openListIndex] (which is 'currentNode') to that of openList[parentIndex] (which is the current parent of 'currentNode')
            session->openList[currentNode->openListIndex] = session->openList[parentIndex];
			
			// Changes openListIndex of the current parent of 'currentNode' to that of 'currentNode' since they exchanged positions
            parentNode->openListIndex = currentNode->openListIndex;
			
			// Changes the node adress of openList[parentIndex] (which is the current parent of 'currentNode') to that of openList[currentNode->openListIndex] (which is 'currentNode')
            session->openList[parentIndex] = currentNode->nodeAdress;
			
			// Changes openListIndex of 'currentNode' to that of the current parent of 'currentNode' since they exchanged positions
            currentNode->openListIndex = parentIndex;
			
			// Updates parentIndex to that of the current parent of 'currentNode'
			parentIndex = (long)floor((currentNode->openListIndex - 1) / 2);
			
        } else {
			break;
		}
	}
	
	if (DEBUG) {
		int result;
		result = recheck_all_nodes_in_binary_heap(session);
		if (result == 0) {
			printf("[Pathfinding error] OpenList integrity check failed after function openListAdd.\n");
		}
	}
}

// Remove node 'currentNode' from openList
void 
openListRemove (CalcPath_session *session, Node* currentNode)
{
	
	// Decreases openList size
	session->openListSize--;
	
	// Cannot move last node to this node place if this node is the last in openList
	if (currentNode->openListIndex == session->openListSize) {
		currentNode->isInOpenList = 0;
		currentNode->openListIndex = 0;
		
	} else {
	
		// Since it was decreaased, but the node was not removed yet, session->openListSize is now also the index of the last node in openList
		// We move the last node in openList to this position and adjust it down as necessary
		session->openList[currentNode->openListIndex] = session->openList[session->openListSize];
		
		Node* movedNode;
		
		// TODO
		movedNode = &session->currentMap[session->openList[currentNode->openListIndex]];
		
		// TODO
		movedNode->openListIndex = currentNode->openListIndex;
		
		// Saves in currentNode that it is no longer in openList
		currentNode->isInOpenList = 0;
		currentNode->openListIndex = 0;
		
		long parentIndex = (long)floor((movedNode->openListIndex - 1) / 2);
		
		// Sometimes we may need to move it up (look http://www.mathcs.emory.edu/~cheung/Courses/171/Syllabus/9-BinTree/heap-delete.html)
		if (parentIndex >= 0 && (session->currentMap[session->openList[parentIndex]].key1 > movedNode->key1 || (session->currentMap[session->openList[parentIndex]].key1 == movedNode->key1 && session->currentMap[session->openList[parentIndex]].key2 > movedNode->key2))) {
				
			Node* parentNode;
			while (parentIndex >= 0) {
				parentNode = &session->currentMap[session->openList[parentIndex]];
				
				if (parentNode->key1 > movedNode->key1 || (parentNode->key1 == movedNode->key1 && parentNode->key2 > movedNode->key2)) {
					session->openList[movedNode->openListIndex] = session->openList[parentIndex];
					
					parentNode->openListIndex = movedNode->openListIndex;
					
					session->openList[parentIndex] = movedNode->nodeAdress;
					
					movedNode->openListIndex = parentIndex;
					
					parentIndex = (long)floor((movedNode->openListIndex - 1) / 2);
					
				} else {
					break;
				}
			}
		
		// But almost always it will need to be moved down
		} else {
			long smallerChildIndex;
			Node* smallerChildNode;
			
			long rightChildIndex = 2 * movedNode->openListIndex + 2;
			Node* rightChildNode;
			
			long leftChildIndex = 2 * movedNode->openListIndex + 1;
			Node* leftChildNode;
			
			long lastIndex = session->openListSize-1;
			
			while (leftChildIndex <= lastIndex) {

				//There are 2 children
				if (rightChildIndex <= lastIndex) {
					
					rightChildNode = &session->currentMap[session->openList[rightChildIndex]];
					leftChildNode = &session->currentMap[session->openList[leftChildIndex]];
					
					if (rightChildNode->key1 > leftChildNode->key1 || (rightChildNode->key1 == leftChildNode->key1 && rightChildNode->key2 > leftChildNode->key2)) {
						smallerChildIndex = leftChildIndex;
					} else {
						smallerChildIndex = rightChildIndex;
					}
				
				//There is 1 children
				} else {
					smallerChildIndex = leftChildIndex;
				}
				
				smallerChildNode = &session->currentMap[session->openList[smallerChildIndex]];
				
				if (movedNode->key1 > smallerChildNode->key1 || (movedNode->key1 == smallerChildNode->key1 && movedNode->key2 > smallerChildNode->key2)) {
					
					// Changes the node adress of openList[movedNode->openListIndex] (which is 'movedNode') to that of openList[smallerChildIndex] (which is the current child of 'movedNode')
					session->openList[movedNode->openListIndex] = smallerChildNode->nodeAdress;
					
					// Changes openListIndex of the current child of 'movedNode' to that of 'movedNode' since they exchanged positions
					smallerChildNode->openListIndex = movedNode->openListIndex;
					
					// Changes the node adress of openList[smallerChildIndex] (which is the current child of 'movedNode') to that of openList[movedNode->openListIndex] (which is 'movedNode')
					session->openList[smallerChildIndex] = movedNode->nodeAdress;
					
					// Changes openListIndex of 'movedNode' to that of the current child of 'movedNode' since they exchanged positions
					movedNode->openListIndex = smallerChildIndex;
					
					// Updates rightChildIndex and leftChildIndex to those of the current children of 'movedNode'
					rightChildIndex = 2 * movedNode->openListIndex + 2;
					leftChildIndex = 2 * movedNode->openListIndex + 1;
					
				} else {
					break;
				}
			}
		}
	}
	
	if (DEBUG) {
		int result;
		result = recheck_all_nodes_in_binary_heap(session);
		if (result == 0) {
			printf("[Pathfinding error] OpenList integrity check failed after function openListRemove.\n");
		}
	}
	
	if (DEBUG) {
		recheck_openList_removed(session, currentNode);
	}
}

// Reajusts node 'currentNode' location in openList
void 
reajustOpenListItem (CalcPath_session *session, Node* currentNode, unsigned long newkey1, unsigned long newkey2)
{
	// Node got ligher, so we ajust it up
	if (currentNode->key1 > newkey1 || (currentNode->key1 == newkey1 && currentNode->key2 > newkey2)) {
		// Dont really know abut the 2 next lines ( TODO )
		currentNode->key1 = newkey1;
		currentNode->key2 = newkey2;
		
		long parentIndex = (long)floor((currentNode->openListIndex - 1) / 2);
		Node* parentNode;
		
		while (parentIndex >= 0) {
			parentNode = &session->currentMap[session->openList[parentIndex]];
			
			if (parentNode->key1 > currentNode->key1 || (parentNode->key1 == currentNode->key1 && parentNode->key2 > currentNode->key2)) {
				session->openList[currentNode->openListIndex] = session->openList[parentIndex];
				
				parentNode->openListIndex = currentNode->openListIndex;
				
				session->openList[parentIndex] = currentNode->nodeAdress;
				
				currentNode->openListIndex = parentIndex;
				
				parentIndex = (long)floor((currentNode->openListIndex - 1) / 2);
				
			} else {
				break;
			}
		}
	
	// Node got heavier, so we ajust it down
	} else {
		// Dont really know abut the 2 next lines ( TODO )
		currentNode->key1 = newkey1;
		currentNode->key2 = newkey2;
	
		long smallerChildIndex;
		Node* smallerChildNode;
		
		long rightChildIndex = 2 * currentNode->openListIndex + 2;
		Node* rightChildNode;
		
		long leftChildIndex = 2 * currentNode->openListIndex + 1;
		Node* leftChildNode;
		
		long lastIndex = session->openListSize-1;
		
		while (leftChildIndex <= lastIndex) {

			//There are 2 children
			if (rightChildIndex <= lastIndex) {
				
				rightChildNode = &session->currentMap[session->openList[rightChildIndex]];
				leftChildNode = &session->currentMap[session->openList[leftChildIndex]];
				
				if (rightChildNode->key1 > leftChildNode->key1 || (rightChildNode->key1 == leftChildNode->key1 && rightChildNode->key2 > leftChildNode->key2)) {
					smallerChildIndex = leftChildIndex;
				} else {
					smallerChildIndex = rightChildIndex;
				}
			
			//There is 1 children
			} else {
				smallerChildIndex = leftChildIndex;
			}
			
			smallerChildNode = &session->currentMap[session->openList[smallerChildIndex]];
			
			if (currentNode->key1 > smallerChildNode->key1 || (currentNode->key1 == smallerChildNode->key1 && currentNode->key2 > smallerChildNode->key2)) {
				
				// Changes the node adress of openList[currentNode->openListIndex] (which is 'currentNode') to that of openList[smallerChildIndex] (which is the current child of 'currentNode')
				session->openList[currentNode->openListIndex] = smallerChildNode->nodeAdress;
				
				// Changes openListIndex of the current child of 'currentNode' to that of 'currentNode' since they exchanged positions
				smallerChildNode->openListIndex = currentNode->openListIndex;
				
				// Changes the node adress of openList[smallerChildIndex] (which is the current child of 'currentNode') to that of openList[currentNode->openListIndex] (which is 'currentNode')
				session->openList[smallerChildIndex] = currentNode->nodeAdress;
				
				// Changes openListIndex of 'currentNode' to that of the current child of 'currentNode' since they exchanged positions
				currentNode->openListIndex = smallerChildIndex;
				
				// Updates rightChildIndex and leftChildIndex to those of the current children of 'currentNode'
				rightChildIndex = 2 * currentNode->openListIndex + 2;
				leftChildIndex = 2 * currentNode->openListIndex + 1;
				
			} else {
				break;
			}
		}
	}
		if (DEBUG) {
			int result;
			result = recheck_all_nodes_in_binary_heap(session);
			if (result == 0) {
				printf("[Pathfinding error] OpenList integrity check failed after function reajustOpenListItem.\n");
			}
		}
}

Node* 
openListGetLowest (CalcPath_session *session)
{
    Node* lowestNode = &session->currentMap[session->openList[0]];
    return lowestNode;
}

void 
updateNode (CalcPath_session *session, Node* currentNode)
{
	if (currentNode->g != currentNode->rhs) {
		if (currentNode->isInOpenList) {
			unsigned long* keys = calcKey(currentNode, session->startX, session->startY, session->avoidWalls, session->k);
			reajustOpenListItem(session, currentNode, keys[0], keys[1]);
		} else {
			unsigned long* keys = calcKey(currentNode, session->startX, session->startY, session->avoidWalls, session->k);
			currentNode->key1 = keys[0];
			currentNode->key2 = keys[1];
			openListAdd (session, currentNode);
		}
		
	} else if (currentNode->isInOpenList) {
		openListRemove(session, currentNode);
	}
}

void 
reconstruct_path(CalcPath_session *session, Node* goal, Node* start)
{
	Node* currentNode = start;
	
	session->solution_size = 0;
	while (currentNode->nodeAdress != goal->nodeAdress)
    {
        currentNode = &session->currentMap[currentNode->sucessor];
		session->solution_size++;
    }
}

int 
CalcPath_pathStep (CalcPath_session *session)
{
	if (!session->initialized) {
		return -2;
	}
	
	Node* start = &session->currentMap[((session->startY * session->width) + session->startX)];
	Node* goal = &session->currentMap[((session->endY * session->width) + session->endX)];
	
	unsigned long* keys;
	
	if (!session->run) {
		session->run = 1;
		session->openListSize = 0;
		session->openList = (unsigned long*) malloc((session->height * session->width) * sizeof(unsigned long));

		keys = calcKey(goal, session->startX, session->startY, session->avoidWalls, session->k);
		
		goal->key1 = keys[0];
		goal->key2 = keys[1];
		openListAdd (session, goal);
	}
	
	Node* currentNode;
	Node* neighborNode;
	
	int i;
	int j;
	
	unsigned int neighbor_x;
	unsigned int neighbor_y;
	unsigned long neighbor_adress;
	unsigned long distanceFromCurrent;
	
	unsigned long timeout = (unsigned long) GetTickCount();
	int loop = 0;
	
	keys = calcKey(start, session->startX, session->startY, session->avoidWalls, session->k);
	start->key1 = keys[0];
	start->key2 = keys[1];
	
	
    while (1) {
		// No path exists
		if (session->openListSize == 0) {
			return -1;
		}
		
        // get lowest key score member of openlist and delete it from it, shrinks openListSize
		currentNode = openListGetLowest (session);
		
		keys = calcKey(start, session->startX, session->startY, session->avoidWalls, session->k);
		
		start->key1 = keys[0];
		start->key2 = keys[1];
		
		keys = calcKey(currentNode, session->startX, session->startY, session->avoidWalls, session->k);

		// Path found
		if (!((start->key1 > currentNode->key1 || (start->key1 == currentNode->key1 && start->key2 > currentNode->key2)) || start->rhs > start->g)) {
			reconstruct_path(session, goal, start);
			return 1;
		}
		
		// Timer count
		loop++;
		if (loop == 100) {
			if (GetTickCount() - timeout > session->time_max) {
				return 0;
			} else
				loop = 0;
		}
		
		if (keys[0] > currentNode->key1 || (keys[0] == currentNode->key1 && keys[1] > currentNode->key2)) {
			// Node should be lower in priority queue than it is now, downgrade it
			reajustOpenListItem(session, currentNode, keys[0], keys[1]);
			
		} else if (currentNode->g > currentNode->rhs) {
			// Node is overconsistent, expand it and remove it from piority queue
			currentNode->g = currentNode->rhs;
			openListRemove(session, currentNode);
			
			// Get all neighbors
			for (i = -1; i <= 1; i++)
			{
				for (j = -1; j <= 1; j++)
				{
					if (i == 0 && j == 0) {
						continue;
					}
					neighbor_x = currentNode->x + i;
					neighbor_y = currentNode->y + j;

					if (neighbor_x >= session->width || neighbor_y >= session->height || neighbor_x < 0 || neighbor_y < 0) {
						continue;
					}

					neighbor_adress = (neighbor_y * session->width) + neighbor_x;

					neighborNode = &session->currentMap[neighbor_adress];

					if (neighborNode->weight == 0) {
						continue;
					}

					if (i != 0 && j != 0) {
						if (session->currentMap[(currentNode->y * session->width) + neighborNode->x].weight == 0 || session->currentMap[(neighborNode->y * session->width) + currentNode->x].weight == 0) {
							continue;
						}
						distanceFromCurrent = DIAGONAL;
					} else {
						distanceFromCurrent = ORTOGONAL;
					}

					if (session->avoidWalls) {
						distanceFromCurrent += neighborNode->weight;
					}

					if (neighbor_x == session->endX && neighbor_y == session->endY) {
						continue;
					}

					// If current cell weight + distant to next cell is lower than next cell's rhs, current cell becomes the neghbor cell's new sucessor
					if (neighborNode->rhs > (currentNode->g + distanceFromCurrent)) {
						neighborNode->sucessor = currentNode->nodeAdress;
						neighborNode->rhs = currentNode->g + distanceFromCurrent;
						updateNode(session, neighborNode);
					}
				}
			}
			
			
		} else {
			// Node is underconsistent, recalculate all the rhs values and sucessors of all cells that have this Node set as their sucessor
			currentNode->g = 10000000;
			updateNode(session, currentNode);
			
			// Get all neighbors
			for (i = -1; i <= 1; i++)
			{
				for (j = -1; j <= 1; j++)
				{
					if (i == 0 && j == 0) {
						continue;
					}
					neighbor_x = currentNode->x + i;
					neighbor_y = currentNode->y + j;

					if (neighbor_x >= session->width || neighbor_y >= session->height || neighbor_x < 0 || neighbor_y < 0) {
						continue;
					}

					neighbor_adress = (neighbor_y * session->width) + neighbor_x;

					neighborNode = &session->currentMap[neighbor_adress];

					if (neighborNode->weight == 0) {
						continue;
					}
					
					if (neighbor_x == session->endX && neighbor_y == session->endY) {
						continue;
					}
					
					// Check if neighbor's sucessor is current Node, if so get a new sucessor for the neighbot node
					if (neighborNode->sucessor == currentNode->nodeAdress) {
						get_new_neighbor_sucessor(session, neighborNode);
					}
				}
			}
		}
	}
}

// Get the neighbor with the least distance + weight and set it as the new sucessor
void
get_new_neighbor_sucessor (CalcPath_session *session, Node *currentNode)
{
	currentNode->rhs = 10000000;
	
	Node* neighborNode;
	
	int i;
	int j;
	
	unsigned int neighbor_x;
	unsigned int neighbor_y;
	unsigned long neighbor_adress;
	unsigned long distanceFromCurrent;
	
	
	// Get all neighbors
	for (i = -1; i <= 1; i++)
	{
		for (j = -1; j <= 1; j++) {
			if (i == 0 && j == 0) {
				continue;
			}
			neighbor_x = currentNode->x + i;
			neighbor_y = currentNode->y + j;
				
			if (neighbor_x >= session->width || neighbor_y >= session->height || neighbor_x < 0 || neighbor_y < 0) {
				continue;
			}
	
			neighbor_adress = (neighbor_y * session->width) + neighbor_x;

			neighborNode = &session->currentMap[neighbor_adress];

			if (neighborNode->weight == 0) {
				continue;
			}
			
			if (i != 0 && j != 0) {
				if (session->currentMap[(currentNode->y * session->width) + neighborNode->x].weight == 0 || session->currentMap[(neighborNode->y * session->width) + currentNode->x].weight == 0) {
					continue;
				}
				distanceFromCurrent = DIAGONAL;
			} else {
				distanceFromCurrent = ORTOGONAL;
			}
			
			if (session->avoidWalls) {
				distanceFromCurrent += currentNode->weight;
			}
			
			if (neighbor_x == session->endX && neighbor_y == session->endY) {
				continue;
			}
			
			// If current cell weight + distant to next cell is lower than next cell's rhs, current cell becomes the neghbor cell's new sucessor
			if (currentNode->rhs > neighborNode->g + distanceFromCurrent) {
				currentNode->rhs = neighborNode->g + distanceFromCurrent;
				currentNode->sucessor = neighbor_adress;
			}
		}
	}
	
	updateNode(session, currentNode);
}

// Create a new pathfinding session, or reset an existing session.
// Resetting is preferred over destroying and creating, because it saves
// unnecessary memory allocations, thus improving performance.
CalcPath_session *
CalcPath_init (CalcPath_session *session, unsigned char *map)
{
	/* Allocate enough memory in currentMap to hold all cells in the map */
	session->currentMap = (Node*) calloc(session->height * session->width, sizeof(Node));
	
	unsigned int x = 0;
	unsigned int y = 0;
	
	unsigned long current;
	for (y = 0; y < session->height; y++) {
		for (x = 0; x < session->width; x++) {
			current = (y * session->width) + x;
			session->currentMap[current].x = x;
			session->currentMap[current].y = y;
			session->currentMap[current].weight = map[current];
			session->currentMap[current].nodeAdress = current;
			session->currentMap[current].g = 10000000;
			session->currentMap[current].rhs = 10000000;
		}
	}
	
	session->currentMap[(session->endY * session->width) + session->endX].rhs = 0;
	
	session->k = 0;
	session->initialized = 1;
	
	return session;
}

// Updates a block weight
int
updateChangedMap (CalcPath_session *session, unsigned int x, unsigned int y, long delta_weight)
{
	unsigned long current = (y * session->width) + x;
	
	Node* currentNode = &session->currentMap[current];
	
	long old_weight = currentNode->weight;
	
	long new_weight = old_weight + delta_weight;
	
	if (DEBUG) {
		if (new_weight <= 0) {
			printf("[Pathfinding error] Map node set to have negative weight on updateChangedMap (%d %d || from %ld to %ld || delta_weight %ld).\n", x, y, old_weight, new_weight, delta_weight);
			return 0;
		}
	}
	
	currentNode->weight = new_weight;
	
	//TODO: should we do this trick no never updatade cells we haven't reached yet?
	if (currentNode->rhs == 10000000 || (session->endX == currentNode->x && session->endY == currentNode->y)) {
		return 1;
	}
	
	//TODO: should we change rhs and g values?
	currentNode->rhs = currentNode->rhs + delta_weight;
	currentNode->g = currentNode->g + delta_weight;
	
	Node* neighborNode;
	
	int i;
	int j;
	
	unsigned int neighbor_x;
	unsigned int neighbor_y;
	unsigned long neighbor_adress;
	unsigned long distanceFromCurrent;
	
	// If cell got ligher it may have new sucessors
	if (old_weight > new_weight) {
		// Get all neighbors
		for (i = -1; i <= 1; i++)
		{
			for (j = -1; j <= 1; j++)
			{
				if (i == 0 && j == 0) {
					continue;
				}
				neighbor_x = x + i;
				neighbor_y = y + j;
					
				if (neighbor_x >= session->width || neighbor_y >= session->height || neighbor_x < 0 || neighbor_y < 0) {
					continue;
				}
	
				neighbor_adress = (neighbor_y * session->width) + neighbor_x;

				neighborNode = &session->currentMap[neighbor_adress];

				if (neighborNode->weight == 0) {
					continue;
				}
				
				if (i != 0 && j != 0) {
					if (session->currentMap[(currentNode->y * session->width) + neighborNode->x].weight == 0 || session->currentMap[(neighborNode->y * session->width) + currentNode->x].weight == 0) {
						continue;
					}
					distanceFromCurrent = DIAGONAL;
				} else {
					distanceFromCurrent = ORTOGONAL;
				}
				
				if (session->avoidWalls) {
					distanceFromCurrent += neighborNode->weight;
				}
				
				if (neighbor_x == session->endX && neighbor_y == session->endY) {
					continue;
				}
				
				// If current cell weight + distant to next cell is lower than next cell's rhs, current cell becomes the neghbor cell's new sucessor
				if (neighborNode->rhs > (currentNode->g + distanceFromCurrent)) {
					neighborNode->sucessor = currentNode->nodeAdress;
					neighborNode->rhs = currentNode->g + distanceFromCurrent;
					updateNode(session, neighborNode);
					
					
				}
			}
		}
	
	// If cell got heavier it may have lost some sucessors
	} else {
		// Get all neighbors
		for (i = -1; i <= 1; i++)
		{
			for (j = -1; j <= 1; j++)
			{
				if (i == 0 && j == 0) {
					continue;
				}
				neighbor_x = x + i;
				neighbor_y = y + j;

				if (neighbor_x >= session->width || neighbor_y >= session->height || neighbor_x < 0 || neighbor_y < 0) {
					continue;
				}

				neighbor_adress = (neighbor_y * session->width) + neighbor_x;

				neighborNode = &session->currentMap[neighbor_adress];

				if (neighborNode->weight == 0) {
					continue;
				}
				
				if (i != 0 && j != 0) {
					if (session->currentMap[(currentNode->y * session->width) + neighborNode->x].weight == 0 || session->currentMap[(neighborNode->y * session->width) + currentNode->x].weight == 0) {
						continue;
					}
				}
					
				if (neighbor_x == session->endX && neighbor_y == session->endY) {
					continue;
				}
				
				// Check if neighbor's sucessor is current Node, if so get a new sucessor for the neighbor node
				if (neighborNode->sucessor == currentNode->nodeAdress) {
					get_new_neighbor_sucessor(session, neighborNode);
				}
			}
		}
	}
	
	if (DEBUG) {
		int result;
		result = recheck_all_nodes_in_binary_heap(session);
		if (result == 0) {
			printf("[Pathfinding error] OpenList integrity check failed after function updateChangedMap.\n");
			return 0;
		}
	}
	
	updateNode(session, currentNode);
	return 1;
}

void
free_currentMap (CalcPath_session *session)
{
	free(session->currentMap);
}

void
free_openList (CalcPath_session *session)
{
	free(session->openList);
}

void
CalcPath_destroy (CalcPath_session *session)
{
	if (session->initialized) {
		free(session->currentMap);
	}
	
	if (session->run) {
		free(session->openList);
	}
	free(session);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
