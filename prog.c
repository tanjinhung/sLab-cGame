#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#define QUEUE_SIZE 1024
#define NODE_NUM 14
#define POS_SIZE 5
#define MAX_PIECES 4
#define MAX_NEIGHBORS 3

typedef struct Node
{
    char label;
    bool color;
    bool occupied;
    struct Node *neighbors[3]; // Pointers to adjacent nodes
} Node;

Node *createNode(char label)
{
    Node *newNode = (Node *)malloc(sizeof(Node));
    if (newNode == NULL)
    {
        perror("Failed to allocate memory for node");
        exit(1);
    }
    newNode->label = label;
    newNode->color = 0; // Default to white
    newNode->occupied = false;
    newNode->neighbors[0] = NULL;
    newNode->neighbors[1] = NULL;
    newNode->neighbors[2] = NULL;
    return newNode;
}

// --- Hash Table ---
#define TABLE_SIZE 1000 // You may need to adjust this size

typedef struct HashEntry
{
    unsigned long key;      // Hash of the board state
    char *whitePos;         // to store white pieces position
    char *blackPos;         // to store black pieces position
    struct HashEntry *next; // For collision handling (chaining)
} HashEntry;

HashEntry *hashTable[TABLE_SIZE]; // The hash table
// --- End of Hash Table ---

unsigned long hashBoardState(Node *board[], int nodeNum)
{
    unsigned long hash = 0;
    // Use base 3 since each node can have 3 different states.
    for (int i = 0; i < nodeNum; i++)
    {
        unsigned long cellValue = 0;
        if (board[i]->occupied)
        {
            // White piece is represented by 1, Black by 2.
            cellValue = (board[i]->color == 0) ? 1UL : 2UL;
        }
        hash = hash * 3UL + cellValue;
    }
    return hash;
}

// Function to initialize the hash table
void initHashTable()
{
    for (int i = 0; i < TABLE_SIZE; i++)
    {
        hashTable[i] = NULL;
    }
}

// Function to insert a board state into the hash table
void insertBoardState(unsigned long key, const char *whitePos,
                      const char *blackPos)
{
    int index = key % TABLE_SIZE;
    HashEntry *newEntry = (HashEntry *)malloc(sizeof(HashEntry));
    newEntry->key = key;
    newEntry->whitePos = strdup(whitePos); // copy the string
    newEntry->blackPos = strdup(blackPos); // copy the string
    newEntry->next = NULL;

    // Handle collision (chaining)
    if (hashTable[index] == NULL)
    {
        hashTable[index] = newEntry;
    }
    else
    {
        HashEntry *current = hashTable[index];
        while (current->next != NULL)
        {
            current = current->next;
        }
        current->next = newEntry;
    }
}

// Function to lookup a board state in the hash table
bool lookupBoardState(unsigned long key, const char *whitePos,
                      const char *blackPos)
{
    int index = key % TABLE_SIZE;
    HashEntry *current = hashTable[index];

    while (current != NULL)
    {
        if (current->key == key && strcmp(current->whitePos, whitePos) == 0 &&
            strcmp(current->blackPos, blackPos) == 0)
        {
            return true; // Board state found
        }
        current = current->next;
    }
    return false; // Board state not found
}

// Function to free the memory allocated for the hash table
void freeHashTable()
{
    for (int i = 0; i < TABLE_SIZE; i++)
    {
        HashEntry *current = hashTable[i];
        while (current != NULL)
        {
            HashEntry *temp = current;
            current = current->next;
            free(temp->whitePos);
            free(temp->blackPos);
            free(temp);
        }
        hashTable[i] = NULL;
    }
}

// --- Queue Implementation ---

typedef struct QueueData
{
    char whitePos[5];
    char blackPos[5];
    char move[3];
} QueueData;

typedef struct
{
    QueueData data[QUEUE_SIZE]; // From and to node
    int head;
    int tail;
    int size;
    int predecessor[QUEUE_SIZE]; // Predecessor of each node
} StringQueue;

void initQueue(StringQueue *queue)
{
    queue->head = 0;
    queue->tail = 0;
    queue->size = 0;
    for (int i = 0; i < QUEUE_SIZE; i++)
    {
        queue->predecessor[i] = -1;
    }
}

bool isQueueEmpty(StringQueue *queue)
{
    return queue->size == 0;
}

bool enqueue(StringQueue *queue, const char *whitePos, const char *blackPos, const char *move, int pred)
{
    if (queue->size == QUEUE_SIZE)
    {
        return false; // Queue full
    }
    // strcpy(queue->data[queue->tail], value);
    strncpy(queue->data[queue->tail].whitePos, whitePos, 4);
    queue->data[queue->tail].whitePos[4] = '\0'; // Ensure null-termination
    strncpy(queue->data[queue->tail].blackPos, blackPos, 4);
    queue->data[queue->tail].blackPos[4] = '\0'; // Ensure null-termination
    strncpy(queue->data[queue->tail].move, move, 2);
    queue->data[queue->tail].move[2] = '\0'; // Ensure null-termination
    queue->predecessor[queue->tail] = pred;  // Set predecessor
    queue->tail = (queue->tail + 1) % QUEUE_SIZE;
    queue->size++;
    return true;
}

QueueData dequeue(StringQueue *queue)
{
    QueueData emptyData = {"", "", ""};
    if (isQueueEmpty(queue))
    {
        return emptyData; // Queue empty
    }
    QueueData value = queue->data[queue->head];
    queue->head = (queue->head + 1) % QUEUE_SIZE;
    queue->size--;
    return value;
}

void printQueue(StringQueue *queue)
{
    printf("Queue: ");
    for (int i = 0; i < queue->size; i++)
    {
        printf("White: %s, Black: %s, Move: %s | ",
               queue->data[(queue->head + i) % QUEUE_SIZE].whitePos,
               queue->data[(queue->head + i) % QUEUE_SIZE].blackPos,
               queue->data[(queue->head + i) % QUEUE_SIZE].move);
    }
    printf("\n");
    // print predecessor
    printf("Predecessors: ");
    for (int i = 0; i < queue->size; i++)
    {
        printf("%d ", queue->predecessor[(queue->head + i) % QUEUE_SIZE]);
    }
    printf("\n");
}
// --- End of Queue Implementation ---

void printBoard(Node *board[], int nodeNum)
{
    printf("\nBoard:\n");
    for (int i = 0; i < nodeNum; i++)
    {
        printf("%c: %s %s\n", board[i]->label, board[i]->color ? "Black" : "White", board[i]->occupied ? "Occupied" : "Empty");
    }
    printf("\n");
}

bool isGoalState(Node *board[], const char *goalWhitePos, const char *goalBlackPos)
{
    printf("Checking goal state...\n");
    // Get current positions of white and black pieces from the board
    char currentWhitePos[NODE_NUM + 1] = {0};
    char currentBlackPos[NODE_NUM + 1] = {0};
    int whiteIndex = 0, blackIndex = 0;
    for (int i = 0; i < NODE_NUM; i++)
    {
        if (board[i]->occupied)
        {
            if (board[i]->color == 0) // White
            {
                currentWhitePos[whiteIndex++] = board[i]->label;
            }
            else // Black
            {
                currentBlackPos[blackIndex++] = board[i]->label;
            }
        }
    }
    currentWhitePos[whiteIndex] = '\0';
    currentBlackPos[blackIndex] = '\0';
    // printf("Current White: %s, Current Black: %s\n", currentWhitePos, currentBlackPos);

    // printf("Goal White: %s, Goal Black: %s\n", goalWhitePos, goalBlackPos);
    // Check if the current state matches the goal state
    if (strcmp(currentWhitePos, goalWhitePos) == 0 && strcmp(currentBlackPos, goalBlackPos) == 0)
    {
        return true;
    }
    return false;
}

void setBoardState(Node *board[], const char *whiteNewPos, const char *blackNewPos)
{
    for (int i = 0; i < NODE_NUM; i++)
    {
        board[i]->occupied = false;
        board[i]->color = 0; // Default to white
    }

    for (int i = 0; whiteNewPos[i] != '\0'; i++)
    {
        for (int j = 0; j < NODE_NUM; j++)
        {
            if (board[j]->label == whiteNewPos[i])
            {
                board[j]->occupied = true;
                board[j]->color = 0; // White
                break;
            }
        }
    }
    for (int i = 0; blackNewPos[i] != '\0'; i++)
    {
        for (int j = 0; j < NODE_NUM; j++)
        {
            if (board[j]->label == blackNewPos[i])
            {
                board[j]->occupied = true;
                board[j]->color = 1; // Black
                break;
            }
        }
    }
}
char *getPiecesPosition(Node *board[])
{
    char *piecesPosition = (char *)malloc(2 * POS_SIZE);

    if (piecesPosition == NULL)
    {
        perror("Memory allocation failed");
        return NULL;
    }

    char *whitePieces = piecesPosition;            // Start of the white piece string
    char *blackPieces = piecesPosition + POS_SIZE; // Start of the black piece string

    for (int i = 0; i < POS_SIZE; i++)
    {
        whitePieces[i] = 0;
        blackPieces[i] = 0;
    }

    int whiteIndex = 0, blackIndex = 0;
    for (int i = 0; i < NODE_NUM; i++)
    {
        if (board[i]->occupied)
        {
            if (board[i]->color == 0) // White
            {
                whitePieces[whiteIndex++] = board[i]->label;
            }
            else // Black
            {
                blackPieces[blackIndex++] = board[i]->label;
            }
        }
    }
    whitePieces[whiteIndex] = '\0';
    blackPieces[blackIndex] = '\0';
    printf("Current White: %s, Current Black: %s\n", whitePieces, blackPieces);
    return piecesPosition;
}



void generateNextStates(Node *board[], StringQueue *queue, int currentIteration)
{
    // Generate next states based on the current board state
    for (int i = 0; i < NODE_NUM; i++)
    {
        if (board[i]->occupied)
        {
            for (int j = 0; j < 3 && board[i]->neighbors[j] != NULL; j++)
            {
                // If neighbor is empty, we can move there
                if (!board[i]->neighbors[j]->occupied)
                {
                    char move[3] = {board[i]->label, board[i]->neighbors[j]->label, '\0'};
                    char *piecesPosition = getPiecesPosition(board);

                    char *whitePieces = piecesPosition;
                    char *blackPieces = piecesPosition + POS_SIZE; // Start of the black piece string

                    enqueue(queue, whitePieces, blackPieces, move, currentIteration);
                    free(piecesPosition);
                }
            }
        }
    }
}

void printPath(StringQueue *queue, int goalIndex, Node *board[])
{
    int path[QUEUE_SIZE];
    int pathIndex = 0;
    int currentIndex = goalIndex;

    // Reconstruct the path by traversing predecessors
    while (currentIndex != -1)
    {
        path[pathIndex++] = currentIndex;
        printf("Current Index: %d\n", currentIndex);
        currentIndex = queue->predecessor[currentIndex];
    }

    printf("Path to goal state:\n");

    // Iterate through the path in reverse to print the steps in the correct order
    for (int i = pathIndex - 1; i >= 0; i--)
    {
        int moveIndex = path[i];
        QueueData state = queue->data[moveIndex];

        printf("Step %d: Move: %s\n", pathIndex - i, state.move);

        setBoardState(board, state.whitePos, state.blackPos);
        // printBoard(board, &board->label);
    }
}

int main(int argc, char *argv[])
{
    //       H
    //       I
    //       J
    //       K
    // A B C L M N
    //     D
    //     E
    //     F
    //     G

    // Check for correct number of arguments
    if (argc != 5)
    {
        fprintf(stderr, "Usage: %s <white pieces> <black pieces> <white end> <black end>\n", argv[0]);
        return 1;
    }

    Node *board[NODE_NUM];
    for (int i = 0; i < NODE_NUM; i++)
    {
        board[i] = createNode('A' + i);
    }

    // --- Define Neighbors ---
    board[0]->neighbors[0] = board[1];   // A neighbor is B
    board[1]->neighbors[0] = board[0];   // B neighbor is A
    board[1]->neighbors[1] = board[2];   // B neighbor is C
    board[2]->neighbors[0] = board[1];   // C neighbor is B
    board[2]->neighbors[1] = board[3];   // C neighbor is D
    board[2]->neighbors[2] = board[11];  // C neighbor is L
    board[3]->neighbors[0] = board[2];   // D neighbor is C
    board[3]->neighbors[1] = board[4];   // D neighbor is E
    board[4]->neighbors[0] = board[3];   // E neighbor is D
    board[4]->neighbors[1] = board[5];   // E neighbor is F
    board[5]->neighbors[0] = board[4];   // F neighbor is E
    board[5]->neighbors[1] = board[6];   // F neighbor is G
    board[6]->neighbors[0] = board[5];   // G neighbor is F
    board[7]->neighbors[0] = board[8];   // H neighbor is I
    board[8]->neighbors[0] = board[7];   // I neighbor is H
    board[8]->neighbors[1] = board[9];   // I neighbor is J
    board[9]->neighbors[0] = board[8];   // J neighbor is I
    board[9]->neighbors[1] = board[10];  // J neighbor is K
    board[10]->neighbors[0] = board[9];  // K neighbor is J
    board[10]->neighbors[1] = board[11]; // K neighbor is L
    board[11]->neighbors[0] = board[2];  // L neighbor is C
    board[11]->neighbors[1] = board[10]; // L neighbor is K
    board[11]->neighbors[2] = board[12]; // L neighbor is M
    board[12]->neighbors[0] = board[11]; // M neighbor is L
    board[12]->neighbors[1] = board[13]; // M neighbor is N
    board[13]->neighbors[0] = board[12]; // N neighbor is M
    // --- End of Neighbors Definition ---

    StringQueue queue;
    initQueue(&queue);
    initHashTable();

    char *whitePieces = strdup(argv[1]);
    char *blackPieces = strdup(argv[2]);

    enqueue(&queue, whitePieces, blackPieces, "", -1);
    setBoardState(board, whitePieces, blackPieces);
    printf("Initial Positions: White: %s, Black: %s\n", whitePieces, blackPieces);
    int iteration = 0;

    printBoard(board, NODE_NUM);
    // Generate current board hash
    unsigned long currentHash = hashBoardState(board, NODE_NUM);
    printf("Current Hash: %lu\n", currentHash);
    // Check if the current state is already in the hash table
    if (lookupBoardState(currentHash, whitePieces, blackPieces))
    {
        printf("State already visited.\n");
        return 0;
    }
    // Insert the current state into the hash table
    insertBoardState(currentHash, whitePieces, blackPieces);
    printf("State inserted into hash table.\n");

    generateNextStates(board, &queue, iteration);
    // printQueue(&queue);

    int i = 0;

    while (!isQueueEmpty(&queue))
    {
        printf("\n---------------\nIteration: %d\n", iteration);
        i++;
        // if (i > 500)
        // {
        //     printf("Too many iterations, exiting...\n");
        //     break;
        // }

        iteration++;

        QueueData state = dequeue(&queue);
        printf("Dequeue: %s %s|%s\n", state.whitePos, state.blackPos, state.move);

        char prevPos = state.move[0];
        char newPos = state.move[1];
        printf("Moving from %c to %c\n", prevPos, newPos);

        setBoardState(board, state.whitePos, state.blackPos);

        Node *prevNode = NULL;
        Node *newNode = NULL;
        // Find the previous and new nodes
        for (int i = 0; i < NODE_NUM; i++)
        {
            if (board[i]->label == prevPos)
            {
                prevNode = board[i];
            }
            if (board[i]->label == newPos)
            {
                newNode = board[i];
            }
        }
        if (prevNode == NULL || !prevNode->occupied)
        {
            printf("[Skipping]: No piece found at %c\n", prevPos);

            continue;
        }
        if (newNode == NULL)
        {
            printf("[Skipping]: Invalid move to %c\n", newPos);
            continue;
        }
        if (newNode->occupied)
        {
            printf("[Skipping]: %c is already occupied\n", newPos);
            continue;
        }

        bool validMove = false;
        for (int j = 0; j < 3; j++)
        {
            if (prevNode->neighbors[j] != NULL && prevNode->neighbors[j]->label == newPos)
            {
                validMove = true;
                break;
            }
        }
        if (!validMove)
        {
            printf("[Skipping]: Invalid move from %c to %c\n", prevPos, newPos);
            continue;
        }
        // Set the new position
        for (int i = 0; i < NODE_NUM; i++)
        {
            if (board[i]->label == prevPos)
            {
                board[i]->occupied = false;
            }
            if (board[i]->label == newPos)
            {
                board[i]->occupied = true;
                board[i]->color = prevNode->color; // Set the color based on the piece
            }
        }

        char *newWhitePos = (char *)malloc(NODE_NUM + 1);
        char *newBlackPos = (char *)malloc(NODE_NUM + 1);
        int whiteIndex = 0, blackIndex = 0;
        for (int i = 0; i < NODE_NUM; i++)
        {
            if (board[i]->occupied)
            {
                if (board[i]->color == 0) // White
                {
                    newWhitePos[whiteIndex++] = board[i]->label;
                }
                else // Black
                {
                    newBlackPos[blackIndex++] = board[i]->label;
                }
            }
        }
        newWhitePos[whiteIndex] = '\0';
        newBlackPos[blackIndex] = '\0';

        printBoard(board, NODE_NUM);

        unsigned long newHash = hashBoardState(board, NODE_NUM);
        printf("New Hash: %lu\n", newHash);
        // Check if the new state is already in the hash table
        if (lookupBoardState(newHash, newWhitePos, newBlackPos))
        {
            printf("[Skipping]: State already visited.\n");
            free(newWhitePos);
            free(newBlackPos);
            continue; // Skip to the next iteration
        }

        // Insert the new state into the hash table
        insertBoardState(newHash, newWhitePos, newBlackPos);
        printf("New state inserted into hash table.\n");
        // Check if the goal state is reached
        if (isGoalState(board, argv[3], argv[4]))
        {
            printf("Goal state reached!\n");

            printf("Final White: %s, Final Black: %s\n", newWhitePos, newBlackPos);
            printf("Final Hash: %lu\n", newHash);
            printf("Path to goal state:\n");
            printf("Step %d: Move: %s\n", iteration - 1, state.move);
            printf("Queue Size: %d\n", queue.size);

            printf("Constructing path...\n");
            
            printPath(&queue, iteration - 1, board);
            // printQueue(&queue);

            free(newWhitePos);
            free(newBlackPos);
            break;
        }

        // Generate next states
        generateNextStates(board, &queue, iteration);
        // printQueue(&queue);

        whitePieces = strdup(newWhitePos);
        blackPieces = strdup(newBlackPos);
        free(newWhitePos);
        free(newBlackPos);
        // sleep(1); // Sleep for 0.5 seconds
    }

    // Free the allocated memory
    for (int i = 0; i < NODE_NUM; i++)
    {
        free(board[i]);
    }

    freeHashTable();

    return 0;
}
