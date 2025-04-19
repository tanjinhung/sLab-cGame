#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#define BINARY
#define SIMPLE

#define TABLE_SIZE 1048576 // 2^20
#define NUM_POSITIONS 14

#define SET_BIT(variable, bit) ((variable) |= (1UL << (bit)))
#define CLEAR_BIT(variable, bit) ((variable) &= ~(1UL << (bit)))
#define CHECK_BIT(variable, bit) (((variable) >> (bit)) & 1)

static inline int pos_offset(char pos)
{
    return 2 * (NUM_POSITIONS - 1 - (pos - 'A'));
}

uint32_t init_board(const char *white_pieces, const char *black_pieces)
{
    uint32_t board = 0;
    for (int i = 0; white_pieces[i] != '\0'; i++)
    {
        int offset = pos_offset(white_pieces[i]);
        board |= (0b01 << offset); // Set the bit for white pieces
    }
    for (int i = 0; black_pieces[i] != '\0'; i++)
    {
        int offset = pos_offset(black_pieces[i]);
        board |= (0b11 << offset); // Set the bit for black pieces
    }
    return board;
}

static inline void move_piece(uint32_t *board, char from, char to)
{
    int from_offset = pos_offset(from);
    int to_offset = pos_offset(to);

    uint32_t mask = 0b11;
    uint32_t piece = (*board >> from_offset) & mask;

    if (piece == 0)
    {
        fprintf(stderr, "Error: No piece at position %c\n", from);
        return;
    }
    if (CHECK_BIT(*board, to_offset))
    {
        fprintf(stderr, "Error: Position %c is already occupied\n", to);
        return;
    }

    *board = (*board & ~(mask << from_offset) & ~(mask << to_offset)) | (piece << to_offset); // Set the piece in the new position
}

char get_symbol(uint32_t board, int pos_index)
{
    int offset = 2 * (NUM_POSITIONS - 1 - pos_index);
    uint32_t val = (board >> offset) & 0b11;
    if ((val & 0b01) == 0)
        return '-';                  // Not occupied
    return (val & 0b10) ? 'x' : 'o'; // Black or White
}

void print_board(uint32_t board)
{
#ifdef BINARY
    printf("0b");
    for (int i = NUM_POSITIONS * 2 - 1; i >= 0; i--)
    {
        printf("%d", (board >> i) & 1);
    }
    printf("\n");
#endif // BINARY
#ifndef BINARY
    // Map each board index to its row and col in the output
    int row_map[NUM_POSITIONS] = {4, 4, 4, 5, 6, 7, 8, 0, 1, 2, 3, 4, 4, 4};
    int col_map[NUM_POSITIONS] = {0, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 4, 5};

    // Find grid size
    int max_row = 0, max_col = 0;
    for (int i = 0; i < NUM_POSITIONS; i++)
    {
        if (row_map[i] > max_row)
            max_row = row_map[i];
        if (col_map[i] > max_col)
            max_col = col_map[i];
    }
    max_row++;
    max_col++;

    // Prepare grid
    char grid[max_row][max_col];
    for (int r = 0; r < max_row; r++)
        for (int c = 0; c < max_col; c++)
            grid[r][c] = ' ';

    // Place symbols
    for (int i = 0; i < NUM_POSITIONS; i++)
    {
        char sym = get_symbol(board, i);
        grid[row_map[i]][col_map[i]] = sym;
    }

    // Print the grid
    for (int r = 0; r < max_row; r++)
    {
        for (int c = 0; c < max_col; c++)
            putchar(grid[r][c]);
        putchar('\n');
    }
#endif // !BINARY
}

#pragma region Queue Implementation
typedef struct QueueNode
{
    uint32_t board;
    int predecessor;
    int move;
    struct QueueNode *next;
} QueueNode;

typedef struct Queue
{
    QueueNode *head;
    QueueNode *tail;
    int size;
} Queue;

void init_queue(Queue *queue)
{
    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;
}

static inline int is_queue_empty(Queue *queue)
{
    return queue->head == NULL;
}

void enqueue(Queue *queue, uint32_t board, int move, int predecessor)
{
    QueueNode *new_node = (QueueNode *)malloc(sizeof(QueueNode));
    if (new_node == NULL)
    {
        perror("Failed to allocate memory for queue node");
        return;
    }
    new_node->board = board;
    new_node->move = move;
    new_node->predecessor = predecessor;
    new_node->next = NULL;

    if (is_queue_empty(queue))
    {
        queue->head = new_node;
        queue->tail = new_node;
    }
    else
    {
        queue->tail->next = new_node;
        queue->tail = new_node;
    }
    queue->size++;
}

QueueNode *dequeue(Queue *queue)
{
    if (is_queue_empty(queue))
    {
        return NULL;
    }
    QueueNode *temp = queue->head;
    queue->head = queue->head->next;
    if (is_queue_empty(queue))
    {
        queue->tail = NULL; // Reset tail if the queue is empty
    }
    queue->size--;
    return temp;
}

void free_queue(Queue *queue)
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

void print_queue(Queue *queue)
{
    printf("Queue: ");
    QueueNode *current = queue->head;
    while (current != NULL)
    {
        printf("%x ", current->board);
        current = current->next;
    }
    printf("\n");
}
#pragma endregion
#pragma region Hash Table Implementation
typedef struct HashEntry
{
    uint32_t board;
    struct HashEntry *next;
} HashEntry;

HashEntry *hash_table[TABLE_SIZE];

static inline unsigned long hash_board_state(uint32_t board)
{
    return board % TABLE_SIZE;
}

void init_hash_table()
{
    for (int i = 0; i < TABLE_SIZE; i++)
    {
        hash_table[i] = NULL;
    }
}

void insert_board_state(uint32_t board)
{
    unsigned long key = hash_board_state(board);
    HashEntry *new_entry = (HashEntry *)malloc(sizeof(HashEntry));
    if (new_entry == NULL)
    {
        perror("Failed to allocate memory for hash entry");
        return;
    }
    new_entry->board = board;
    new_entry->next = hash_table[key];
    hash_table[key] = new_entry;
}

int lookup_board_state(uint32_t board)
{
    unsigned long key = hash_board_state(board);
    HashEntry *current = hash_table[key];
    while (current != NULL)
    {
        if (current->board == board)
        {
            return 1; // Found
        }
        current = current->next;
    }
    return 0; // Not found
}

void free_hash_table()
{
    for (int i = 0; i < TABLE_SIZE; i++)
    {
        HashEntry *current = hash_table[i];
        while (current != NULL)
        {
            HashEntry *temp = current;
            current = current->next;
            free(temp);
        }
        hash_table[i] = NULL;
    }
}
#pragma endregion
#pragma region Predecessor Table Implementation
typedef struct Predecessor
{
    uint32_t board;
    int predecessor;
    int move;
} Predecessor;

Predecessor predecessors[TABLE_SIZE];

void init_predecessor_table()
{
    for (int i = 0; i < TABLE_SIZE; i++)
    {
        predecessors[i].predecessor = -1;
        predecessors[i].move = -1;
    }
}

void set_predecessor(int index, int predecessor, int move, uint32_t board)
{
    predecessors[index].board = board;
    predecessors[index].predecessor = predecessor;
    predecessors[index].move = move;
}

void reconstruct_path(int goal_index, int path[], int *path_length)
{
    if (predecessors == NULL || path == NULL || path_length == NULL)
    {
        fprintf(stderr, "[Error]: Null pointer passed to reconstructPath.\n");
        return;
    }
    // Initialize path length

    // Reconstruct the path from the goal index to the start
    int current_index = goal_index;
    int index = 0;
    while (current_index != -1)
    {
        // printf("Current index: %d\n", current_index);
        path[index] = current_index;
        current_index = predecessors[current_index].predecessor;
        index++;
    }

    *path_length = index;

    // Reverse the path to get it from start to goal
    for (int i = 0; i < *path_length / 2; i++)
    {
        int temp = path[i];
        path[i] = path[*path_length - 1 - i];
        path[*path_length - 1 - i] = temp;
    }
}

void print_path(int *path, int path_length)
{
    printf("Path:\n");
    for (int i = 0; i < path_length; i++)
    {
        int move = predecessors[path[i]].move;
        char from = 'A' + ((move >> 4) & 0b1111);
        char to = 'A' + (move & 0b1111);
#ifdef SIMPLE
        printf("%c%c\n", from, to);
#else
        print_board(predecessors[path[i]].board);
        printf("Move: %c -> %c | %x, Predecessor: %d\n",
               from, to, predecessors[path[i]].move, predecessors[path[i]].predecessor);
#endif // !SIMPLE
    }
    printf("\n");
}

#pragma endregion

void generateNextState(Queue *queue, uint32_t board, const int neighbors[NUM_POSITIONS][4], int predecessor)
{
    for (int i = 0; i < NUM_POSITIONS; i++)
    {
        char from = 'A' + i;
        int state = (board >> pos_offset(from)) & 0b11;
        if ((state & 0b01) == 0)
            continue; // No piece at this position

        for (int j = 0; j < 4; j++)
        {
            int neighbor = neighbors[i][j];
            if (neighbor == -1)
                break; // No more neighbor

            char to = 'A' + neighbor;
            int to_state = (board >> pos_offset(to)) & 0b11;
            if (to_state == 0) // Empty position
            {
                uint32_t new_board = board;
                move_piece(&new_board, from, to);
                if (!lookup_board_state(new_board))
                {
                    int move = ((from - 'A') << 4) | (to - 'A');
                    // printf("Move: %c->%c | %x\n", from, to, move);
                    enqueue(queue, new_board, move, predecessor);
                    insert_board_state(new_board);
                }
            }
        }
    }
}

int main(int argc, char *argv[])
{

    clock_t start, end;
    double cpu_time_used;

#pragma region Argument Validation
    if (argc != 5)
    {
        fprintf(stderr, "Usage: %s <white pieces> <black pieces> <white end> <black end>\n", argv[0]);
        return 1;
    }

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
#pragma region Initialization
    const char *white_start = argv[1];
    const char *black_start = argv[2];
    const char *white_goal = argv[3];
    const char *black_goal = argv[4];
    const int neighbors[NUM_POSITIONS][4] = {
        /* A */ {1, -1, -1, -1},
        /* B */ {0, 2, -1, -1},
        /* C */ {1, 3, 11, -1},
        /* D */ {2, 4, -1, -1},
        /* E */ {3, 5, -1, -1},
        /* F */ {4, 6, -1, -1},
        /* G */ {5, -1, -1, -1},
        /* H */ {8, -1, -1, -1},
        /* I */ {7, 9, -1, -1},
        /* J */ {8, 10, -1, -1},
        /* K */ {9, 11, -1, -1},
        /* L */ {2, 10, 12, -1},
        /* M */ {11, 13, -1, -1},
        /* N */ {12, -1, -1, -1}};
    uint32_t board_start = init_board(white_start, black_start);
    uint32_t board_goal = init_board(white_goal, black_goal);

    Queue queue;
    init_queue(&queue);
    init_hash_table();
    int goal_state = -1;
    int *path = (int *)malloc(TABLE_SIZE * sizeof(int));
    int path_length = 0;
#pragma endregion

    insert_board_state(board_start);
    generateNextState(&queue, board_start, neighbors, -1);
    print_board(board_start);

#pragma region Main Loop
    start = clock();
    int iteration = 0;
    while (!is_queue_empty(&queue))
    {
        QueueNode *state = dequeue(&queue);
        if (state == NULL)
            break; // Queue is empty

        uint32_t current_board = state->board;
        int predecessor = state->predecessor;
        int move = state->move;

        set_predecessor(iteration, predecessor, move, current_board);

        // Check if the current board matches the goal board
        if (current_board == board_goal)
        {
            printf("Goal state reached!\n");
            print_board(current_board);

            goal_state = iteration;
            reconstruct_path(iteration, path, &path_length);

            free(state);
            break;
        }

        generateNextState(&queue, current_board, neighbors, iteration++);
        free(state);
        // sleep(1);
    }
    end = clock();
#pragma endregion

    if (goal_state == -1)
    {
        printf("No solution found.\n");
    }
    else
    {
        printf("Solution found in %d iterations.\n\n", goal_state);
        print_path(path, path_length);
    }

    free_queue(&queue);
    free_hash_table();
    free(path);
    cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Time taken: %f seconds\n", cpu_time_used);
    return 0;
}