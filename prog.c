#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>

#define QUEUE_SIZE 300000
#define NODE_NUM 14
// #define POS_SIZE 5
// #define MAX_PIECES 4
#define MAX_NEIGHBORS 3
#define SIMPLE

#pragma region Node
//  --- Node Structure ---
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
// --- End of Node Structure ---
#pragma endregion

#pragma region Helper Function Prototypes
void printBoard(Node *board[]);
bool isGoalState(Node *board[], const char *goalWhitePos, const char *goalBlackPos);
void setBoardState(Node *board[], const char *whiteNewPos, const char *blackNewPos);
char *getPiecesPosition(Node *board[]);
#pragma endregion

#pragma region Hash Table
// --- Hash Table ---
#define TABLE_SIZE 1000000

typedef struct HashEntry
{
    unsigned long key;      // Hash of the board state
    char *whitePos;         // to store white pieces position
    char *blackPos;         // to store black pieces position
    struct HashEntry *next; // For collision handling (chaining)
} HashEntry;

HashEntry *hashTable[TABLE_SIZE]; // The hash table

unsigned long hashBoardState(Node *board[], int nodeNum)
{
    unsigned long hash = 0;
    unsigned long prime = 53; // Or any other suitable prime number
    for (int i = 0; i < nodeNum; i++)
    {
        unsigned long cellValue = 0;
        if (board[i]->occupied)
        {
            cellValue = (board[i]->color == 0) ? 1UL : 2UL;
        }
        hash = hash * prime + cellValue;
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
    if (newEntry == NULL)
    {
        perror("Failed to allocate memory for hash entry");
        return;
    }
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
// --- End of Hash Table ---
#pragma endregion

#pragma region Queue
// --- Queue Implementation ---
typedef struct QueueData
{
    char whitePos[NODE_NUM + 1];
    char blackPos[NODE_NUM + 1];
    char move[3];
    int predecessor;
} QueueData;

typedef struct QueueNode
{
    QueueData data;
    struct QueueNode *next;
} QueueNode;

typedef struct DynamicLinkedQueue
{
    QueueNode *head;
    QueueNode *tail;
    int size;
} DynamicLinkedQueue;

void initQueue(DynamicLinkedQueue *queue)
{
    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;
}

bool isQueueEmpty(DynamicLinkedQueue *queue)
{
    return queue->head == NULL;
}

bool enqueueQueue(DynamicLinkedQueue *queue, Node *board[], const char *whitePos, const char *blackPos,
                  const char *move, int currentIteration, int whiteLen, int blackLen)
{
    QueueNode *newNode = (QueueNode *)malloc(sizeof(QueueNode));
    if (newNode == NULL)
    {
        perror("Failed to allocate memory for queue node");
        return false;
    }
    
    strncpy(newNode->data.whitePos, whitePos, whiteLen);
    newNode->data.whitePos[whiteLen] = '\0'; // Ensure null-termination
    strncpy(newNode->data.blackPos, blackPos, blackLen);
    newNode->data.blackPos[blackLen] = '\0'; // Ensure null-termination
    strncpy(newNode->data.move, move, 2);
    newNode->data.move[2] = '\0'; // Ensure null-termination
    newNode->data.predecessor = currentIteration;
    newNode->next = NULL;

    if (isQueueEmpty(queue))
    {
        queue->head = newNode;
        queue->tail = newNode;
    }
    else
    {
        queue->tail->next = newNode;
        queue->tail = newNode;
    }
    queue->size++;

    return true;
}

QueueData dequeueQueue(DynamicLinkedQueue *queue)
{
    QueueData emptyData = {"", "", "", -1};
    if (isQueueEmpty(queue))
    {
        return emptyData; // Queue empty
    }
    QueueNode *temp = queue->head;
    QueueData data = temp->data;
    queue->head = queue->head->next;
    if (isQueueEmpty(queue))
    {
        queue->tail = NULL; // Reset tail if the queue is empty
    }
    free(temp);
    queue->size--;
    return data;
}

void printQueue(DynamicLinkedQueue *queue)
{
    printf("Queue: ");
    QueueNode *current = queue->head;
    while (current != NULL)
    {
        printf("White: %s, Black: %s, Move: %s\n ",
               current->data.whitePos,
               current->data.blackPos,
               current->data.move);
        current = current->next;
    }
    printf("End of Queue\n");
}

void freeQueue(DynamicLinkedQueue *queue)
{
    QueueNode *current = queue->head;
    while (current != NULL)
    {
        QueueNode *temp = current;
        current = current->next;
        free(temp);
    }
    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;
}

typedef struct Predecessor
{
    char move[3];
    int predecessor;
} Predecessor;

void reconstructPath(Predecessor *predecessors, int goalIndex, int path[], int *pathLength)
{
    if (predecessors == NULL || path == NULL || pathLength == NULL)
    {
        fprintf(stderr, "[Error]: Null pointer passed to reconstructPath.\n");
        return;
    }
    // Initialize path length

    // Reconstruct the path from the goal index to the start
    int currentIndex = goalIndex;
    int index = 0;
    while (currentIndex != -1)
    {
        path[index] = currentIndex;
        currentIndex = predecessors[currentIndex].predecessor;
        index++;
    }

    *pathLength = index;

    // Reverse the path to get it from start to goal
    for (int i = 0; i < *pathLength / 2; i++)
    {
        int temp = path[i];
        path[i] = path[*pathLength - 1 - i];
        path[*pathLength - 1 - i] = temp;
    }

    // Print the reconstructed path
    printf("Reconstructed Path:\n");
    for (int i = 0; i < *pathLength; i++)
    {
#ifdef SIMPLE
        printf("%s\n", predecessors[path[i]].move);
#else
        printf("Move: %s, Predecessor: %d\n", predecessors[path[i]].move, predecessors[path[i]].predecessor);
#endif // !SIMPLE
    }
}

void printBoardForSolution(Node *board[], const char *whitePos, const char *blackPos, int path[], int pathLength, Predecessor *predecessors)
{
    setBoardState(board, whitePos, blackPos);

    printf("Path to solution:\n");
    for (int i = 0; i < pathLength; i++)
    {
        int index = path[i];
        char prevPos = predecessors[index].move[0];
        char newPos = predecessors[index].move[1];

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

        printBoard(board);
        // printf("Move: %s, Predecessor: %d\n", predecessors[index].move, predecessors[index].predecessor);
    }
}

#pragma endregion

#pragma region Helper Functions
void printBoard(Node *board[])
{
    printf("\nBoard:\n");
    for (int i = 0; i < NODE_NUM; i++)
    {
        printf("%c: %s\n", board[i]->label, board[i]->occupied ? board[i]->color ? "Black" : "White" : "");
    }
    printf("\n");
}

bool isGoalState(Node *board[], const char *goalWhitePos, const char *goalBlackPos)
{
    // printf("Checking goal state...\n");
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
    // printf("Setting board state...\n");

    // 1. Reset the board state
    for (int i = 0; i < NODE_NUM; i++)
    {
        board[i]->occupied = false;
        board[i]->color = 0; // Default to white
    }

    // 2. Set the white pieces
    for (int i = 0; whiteNewPos[i] != '\0'; i++)
    {
        bool found = false;
        for (int j = 0; j < NODE_NUM; j++)
        {
            if (board[j]->label == whiteNewPos[i])
            {
                board[j]->occupied = true;
                board[j]->color = 0; // White
                found = true;
                break; // Exit inner loop once found
            }
        }
        if (!found)
        {
            fprintf(stderr, "Warning: Invalid white piece label: %c\n",
                    whiteNewPos[i]); // Report the invalid label
        }
    }

    // 3. Set the black pieces (similar to white pieces)
    for (int i = 0; blackNewPos[i] != '\0'; i++)
    {
        bool found = false;
        for (int j = 0; j < NODE_NUM; j++)
        {
            if (board[j]->label == blackNewPos[i])
            {
                board[j]->occupied = true;
                board[j]->color = 1; // Black
                found = true;
                break;
            }
        }
        if (!found)
        {
            fprintf(stderr, "Warning: Invalid black piece label: %c\n",
                    blackNewPos[i]); // Report the invalid label
        }
    }

    // printf("Board state set.\n");
}

void generateNextStates(Node *board[], DynamicLinkedQueue *queue, int currentIteration)
{
    // Get the current positions of white and black pieces from the board
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

    // Generate next states based on the current board state
    for (int i = 0; i < NODE_NUM; i++)
    {
        if (board[i]->occupied)
        {
            for (int j = 0; j < MAX_NEIGHBORS && board[i]->neighbors[j] != NULL; j++)
            {
                // If neighbor is empty, we can move there
                if (!board[i]->neighbors[j]->occupied)
                {
                    char move[3] = {board[i]->label, board[i]->neighbors[j]->label, '\0'};

                    enqueueQueue(queue, board, currentWhitePos, currentBlackPos, move, currentIteration,
                                 strlen(currentWhitePos), strlen(currentBlackPos));
                }
            }
        }
    }
}

#pragma endregion

int main(int argc, char *argv[])
{
    clock_t start, end;
    double cpu_time_used;

#pragma region Error Handling
    // Check for correct number of arguments
    if (argc != 5)
    {
        fprintf(stderr, "Usage: %s <white pieces> <black pieces> <white end> <black end>\n", argv[0]);
        return 1;
    }
    // Check for same starting and ending length
    if (strlen(argv[1]) != strlen(argv[3]) || strlen(argv[2]) != strlen(argv[4]))
    {
        fprintf(stderr, "Error: Starting and ending positions must have the same number of pieces.\n");
        return 1;
    }
    // Check for valid piece positions and ensure they are unique
    for (int i = 0; i < strlen(argv[1]); i++)
    {
        if (strchr(argv[1], argv[1][i]) != strrchr(argv[1], argv[1][i]))
        {
            fprintf(stderr, "Error: Duplicate piece positions in white pieces.\n");
            return 1;
        }
    }
    for (int i = 0; i < strlen(argv[2]); i++)
    {
        if (strchr(argv[2], argv[2][i]) != strrchr(argv[2], argv[2][i]))
        {
            fprintf(stderr, "Error: Duplicate piece positions in black pieces.\n");
            return 1;
        }
    }
    for (int i = 0; i < strlen(argv[3]); i++)
    {
        if (strchr(argv[3], argv[3][i]) != strrchr(argv[3], argv[3][i]))
        {
            fprintf(stderr, "Error: Duplicate goal positions in white pieces.\n");
            return 1;
        }
    }
    for (int i = 0; i < strlen(argv[4]); i++)
    {
        if (strchr(argv[4], argv[4][i]) != strrchr(argv[4], argv[4][i]))
        {
            fprintf(stderr, "Error: Duplicate goal positions in black pieces.\n");
            return 1;
        }
    }
    // Check for valid piece positions
    for (int i = 0; i < strlen(argv[1]); i++)
    {
        if (argv[1][i] < 'A' || argv[1][i] > 'N')
        {
            fprintf(stderr, "Error: Invalid piece position in white pieces: %c\n", argv[1][i]);
            return 1;
        }
    }
    for (int i = 0; i < strlen(argv[2]); i++)
    {
        if (argv[2][i] < 'A' || argv[2][i] > 'N')
        {
            fprintf(stderr, "Error: Invalid piece position in black pieces: %c\n", argv[2][i]);
            return 1;
        }
    }
    // Check for valid goal positions
    for (int i = 0; i < strlen(argv[3]); i++)
    {
        if (argv[3][i] < 'A' || argv[3][i] > 'N')
        {
            fprintf(stderr, "Error: Invalid goal position in white pieces: %c\n", argv[3][i]);
            return 1;
        }
    }
    for (int i = 0; i < strlen(argv[4]); i++)
    {
        if (argv[4][i] < 'A' || argv[4][i] > 'N')
        {
            fprintf(stderr, "Error: Invalid goal position in black pieces: %c\n", argv[4][i]);
            return 1;
        }
    }
#pragma endregion

#pragma region Node Initialization
    // --- Initialize Nodes ---
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

    DynamicLinkedQueue queue;
    initQueue(&queue);
    initHashTable();
    Predecessor predecessors[QUEUE_SIZE];
    int goalState = -1;
#pragma endregion

    // printf("Starting positions: White: %s, Black: %s\n", argv[1], argv[2]);
    char *whitePieces = strdup(argv[1]);
    char *blackPieces = strdup(argv[2]);
    int whitePiecesLength = strlen(whitePieces);
    int blackPiecesLength = strlen(blackPieces);

    setBoardState(board, whitePieces, blackPieces);
    // printf("Initial Positions: White: %s, Black: %s\n", whitePieces, blackPieces);
    int iteration = 0;

    printBoard(board);
    // Generate current board hash
    unsigned long currentHash = hashBoardState(board, NODE_NUM);
    // printf("Current Hash: %lu\n", currentHash);
    // Check if the current state is already in the hash table
    if (lookupBoardState(currentHash, whitePieces, blackPieces))
    {
        printf("State already visited.\n");
        return 0;
    }
    // Insert the current state into the hash table
    insertBoardState(currentHash, whitePieces, blackPieces);
    // printf("State inserted into hash table.\n");

    predecessors[0].predecessor = -1;
    strncpy(predecessors[0].move, "--", 2);
    predecessors[0].move[2] = '\0'; // Ensure null-termination

    generateNextStates(board, &queue, iteration);
    // printQueue(&queue);

    start = clock();

    while (!isQueueEmpty(&queue))
    {
    next_iteration:;

#ifndef SIMPLE
        printf("----------Iteration: %6d----------\n", iteration);
#endif // !SIMPLE

        QueueData state = dequeueQueue(&queue);

#ifndef SIMPLE
        printf("Dequeue: %s %s|%s : ", state.whitePos, state.blackPos, state.move);
#endif // !SIMPLE

        char prevPos = state.move[0];
        char newPos = state.move[1];
        // printf("Moving from %c to %c\n", prevPos, newPos);

        setBoardState(board, state.whitePos, state.blackPos);

        Node *prevNode = NULL;
        Node *newNode = NULL;

        // Find the previous and new nodes and validate move
        for (int i = 0; i < NODE_NUM; i++)
        {
            if (board[i]->label == prevPos)
            {
                prevNode = board[i];
                if (!prevNode->occupied)
                {
                    printf("[Skipping]: No piece found at %c\n", prevPos);
                    goto next_iteration; // Jump to the end of the loop
                }
            }
            else if (board[i]->label == newPos)
            {
                newNode = board[i];
                if (newNode->occupied)
                {
                    printf("[Skipping]: %c is already occupied\n", newPos);
                    goto next_iteration; // Jump to the end of the loop
                }
            }
        }

        if (prevNode == NULL)
        {
            printf("[Skipping]: No piece found at %c\n", prevPos);
            goto next_iteration;
        }
        if (newNode == NULL)
        {
            printf("[Skipping]: Invalid move to %c\n", newPos);
            goto next_iteration;
        }

        bool validMove = false;
        for (int j = 0; j < 3 && prevNode->neighbors[j] != NULL; j++)
        {
            if (prevNode->neighbors[j]->label == newPos)
            {
                validMove = true;
                break;
            }
        }
        if (!validMove)
        {
            printf("[Skipping]: Invalid move from %c to %c\n", prevPos, newPos);
            goto next_iteration;
        }

        // Set the new position in a single loop
        for (int i = 0; i < NODE_NUM; i++)
        {
            if (board[i] == prevNode)
            {
                board[i]->occupied = false;
            }
            else if (board[i] == newNode)
            {
                board[i]->occupied = true;
                board[i]->color = prevNode->color;
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

        unsigned long newHash = hashBoardState(board, NODE_NUM);
        // printf("New Hash: %lu\n", newHash);
        // Check if the new state is already in the hash table
        if (lookupBoardState(newHash, newWhitePos, newBlackPos))
        {
#ifndef SIMPLE
            printf("[Skipping]: State already visited.\n");
#endif // !SIMPLE
            free(newWhitePos);
            free(newBlackPos);
            continue; // Skip to the next iteration
        }

        // Insert the new state into the hash table
        insertBoardState(newHash, newWhitePos, newBlackPos);
        iteration++;

        // Generate next states
        predecessors[iteration].predecessor = state.predecessor;
        strncpy(predecessors[iteration].move, state.move, 2);
        predecessors[iteration].move[2] = '\0'; // Ensure null-termination

        if (isGoalState(board, argv[3], argv[4]))
        {
            printf("Goal state reached!\n");
            goalState = iteration;
            printBoard(board);
            int *path = (int *)malloc(QUEUE_SIZE * sizeof(int));
            int pathLength = 0;
            reconstructPath(predecessors, iteration, path, &pathLength);
#ifndef SIMPLE
            printBoardForSolution(board, argv[1], argv[2], path, pathLength, predecessors);
#endif // !SIMPLE

            free(path);
            free(newWhitePos);
            free(newBlackPos);
            break;
        }

        generateNextStates(board, &queue, iteration);
        // printQueue(&queue);

        free(whitePieces);
        free(blackPieces);

        whitePieces = strdup(newWhitePos);
        blackPieces = strdup(newBlackPos);
        free(newWhitePos);
        free(newBlackPos);
        // sleep(2); // Sleep for 0.5 seconds
    }
    end = clock();
    cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;

    if (goalState == -1)
    {
        printf("No solution found.\n");
    }
    else
    {
#ifndef SIMPLE
        printf("Solution found in %d iterations.\n", goalState);
#endif // !SIMPLE
    }
    printf("Time taken: %f seconds\n", cpu_time_used);

    freeQueue(&queue);
    free(whitePieces);
    free(blackPieces);
    for (int i = 0; i < NODE_NUM; i++)
    {
        free(board[i]);
    }

    freeHashTable();

    return 0;
}
