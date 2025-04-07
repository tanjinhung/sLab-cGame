#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define NODE_NUM 14

typedef struct node
{
    char label;
    bool color;
    bool occupied;

} Node;

Node board[NODE_NUM];

int neighbours[NODE_NUM][3] = {
    { 1, -1, -1}, // A - 0
    { 0,  2, -1}, // B - 1
    { 1,  3, 11}, // C - 2
    { 2,  4, -1}, // D - 3
    { 3,  5, -1}, // E - 4
    { 4,  6, -1}, // F - 5
    { 5, -1, -1}, // G - 6
    { 8, -1, -1}, // H - 7
    { 7,  9, -1}, // I - 8
    { 8, 10, -1}, // J - 9
    { 9, 11, -1}, // K - 10
    {10, 12,  2}, // L - 11
    {13, 11, -1}, // M - 12
    {12, -1, -1}, // N - 13
};

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

    for (int i = 0; i < NODE_NUM; i++)
    {
        board[i].label = 'A' + i;
        board[i].color = 0;
        board[i].occupied = false;
    }

    // Set the initial state of the board
    char *startPos = (char *)malloc(sizeof(argv[1]) + sizeof(argv[2] + 1));
    strcpy(startPos, argv[1]);
    strcat(startPos, argv[2]);

    int whiteCount = strlen(argv[1]);
    int blackCount = strlen(argv[2]);

    for (int i = 0; i < strlen(startPos); i++)
    {
        for (int j = 0; j < NODE_NUM; j++)
        {
            if (board[j].label == startPos[i] && i < whiteCount)
            {
                board[j].color = 0; // White
                board[j].occupied = true;
                break;
            }
            else if (board[j].label == startPos[i] && i >= whiteCount)
            {
                board[j].color = 1; // Black
                board[j].occupied = true;
                break;
            }
        }
    }

    
    
    // Print the initial state of the board
    printf("Initial state of the board:\n");
    for (int i = 0; i < NODE_NUM; i++)
    {
        printf("%c: %s %s\n", board[i].label, board[i].color == 0 ? "white" : "black", board[i].occupied ? "Occupied" : "Empty");
    }
    printf("\n");


    return 0;
}
