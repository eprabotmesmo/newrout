#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "algorithm.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define DIAGONAL 14
#define ORTOGONAL 10
#define NONE 0
#define OPEN 1
#define CLOSED 2
#define PATH 3
#define LCHILD(currentIndex) 2 * currentIndex + 1
#define RCHILD(currentIndex) 2 * currentIndex + 2
#define PARENT(currentIndex) (int)floor((currentIndex - 1) / 2)

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

int 
heuristic_cost_estimate(int currentX, int currentY, int goalX, int goalY, int avoidWalls)
{
    int xDistance = abs(currentX - goalX);
    int yDistance = abs(currentY - goalY);
    
    int hScore = (ORTOGONAL * (xDistance + yDistance)) + ((DIAGONAL - (2 * ORTOGONAL)) * ((xDistance > yDistance) ? yDistance : xDistance));
    
    if (avoidWalls) {
        hScore += (((xDistance > yDistance) ? xDistance : yDistance) * 10);
    }
    
    return hScore;
}

//Openlist is a binary heap of min-heap type

void 
openListAdd (CalcPath_session *session, Node* infoAdress)
{
	int currentIndex = session->openListSize;
    session->openList[currentIndex].x = infoAdress->x;
    session->openList[currentIndex].y = infoAdress->y;
    session->openList[currentIndex].f = infoAdress->f;
    infoAdress->openListIndex = currentIndex;
    TypeList Temporary;
    while (PARENT(currentIndex) >= 0) {
        if (session->openList[PARENT(currentIndex)].f > session->openList[currentIndex].f) {
            Temporary = session->openList[currentIndex];
            session->openList[currentIndex] = session->openList[PARENT(currentIndex)];
            session->currentMap[(session->openList[currentIndex].y * session->width) + session->openList[currentIndex].x].openListIndex = currentIndex;
            session->openList[PARENT(currentIndex)] = Temporary;
            infoAdress->openListIndex = PARENT(currentIndex);
            currentIndex = PARENT(currentIndex);
        } else { break; }
    }
}

void 
reajustOpenListItem (CalcPath_session *session, Node* infoAdress)
{
    int currentIndex = infoAdress->openListIndex;
    session->openList[currentIndex].f = infoAdress->f;
    TypeList Temporary;
    while (PARENT(currentIndex) >= 0) {
        if (session->openList[PARENT(currentIndex)].f > session->openList[currentIndex].f) {
            Temporary = session->openList[currentIndex];
            session->openList[currentIndex] = session->openList[PARENT(currentIndex)];
            session->currentMap[(session->openList[currentIndex].y * session->width) + session->openList[currentIndex].x].openListIndex = currentIndex;
            session->openList[PARENT(currentIndex)] = Temporary;
            infoAdress->openListIndex = PARENT(currentIndex);
            currentIndex = PARENT(currentIndex);
        } else { break; }
    }
}

Node* 
openListGetLowest (CalcPath_session *session)
{
    Node* lowestNode = &session->currentMap[(session->openList[0].y * session->width) + session->openList[0].x];
    session->openList[0] = session->openList[session->openListSize-1];
    session->currentMap[(session->openList[0].y * session->width) + session->openList[0].x].openListIndex = 0;
    int lowestChildIndex = 0;
    int currentIndex = 0;
    TypeList Temporary;
    while (LCHILD(currentIndex) < session->openListSize - 2) {
        //There are 2 children
        if (RCHILD(currentIndex) <= session->openListSize - 2) {
            if (session->openList[RCHILD(currentIndex)].f <= session->openList[LCHILD(currentIndex)].f) {
                lowestChildIndex = RCHILD(currentIndex);
            } else {
                lowestChildIndex = LCHILD(currentIndex);
            }
        } else {
            //There is 1 children
            if (LCHILD(currentIndex) <= session->openListSize - 2) {
                lowestChildIndex = LCHILD(currentIndex);
            } else {
                break;
            }
        }
        if (session->openList[currentIndex].f > session->openList[lowestChildIndex].f) {
            Temporary = session->openList[currentIndex];
            session->openList[currentIndex] = session->openList[lowestChildIndex];
            session->currentMap[(session->openList[currentIndex].y * session->width) + session->openList[currentIndex].x].openListIndex = currentIndex;
            session->openList[lowestChildIndex] = Temporary;
            session->currentMap[(session->openList[lowestChildIndex].y * session->width) + session->openList[lowestChildIndex].x].openListIndex = lowestChildIndex;
            currentIndex = lowestChildIndex;
        } else { break; }
    }
    return lowestNode;
}

void 
reconstruct_path(CalcPath_session *session, Node* currentNode)
{
	while (currentNode->x != session->startX || currentNode->y != session->startY)
    {
        session->currentMap[(currentNode->parentY * session->width) + currentNode->parentX].whichlist = PATH;
        currentNode = &session->currentMap[(currentNode->parentY * session->width) + currentNode->parentX];
        session->solution_size++;
    }
}

int 
CalcPath_pathStep (CalcPath_session *session)
{
	
	if (!session->initialized) {
		return -2;
	}
	
	if (!session->run) {
		session->run = 1;
		session->solution_size = 0;
		session->size = session->height * session->width;
		session->openListSize = 1;
		session->openList = (TypeList*) malloc(session->size * sizeof(TypeList));
		session->openList[0].x = session->startX;
		session->openList[0].y = session->startY;
	}
	
	Node* currentNode;
	Node* infoAdress;
	unsigned int Gscore = 0;
	int indexNeighbor = 0;
	int nodeList;
	
	int next = 0;
	
	unsigned long timeout = (unsigned long) GetTickCount();
	int loop = 0;
    while (session->openListSize > 0) {
		
		loop++;
		if (loop == 100) {
			if (GetTickCount() - timeout > session->time_max) {
				return 0;
			} else
				loop = 0;
		}
		
        //get lowest F score member of openlist and delete it from it
		if (next > 0) {
			currentNode = &session->currentMap[next];
			next = 0;
		} else {
			currentNode = openListGetLowest (session);
		}
		
        session->openListSize--;

        //add currentNode to closedList
        currentNode->whichlist = CLOSED;

		//if current is the goal, return the path.
		if (currentNode->x == session->endX && currentNode->y == session->endY) {
            //return path
            reconstruct_path(session, currentNode);
			return 1;
		}
		
		int i;
		for (i = -1; i <= 1; i++)
		{
			int j;
			for (j = -1; j <= 1; j++)
			{
				if (i == 0 && j == 0){ continue; }
				unsigned int x = currentNode->x + i;
				unsigned int y = currentNode->y + j;
				
				if (x >= session->width || y >= session->height || x < 0 || y < 0){ continue; }
				
				int current = (y * session->width) + x;
				
				if (session->map[current] == 0){ continue; }
				
				infoAdress = &session->currentMap[current];
				
				if (infoAdress->whichlist == CLOSED) { continue; }
				
				int distanceFromCurrent;
				if (i != 0 && j != 0) {
				   if (session->map[(currentNode->y * session->width) + x] == 0 || session->map[(y * session->width) + currentNode->x] == 0){ continue; }
					distanceFromCurrent = DIAGONAL;
				} else {
					distanceFromCurrent = ORTOGONAL;
				}
				if (session->avoidWalls) {
					distanceFromCurrent += session->map[current];
				}
				
				Gscore = currentNode->g + distanceFromCurrent;
				
				if (infoAdress->whichlist == NONE) {
					infoAdress->x = x;
					infoAdress->y = y;
					infoAdress->parentX = currentNode->x;
					infoAdress->parentY = currentNode->y;
					infoAdress->g = Gscore;
					infoAdress->h = heuristic_cost_estimate(infoAdress->x, infoAdress->y, session->endX, session->endY, session->avoidWalls);
					infoAdress->f = infoAdress->g + infoAdress->h;
					if (next == 0 && infoAdress->f == currentNode->f) {
						infoAdress->whichlist = CLOSED;
						next = current;
					} else {
						infoAdress->whichlist = OPEN;
						openListAdd (session, infoAdress);
						session->openListSize++;
					}
				} else {
					if (Gscore < infoAdress->g) {
						infoAdress->parentX = currentNode->x;
						infoAdress->parentY = currentNode->y;
						infoAdress->g = Gscore;
						infoAdress->f = infoAdress->g + infoAdress->h;
						reajustOpenListItem (session, infoAdress);
					}
				}
			}
		}
	}
	return -1;
}

// Create a new pathfinding session, or reset an existing session.
// Resetting is preferred over destroying and creating, because it saves
// unnecessary memory allocations, thus improving performance.
CalcPath_session *
CalcPath_init (CalcPath_session *session)
{
	session->currentMap[(session->startY * session->width) + session->startX].x = session->startX;
	session->currentMap[(session->startY * session->width) + session->startX].y = session->startY;
	session->currentMap[(session->startY * session->width) + session->startX].g = 0;
	session->currentMap[(session->endY * session->width) + session->endX].x = session->endX;
	session->currentMap[(session->endY * session->width) + session->endX].y = session->endY;
	
	session->initialized = 1;
	
	return session;
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

	free (session);

}

#ifdef __cplusplus
}
#endif /* __cplusplus */
