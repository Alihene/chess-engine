#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <math.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float f32;
typedef double f64;

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

typedef enum {
    KING = 0x0,
    PAWN = 0x1,
    BISHOP = 0x2,
    KNIGHT = 0x3,
    ROOK = 0x4,
    QUEEN = 0x5
} PieceType;

typedef struct {
    PieceType type;

    // 0 = white, 1 = black
    u8 color;
} Piece;

typedef struct {
    struct {
        u32 x, y;
    } from, to;

    bool capture;
} Move;

#define COLOR_WHITE 0
#define COLOR_BLACK 1

typedef struct {
    Piece pieces[64];
    u64 pieces_state;

    struct {
        u32 x, y;
    } white_king_pos, black_king_pos;
} Board;

Board board = {0};
Board search_board = {0};

u32 minimax_count = 0;
Move best_move;

Piece *get_piece(u32 x, u32 y, Board *b) {
    u32 index = x + y * 8;
    if(index >= 64) {
        return NULL;
    }
    if(b->pieces_state & (1L << index)) {
        return &b->pieces[index];
    }

    return NULL;
}

void set_piece(u32 x, u32 y, const Piece *piece, Board *b) {
    b->pieces[x + y * 8] = *piece;
    b->pieces_state |= (1L << (x + y * 8));

    if(piece->type == KING) {
        if(piece->color == COLOR_WHITE) {
            b->white_king_pos.x = x;
            b->white_king_pos.y = y;
        } else {
            b->black_king_pos.x = x;
            b->black_king_pos.y = y;
        }
    }
}

void setup_board(Board *b) {
    set_piece(0, 0, &(Piece){.type = ROOK, .color = COLOR_WHITE}, b);
    set_piece(1, 0, &(Piece){.type = KNIGHT, .color = COLOR_WHITE}, b);
    set_piece(2, 0, &(Piece){.type = BISHOP, .color = COLOR_WHITE}, b);
    set_piece(3, 0, &(Piece){.type = QUEEN, .color = COLOR_WHITE}, b);
    set_piece(4, 0, &(Piece){.type = KING, .color = COLOR_WHITE}, b);
    set_piece(5, 0, &(Piece){.type = BISHOP, .color = COLOR_WHITE}, b);
    set_piece(6, 0, &(Piece){.type = KNIGHT, .color = COLOR_WHITE}, b);
    set_piece(7, 0, &(Piece){.type = ROOK, .color = COLOR_WHITE}, b);
    for(u32 i = 0; i < 8; i++) {
        set_piece(i, 1, &(Piece){.type = PAWN, .color = COLOR_WHITE}, b);
    }

    set_piece(0, 7, &(Piece){.type = ROOK, .color = COLOR_BLACK}, b);
    set_piece(1, 7, &(Piece){.type = KNIGHT, .color = COLOR_BLACK}, b);
    set_piece(2, 7, &(Piece){.type = BISHOP, .color = COLOR_BLACK}, b);
    set_piece(3, 7, &(Piece){.type = QUEEN, .color = COLOR_BLACK}, b);
    set_piece(4, 7, &(Piece){.type = KING, .color = COLOR_BLACK}, b);
    set_piece(5, 7, &(Piece){.type = BISHOP, .color = COLOR_BLACK}, b);
    set_piece(6, 7, &(Piece){.type = KNIGHT, .color = COLOR_BLACK}, b);
    set_piece(7, 7, &(Piece){.type = ROOK, .color = COLOR_BLACK}, b);
    for(u32 i = 0; i < 8; i++) {
        set_piece(i, 6, &(Piece){.type = PAWN, .color = COLOR_BLACK}, b);
    }
}

void remove_piece(u32 x, u32 y, Board *b) {
    b->pieces_state &= ~(1L << (x + y * 8));
}

void move_piece(u32 x_from, u32 y_from, u32 x_to, u32 y_to, Board *b) {
    Piece *piece = get_piece(x_from, y_from, b);
    if(!piece) {
        return;
    }

    remove_piece(x_from, y_from, b);
    set_piece(x_to, y_to, piece, b);
}

i32 evaluate_board(Board *b) {
    i32 white = 0;
    i32 black = 0;

    for(i32 x = 0; x < 8; x++) {
        for(i32 y = 0; y < 8; y++) {
            Piece *piece = get_piece(x, y, b);
            if(piece) {
                i32 value = 0;
                switch(piece->type) {
                    case KING:
                    {
                        value = 2000;
                        break;
                    }
                    case PAWN:
                    {
                        value = 100;
                        value += (i32)(8 - fabsf(3.5f - x)) * 8;
                        value += (i32)(8 - fabsf(3.5f - y)) * 8;
                        break;
                    }
                    case BISHOP:
                    case KNIGHT:
                    {
                        value = 300;
                        value += (i32)(8 - fabsf(3.5f - x)) * 6;
                        value += (i32)(8 - fabsf(3.5f - y)) * 6;
                        break;
                    }
                    case ROOK:
                    {
                        value = 500;
                        break;
                    }
                    case QUEEN:
                    {
                        value = 900;
                        value += (i32)(8 - fabsf(3.5f - x)) * 5;
                        value += (i32)(8 - fabsf(3.5f - y)) * 5;
                        break;
                    }
                    default:
                    {
                        value = 0;
                        break;
                    }
                }

                if(piece->color == COLOR_WHITE) {
                    white += value;
                } else {
                    black += value;
                }
            }
        }
    }

    i32 piece_eval = (white - black);
    return piece_eval;
}

char piece_to_char(const Piece *piece) {
    switch(piece->type) {
        case KING:
        {
            if(!piece->color) {
                return 'K';
            } else {
                return 'k';
            }
            break;
        }
        case PAWN:
        {
            if(!piece->color) {
                return 'P';
            } else {
                return 'p';
            }
            break;
        }
        case BISHOP:
        {
            if(!piece->color) {
                return 'B';
            } else {
                return 'b';
            }
            break;
        }
        case KNIGHT:
        {
            if(!piece->color) {
                return 'N';
            } else {
                return 'n';
            }
            break;
        }
        case ROOK:
        {
            if(!piece->color) {
                return 'R';
            } else {
                return 'r';
            }
            break;
        }
        case QUEEN:
        {
            if(!piece->color) {
                return 'Q';
            } else {
                return 'q';
            }
            break;
        }
        default:
        {
            return '\0';
            break;
        }
    }
}

void print_board(const Board *b) {
    printf("-----------------\n");
    for(i32 i = 56; i >= 0; i -= 8) {
        printf("|");
        for(u32 j = i; j < i + 8; j++) {
            if(b->pieces_state & (1L << j)) {
                printf("%c|", piece_to_char(&b->pieces[j]));
            } else {
                printf(" |");
            }
        }
        printf("\n");
    }
    printf("-----------------\n");
}

void generate_moves_for_piece(u32 x, u32 y, u32 color_to_move, Board *b, Move *moves, u32 *move_count) {
    const Piece *piece = get_piece(x, y, b);
    if(!piece) {
        *move_count = 0;
        return;
    }
    if(piece->color != color_to_move) {
        *move_count = 0;
        return;
    }

    u32 i = 0;
    Move move;
    move.from.x = x;
    move.from.y = y;
    move.to.x = x;
    move.to.y = y;
    move.capture = false;
    switch(piece->type) {
        case KING:
        {
            // Left
            if(x > 0 && !(get_piece(x - 1, y, b) && get_piece(x - 1, y, b)->color == color_to_move)) {
                move.to.x = x - 1;
                if(get_piece(x - 1, y, b) && get_piece(x - 1, y, b)->color != color_to_move) {
                    move.capture = true;
                } else {
                    move.capture = false;
                }
                moves[i] = move;
                i++;
            }

            // Top left
            if(x > 0 && y < 7 && !(get_piece(x - 1, y + 1, b) && get_piece(x - 1, y + 1, b)->color == color_to_move)) {
                move.to.x = x - 1;
                move.to.y = y + 1;
                if(get_piece(x - 1, y + 1, b) && get_piece(x - 1, y + 1, b)->color != color_to_move) {
                    move.capture = true;
                } else {
                    move.capture = false;
                }
                moves[i] = move;
                i++;
            }

            // Top
            if(y < 7 && !(get_piece(x, y + 1, b) && get_piece(x, y + 1, b)->color == color_to_move)) {
                move.to.y = y + 1;
                if(get_piece(x, y + 1, b) && get_piece(x, y + 1, b)->color != color_to_move) {
                    move.capture = true;
                } else {
                    move.capture = false;
                }
                moves[i] = move;
                i++;
            }

            // Top right
            if(x < 7 && y < 7 && !(get_piece(x + 1, y + 1, b) && get_piece(x + 1, y + 1, b)->color == color_to_move)) {
                move.to.x = x + 1;
                move.to.y = y + 1;
                if(get_piece(x + 1, y + 1, b) && get_piece(x + 1, y + 1, b)->color != color_to_move) {
                    move.capture = true;
                } else {
                    move.capture = false;
                }
                moves[i] = move;
                i++;
            }

            // Right
            if(x < 7 && !(get_piece(x + 1, y, b) && get_piece(x + 1, y, b)->color == color_to_move)) {
                move.to.x = x + 1;
                if(get_piece(x + 1, y, b) && get_piece(x + 1, y, b)->color != color_to_move) {
                    move.capture = true;
                } else {
                    move.capture = false;
                }
                moves[i] = move;
                i++;
            }

            // Bottom right
            if(x < 7 && y > 0 && !(get_piece(x + 1, y - 1, b) && get_piece(x + 1, y - 1, b)->color == color_to_move)) {
                move.to.x = x + 1;
                move.to.y = y - 1;
                if(get_piece(x + 1, y - 1, b) && get_piece(x + 1, y - 1, b)->color != color_to_move) {
                    move.capture = true;
                } else {
                    move.capture = false;
                }
                moves[i] = move;
                i++;
            }

            // Bottom
            if(y > 0 && !(get_piece(x, y - 1, b) && get_piece(x, y - 1, b)->color == color_to_move)) {
                move.to.y = y - 1;
                if(get_piece(x, y - 1, b) && get_piece(x, y - 1, b)->color != color_to_move) {
                    move.capture = true;
                } else {
                    move.capture = false;
                }
                moves[i] = move;
                i++;
            }

            // Bottom left
            if(x > 0 && y > 0 && !(get_piece(x - 1, y - 1, b) && get_piece(x - 1, y - 1, b)->color == color_to_move)) {
                move.to.x = x - 1;
                move.to.y = y - 1;
                if(get_piece(x - 1, y - 1, b) && get_piece(x - 1, y - 1, b)->color != color_to_move) {
                    move.capture = true;
                } else {
                    move.capture = false;
                }
                moves[i] = move;
                i++;
            }
            break;
        }
        case PAWN:
        {
            if(color_to_move == COLOR_WHITE) {
                // One square
                if(y < 7 && !get_piece(x, y + 1, b)) {
                    move.to.y = y + 1;
                    moves[i] = move;
                    i++;
                }
                // Two squares
                if(y == 1
                    && !get_piece(x, 2, b)
                    && !get_piece(x, 3, b)) {
                    move.to.y = 3;
                    moves[i] = move;
                    i++;
                }
                // Diagonal capture
                if(y < 7
                    && x < 7
                    && get_piece(x + 1, y + 1, b)
                    && get_piece(x + 1, y + 1, b)->color != color_to_move) {
                    move.to.x = x + 1;
                    move.to.y = y + 1;
                    move.capture = true;
                    moves[i] = move;
                    i++;
                }
                if(y < 7
                    && x > 0
                    && get_piece(x - 1, y + 1, b)
                    && get_piece(x - 1, y + 1, b)->color != color_to_move) {
                    move.to.x = x - 1;
                    move.to.y = y + 1;
                    move.capture = true;
                    moves[i] = move;
                    i++;
                }
                move.capture = false;
            } else {
                // One square
                if(y > 0 && !get_piece(x, y - 1, b)) {
                    move.to.y = y - 1;
                    moves[i] = move;
                    i++;
                }
                // Two squares
                if(y == 6
                    && !get_piece(x, 5, b)
                    && !get_piece(x, 4, b)) {
                    move.to.y = 4;
                    moves[i] = move;
                    i++;
                }
                // Diagonal capture
                if(y > 0
                    && x < 7
                    && get_piece(x + 1, y - 1, b)
                    && get_piece(x + 1, y - 1, b)->color != color_to_move) {
                    move.to.x = x + 1;
                    move.to.y = y - 1;
                    move.capture = true;
                    moves[i] = move;
                    i++;
                }
                if(y > 0
                    && x > 0
                    && get_piece(x - 1, y - 1, b)
                    && get_piece(x - 1, y - 1, b)->color != color_to_move) {
                    move.to.x = x - 1;
                    move.to.y = y - 1;
                    move.capture = true;
                    moves[i] = move;
                    i++;
                }
                move.capture = false;
            }
            break;
        }
        case BISHOP:
        {
            // Top left
            u32 x1 = x;
            u32 y1 = y;
            move.capture = false;
            while(x1 > 0 && y1 < 7) {
                x1--;
                y1++;
                Piece *next_piece = get_piece(x1, y1, b);
                if(next_piece && next_piece->color == color_to_move) {
                    break;
                } else if(next_piece && next_piece->color != color_to_move) {
                    move.to.x = x1;
                    move.to.y = y1;
                    move.capture = true;
                    moves[i] = move;
                    i++;
                    break;
                } else {
                    move.to.x = x1;
                    move.to.y = y1;
                    moves[i] = move;
                    i++;
                }
            }

            // Top right
            x1 = x;
            y1 = y;
            move.capture = false;
            while(x1 < 7 && y1 < 7) {
                x1++;
                y1++;
                Piece *next_piece = get_piece(x1, y1, b);
                if(next_piece && next_piece->color == color_to_move) {
                    break;
                } else if(next_piece && next_piece->color != color_to_move) {
                    move.to.x = x1;
                    move.to.y = y1;
                    move.capture = true;
                    moves[i] = move;
                    i++;
                    break;
                } else {
                    move.to.x = x1;
                    move.to.y = y1;
                    moves[i] = move;
                    i++;
                }
            }

            // Bottom right
            x1 = x;
            y1 = y;
            move.capture = false;
            while(x1 < 7 && y1 > 0) {
                x1++;
                y1--;
                Piece *next_piece = get_piece(x1, y1, b);
                if(next_piece && next_piece->color == color_to_move) {
                    break;
                } else if(next_piece && next_piece->color != color_to_move) {
                    move.to.x = x1;
                    move.to.y = y1;
                    move.capture = true;
                    moves[i] = move;
                    i++;
                    break;
                } else {
                    move.to.x = x1;
                    move.to.y = y1;
                    moves[i] = move;
                    i++;
                }
            }

            // Bottom left
            x1 = x;
            y1 = y;
            move.capture = false;
            while(x1 > 0 && y1 > 0) {
                x1--;
                y1--;
                Piece *next_piece = get_piece(x1, y1, b);
                if(next_piece && next_piece->color == color_to_move) {
                    break;
                } else if(next_piece && next_piece->color != color_to_move) {
                    move.to.x = x1;
                    move.to.y = y1;
                    move.capture = true;
                    moves[i] = move;
                    i++;
                    break;
                } else {
                    move.to.x = x1;
                    move.to.y = y1;
                    moves[i] = move;
                    i++;
                }
            }
            break;
        }
        case KNIGHT:
        {
            // Left 1
            if(x > 1 && y > 0 && !(get_piece(x - 2, y - 1, b) && get_piece(x - 2, y - 1, b)->color == color_to_move)) {
                move.to.x = x - 2;
                move.to.y = y - 1;
                if(get_piece(x - 2, y - 1, b) && get_piece(x - 2, y - 1, b)->color != color_to_move) {
                    move.capture = true;
                } else {
                    move.capture = false;
                }
                moves[i] = move;
                i++;
            }
            // Left 2
            if(x > 1 && y < 7 && !(get_piece(x - 2, y + 1, b) && get_piece(x - 2, y + 1, b)->color == color_to_move)) {
                move.to.x = x - 2;
                move.to.y = y + 1;
                if(get_piece(x - 2, y + 1, b) && get_piece(x - 2, y + 1, b)->color != color_to_move) {
                    move.capture = true;
                } else {
                    move.capture = false;
                }
                moves[i] = move;
                i++;
            }
            // Top 1
            if(x > 0 && y < 6 && !(get_piece(x - 1, y + 2, b) && get_piece(x - 1, y + 2, b)->color == color_to_move)) {
                move.to.x = x - 1;
                move.to.y = y + 2;
                if(get_piece(x - 1, y + 2, b) && get_piece(x - 1, y + 2, b)->color != color_to_move) {
                    move.capture = true;
                } else {
                    move.capture = false;
                }
                moves[i] = move;
                i++;
            }
            // Top 2
            if(x < 7 && y < 6 && !(get_piece(x + 1, y + 2, b) && get_piece(x + 1, y + 2, b)->color == color_to_move)) {
                move.to.x = x + 1;
                move.to.y = y + 2;
                if(get_piece(x + 1, y + 2, b) && get_piece(x + 1, y + 2, b)->color != color_to_move) {
                    move.capture = true;
                } else {
                    move.capture = false;
                }
                moves[i] = move;
                i++;
            }
            // Right 1
            if(x < 6 && y < 7 && !(get_piece(x + 2, y + 1, b) && get_piece(x + 2, y + 1, b)->color == color_to_move)) {
                move.to.x = x + 2;
                move.to.y = y + 1;
                if(get_piece(x + 2, y + 1, b) && get_piece(x + 2, y + 1, b)->color != color_to_move) {
                    move.capture = true;
                } else {
                    move.capture = false;
                }
                moves[i] = move;
                i++;
            }
            // Right 2
            if(x < 6 && y > 0 && !(get_piece(x + 2, y - 1, b) && get_piece(x + 2, y - 1, b)->color == color_to_move)) {
                move.to.x = x + 2;
                move.to.y = y - 1;
                if(get_piece(x + 2, y - 1, b) && get_piece(x + 2, y - 1, b)->color != color_to_move) {
                    move.capture = true;
                } else {
                    move.capture = false;
                }
                moves[i] = move;
                i++;
            }
            // Bottom 1
            if(x < 7 && y > 1 && !(get_piece(x + 1, y - 2, b) && get_piece(x + 1, y - 2, b)->color == color_to_move)) {
                move.to.x = x + 1;
                move.to.y = y - 2;
                if(get_piece(x + 1, y - 2, b) && get_piece(x + 1, y - 2, b)->color != color_to_move) {
                    move.capture = true;
                } else {
                    move.capture = false;
                }
                moves[i] = move;
                i++;
            }
            // Bottom 2
            if(x > 0 && y > 1 && !(get_piece(x - 1, y - 2, b) && get_piece(x - 1, y - 2, b)->color == color_to_move)) {
                move.to.x = x - 1;
                move.to.y = y - 2;
                if(get_piece(x - 1, y - 2, b) && get_piece(x - 1, y - 2, b)->color != color_to_move) {
                    move.capture = true;
                } else {
                    move.capture = false;
                }
                moves[i] = move;
                i++;
            }
            break;
        }
        case ROOK:
        {
            // Left
            u32 x1 = x;
            u32 y1 = y;
            while(x1 > 0) {
                x1--;
                Piece *next_piece = get_piece(x1, y1, b);
                if(next_piece && next_piece->color == color_to_move) {
                    break;
                } else if(next_piece && next_piece->color != color_to_move) {
                    move.to.x = x1;
                    move.to.y = y1;
                    moves[i] = move;
                    i++;
                    break;
                } else {
                    move.to.x = x1;
                    move.to.y = y1;
                    moves[i] = move;
                    i++;
                }
            }
            // Top
            x1 = x;
            y1 = y;
            while(y1 < 7) {
                y1++;
                Piece *next_piece = get_piece(x1, y1, b);
                if(next_piece && next_piece->color == color_to_move) {
                    break;
                } else if(next_piece && next_piece->color != color_to_move) {
                    move.to.x = x1;
                    move.to.y = y1;
                    moves[i] = move;
                    i++;
                    break;
                } else {
                    move.to.x = x1;
                    move.to.y = y1;
                    moves[i] = move;
                    i++;
                }
            }
            // Right
            x1 = x;
            y1 = y;
            while(x1 < 7) {
                x1++;
                Piece *next_piece = get_piece(x1, y1, b);
                if(next_piece && next_piece->color == color_to_move) {
                    break;
                } else if(next_piece && next_piece->color != color_to_move) {
                    move.to.x = x1;
                    move.to.y = y1;
                    moves[i] = move;
                    i++;
                    break;
                } else {
                    move.to.x = x1;
                    move.to.y = y1;
                    moves[i] = move;
                    i++;
                }
            }
            // Bottom
            x1 = x;
            y1 = y;
            while(y1 > 0) {
                y1--;
                Piece *next_piece = get_piece(x1, y1, b);
                if(next_piece && next_piece->color == color_to_move) {
                    break;
                } else if(next_piece && next_piece->color != color_to_move) {
                    move.to.x = x1;
                    move.to.y = y1;
                    moves[i] = move;
                    i++;
                    break;
                } else {
                    move.to.x = x1;
                    move.to.y = y1;
                    moves[i] = move;
                    i++;
                }
            }
            break;
        }
        case QUEEN:
        {
            // Left
            u32 x1 = x;
            u32 y1 = y;
            while(x1 > 0) {
                x1--;
                Piece *next_piece = get_piece(x1, y1, b);
                if(next_piece && next_piece->color == color_to_move) {
                    break;
                } else if(next_piece && next_piece->color != color_to_move) {
                    move.to.x = x1;
                    move.to.y = y1;
                    moves[i] = move;
                    i++;
                    break;
                } else {
                    move.to.x = x1;
                    move.to.y = y1;
                    moves[i] = move;
                    i++;
                }
            }
            // Top left
            x1 = x;
            y1 = y;
            while(x1 > 0 && y1 < 7) {
                x1--;
                y1++;
                Piece *next_piece = get_piece(x1, y1, b);
                if(next_piece && next_piece->color == color_to_move) {
                    break;
                } else if(next_piece && next_piece->color != color_to_move) {
                    move.to.x = x1;
                    move.to.y = y1;
                    moves[i] = move;
                    i++;
                    break;
                } else {
                    move.to.x = x1;
                    move.to.y = y1;
                    moves[i] = move;
                    i++;
                }
            }
            // Top
            x1 = x;
            y1 = y;
            while(y1 < 7) {
                y1++;
                Piece *next_piece = get_piece(x1, y1, b);
                if(next_piece && next_piece->color == color_to_move) {
                    break;
                } else if(next_piece && next_piece->color != color_to_move) {
                    move.to.x = x1;
                    move.to.y = y1;
                    moves[i] = move;
                    i++;
                    break;
                } else {
                    move.to.x = x1;
                    move.to.y = y1;
                    moves[i] = move;
                    i++;
                }
            }
            // Top right
            x1 = x;
            y1 = y;
            while(x1 < 7 && y1 < 7) {
                x1++;
                y1++;
                Piece *next_piece = get_piece(x1, y1, b);
                if(next_piece && next_piece->color == color_to_move) {
                    break;
                } else if(next_piece && next_piece->color != color_to_move) {
                    move.to.x = x1;
                    move.to.y = y1;
                    moves[i] = move;
                    i++;
                    break;
                } else {
                    move.to.x = x1;
                    move.to.y = y1;
                    moves[i] = move;
                    i++;
                }
            }
            // Right
            x1 = x;
            y1 = y;
            while(x1 < 7) {
                x1++;
                Piece *next_piece = get_piece(x1, y1, b);
                if(next_piece && next_piece->color == color_to_move) {
                    break;
                } else if(next_piece && next_piece->color != color_to_move) {
                    move.to.x = x1;
                    move.to.y = y1;
                    moves[i] = move;
                    i++;
                    break;
                } else {
                    move.to.x = x1;
                    move.to.y = y1;
                    moves[i] = move;
                    i++;
                }
            }
            // Bottom right
            x1 = x;
            y1 = y;
            while(x1 < 7 && y1 > 0) {
                x1++;
                y1--;
                Piece *next_piece = get_piece(x1, y1, b);
                if(next_piece && next_piece->color == color_to_move) {
                    break;
                } else if(next_piece && next_piece->color != color_to_move) {
                    move.to.x = x1;
                    move.to.y = y1;
                    moves[i] = move;
                    i++;
                    break;
                } else {
                    move.to.x = x1;
                    move.to.y = y1;
                    moves[i] = move;
                    i++;
                }
            }
            // Bottom
            x1 = x;
            y1 = y;
            while(y1 > 0) {
                y1--;
                Piece *next_piece = get_piece(x1, y1, b);
                if(next_piece && next_piece->color == color_to_move) {
                    break;
                } else if(next_piece && next_piece->color != color_to_move) {
                    move.to.x = x1;
                    move.to.y = y1;
                    moves[i] = move;
                    i++;
                    break;
                } else {
                    move.to.x = x1;
                    move.to.y = y1;
                    moves[i] = move;
                    i++;
                }
            }
            // Bottom left
            x1 = x;
            y1 = y;
            while(x1 > 0 && y1 > 0) {
                x1--;
                y1--;
                Piece *next_piece = get_piece(x1, y1, b);
                if(next_piece && next_piece->color == color_to_move) {
                    break;
                } else if(next_piece && next_piece->color != color_to_move) {
                    move.to.x = x1;
                    move.to.y = y1;
                    moves[i] = move;
                    i++;
                    break;
                } else {
                    move.to.x = x1;
                    move.to.y = y1;
                    moves[i] = move;
                    i++;
                }
            }
            break;
        }
        default:
        {
            break;
        }
    }

    *move_count = i;
}

// out_moves must be size 256
Move *generate_moves(i32 color_to_move, u32 *count, Board *b) {
    // TODO: custom allocator
    Move *out_moves = malloc(256 * sizeof(Move));
    if(!out_moves) {
        fprintf(stderr, "Failed to allocate 256 move buffer\n");
        exit(-1);
        return NULL;
    }
    u32 move_count = 0;

    for(u32 x = 0; x < 8; x++) {
        for(u32 y = 0; y < 8; y++) {
            u32 m = 0;
            generate_moves_for_piece(x, y, color_to_move, b, &out_moves[move_count], &m);
            move_count += m;
        }
    }

    if(count) {
        *count = move_count;
    }

    return out_moves;
}

// Check if side with specified color in check
bool is_in_check(u32 color, Board *b) {
    Move moves[256];

    for(u32 x = 0; x < 8; x++) {
        for(u32 y = 0; y < 8; y++) {
            Piece *piece = get_piece(x, y, b);
            if(piece && piece->color != color) {
                u32 move_count = 0;
                generate_moves_for_piece(x, y, piece->color, b, moves, &move_count);

                for(u32 i = 0; i < move_count; i++) {
                    Move *m = &moves[i];
                    if(color == COLOR_WHITE) {
                        if(m->to.x == b->white_king_pos.x
                            && m->to.y == b->white_king_pos.y) {
                            return true;
                        }
                    } else {
                        if(m->to.x == b->black_king_pos.x
                            && m->to.y == b->black_king_pos.y) {
                            return true;
                        }
                    }
                }
            }
        }
    }

    return false;
}

i32 minimax(Board *b, i32 depth, i32 ply_from_root, i32 alpha, i32 beta, i32 who_to_move) {
    minimax_count++;

    if(depth == 0) {
        return evaluate_board(b);
    }

    u32 move_count = 0;
    Move *minimax_moves = generate_moves(who_to_move, &move_count, b);

    u32 moves_to_remove[256];
    u32 moves_to_remove_counter = 0;

    for(i32 i = move_count - 1; i >= 0; i--) {
        Piece *p = get_piece(minimax_moves[i].to.x, minimax_moves[i].to.y, b);
        bool piece_exists = p != NULL;
        Piece p_copy;
        if(p) {
            p_copy = *p;
        }
        move_piece(
            minimax_moves[i].from.x,
            minimax_moves[i].from.y,
            minimax_moves[i].to.x,
            minimax_moves[i].to.y,
            b);
        if(is_in_check(who_to_move, b)) {
            // Can't make a move that results in check of your king!
            moves_to_remove[moves_to_remove_counter] = i;
            moves_to_remove_counter++;
        }
        move_piece(
            minimax_moves[i].to.x,
            minimax_moves[i].to.y,
            minimax_moves[i].from.x,
            minimax_moves[i].from.y,
            b);
        if(piece_exists) {
            set_piece(minimax_moves[i].to.x, minimax_moves[i].to.y, &p_copy, b);
        }
    }

    for(u32 i = 0; i < moves_to_remove_counter; i++) {
        for(u32 j = moves_to_remove[i]; j < move_count - 1; j++) {
            minimax_moves[j] = minimax_moves[j + 1];
        }
        move_count--;
    }

    // Check capture moves first
    u32 move_scores[256];
    for(u32 i = 0; i < move_count; i++) {
        if(minimax_moves[i].capture) {
            move_scores[i] = 5;
        } else {
            move_scores[i] = 1;
        }
    }

    // Bubble sort
    for(i32 i = 0; i < move_count; i++) {
        u32 swaps = 0;
        for(i32 j = 0; j < move_count - i - 1; j++) {
            if(move_scores[j] < move_scores[j + 1]) {
                Move temp = minimax_moves[j];
                minimax_moves[j] = minimax_moves[j + 1];
                minimax_moves[j + 1] = temp;

                u32 temp_score = move_scores[j];
                move_scores[j] = move_scores[j + 1];
                move_scores[j + 1] = temp_score;

                swaps = 1;
            }
        }

        if(!swaps) {
            break;
        }
    }

    bool in_check = is_in_check(who_to_move, b);
    if(move_count == 0 && in_check) {
        free(minimax_moves);
        return who_to_move == COLOR_WHITE ? INT_MIN : INT_MAX;
    } else if(move_count == 0 && !in_check) {
        free(minimax_moves);
        return 0;
    }

    if(who_to_move == COLOR_WHITE) {
        i32 max_eval = INT_MIN;
        for(u32 i = 0; i < move_count; i++) {
            Piece *p = get_piece(minimax_moves[i].to.x, minimax_moves[i].to.y, b);
            bool piece_exists = p;
            Piece p_copy;
            if(p) {
                p_copy = *p;
            }
            move_piece(
                minimax_moves[i].from.x,
                minimax_moves[i].from.y,
                minimax_moves[i].to.x,
                minimax_moves[i].to.y,
                b);
            i32 eval = minimax(b, depth - 1, ply_from_root + 1, alpha, beta, COLOR_BLACK);
            if(eval > max_eval && ply_from_root == 0) {
                best_move = minimax_moves[i];
            }
            max_eval = MAX(max_eval, eval);
            move_piece(
                minimax_moves[i].to.x,
                minimax_moves[i].to.y,
                minimax_moves[i].from.x,
                minimax_moves[i].from.y,
                b);
            if(piece_exists) {
                set_piece(minimax_moves[i].to.x, minimax_moves[i].to.y, &p_copy, b);
            }
            alpha = MAX(alpha, eval);
            if(eval >= beta) {
                break;
            }
        }
        free(minimax_moves);
        return max_eval;
    } else {
        i32 min_eval = INT_MAX;
        for(u32 i = 0; i < move_count; i++) {
            Piece *p = get_piece(minimax_moves[i].to.x, minimax_moves[i].to.y, b);
            bool piece_exists = p;
            Piece p_copy;
            if(p) {
                p_copy = *p;
            }
            move_piece(
                minimax_moves[i].from.x,
                minimax_moves[i].from.y,
                minimax_moves[i].to.x,
                minimax_moves[i].to.y,
                b);
            i32 eval = minimax(b, depth - 1, ply_from_root + 1, alpha, beta, COLOR_WHITE);
            if(eval < min_eval && ply_from_root == 0) {
                best_move = minimax_moves[i];
            }
            min_eval = MIN(min_eval, eval);
            move_piece(
                minimax_moves[i].to.x,
                minimax_moves[i].to.y,
                minimax_moves[i].from.x,
                minimax_moves[i].from.y,
                b);
            if(piece_exists) {
                set_piece(minimax_moves[i].to.x, minimax_moves[i].to.y, &p_copy, b);
            }
            beta = MIN(beta, eval);
            if(eval <= alpha) {
                break;
            }
        }
        free(minimax_moves);
        return min_eval;
    }
}

int main() {
    setup_board(&board);
    memcpy(&search_board, &board, sizeof(board));

    for(u32 i = 0; i < 50; i++) {
        minimax(&search_board, 6, 0, INT_MIN, INT_MAX, i % 2);
        move_piece(best_move.from.x, best_move.from.y, best_move.to.x, best_move.to.y, &board);
        memcpy(&search_board, &board, sizeof(board));
        printf("Half move %u\n", i + 1);
        printf("Evaluated %u positions\n", minimax_count);
        minimax_count = 0;
        print_board(&board);
    }

    return 0;
}