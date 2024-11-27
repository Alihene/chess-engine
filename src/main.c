#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

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
} Move;

#define COLOR_WHITE 0
#define COLOR_BLACK 1

typedef struct {
    Piece pieces[64];
    u64 pieces_state;
} Board;

Board board = {0};
Board search_board = {0};

Piece *get_piece(u32 x, u32 y, Board *b) {
    u32 index = x + y * 8;
    if(b->pieces_state & (1L << index)) {
        return &b->pieces[x + y * 8];
    }

    return NULL;
}

void set_piece(u32 x, u32 y, const Piece *piece, Board *b) {
    b->pieces[x + y * 8] = *piece;
    b->pieces_state |= (1L << (x + y * 8));
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

    for(u32 x = 0; x < 8; x++) {
        for(u32 y = 0; y < 8; y++) {
            Piece *piece = get_piece(x, y, b);
            if(piece) {
                i32 value;
                switch(piece->type) {
                    case KING:
                    {
                        value = 4;
                        break;
                    }
                    case PAWN:
                    {
                        value = 1;
                        break;
                    }
                    case BISHOP:
                    case KNIGHT:
                    {
                        value = 3;
                        break;
                    }
                    case ROOK:
                    {
                        value = 5;
                        break;
                    }
                    case QUEEN:
                    {
                        value = 9;
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

    return white - black;
}

void generate_moves_for_piece(u32 x, u32 y, u32 color_to_move, Board *b, Move *moves, u32 *move_count) {
    if(!(b->pieces_state & (1L << (x + y * 8)))) {
        *move_count = 0;
        return;
    }

    const Piece *piece = &b->pieces[x + y * 8];
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
    switch(piece->type) {
        case KING:
        {
            // Left
            if(x > 0 && !(get_piece(x - 1, y, b) && get_piece(x - 1, y, b)->color == color_to_move)) {
                move.to.x = x - 1;
                moves[i] = move;
                i++;
            }

            // Top left
            if(x > 0 && y < 7 && !(get_piece(x - 1, y + 1, b) && get_piece(x - 1, y + 1, b)->color == color_to_move)) {
                move.to.x = x - 1;
                move.to.y = y + 1;
                moves[i] = move;
                i++;
            }

            // Top
            if(y < 7 && !(get_piece(x, y + 1, b) && get_piece(x, y + 1, b)->color == color_to_move)) {
                move.to.y = y + 1;
                moves[i] = move;
                i++;
            }

            // Top right
            if(x < 7 && y < 7 && !(get_piece(x + 1, y + 1, b) && get_piece(x + 1, y + 1, b)->color == color_to_move)) {
                move.to.x = x + 1;
                move.to.y = y + 1;
                moves[i] = move;
                i++;
            }

            // Right
            if(x < 7 && !(get_piece(x + 1, y, b) && get_piece(x + 1, y, b)->color == color_to_move)) {
                move.to.x = x + 1;
                moves[i] = move;
                i++;
            }

            // Bottom right
            if(x < 7 && y > 0 && !(get_piece(x + 1, y - 1, b) && get_piece(x + 1, y - 1, b)->color == color_to_move)) {
                move.to.x = x + 1;
                move.to.y = y - 1;
                moves[i] = move;
                i++;
            }

            // Bottom
            if(y > 0 && !(get_piece(x, y - 1, b) && get_piece(x, y - 1, b)->color == color_to_move)) {
                move.to.y = y - 1;
                moves[i] = move;
                i++;
            }

            // Bottom left
            if(x > 0 && y > 0 && !(get_piece(x - 1, y - 1, b) && get_piece(x - 1, y - 1, b)->color == color_to_move)) {
                move.to.x = x - 1;
                move.to.y = y - 1;
                moves[i] = move;
                i++;
            }
            break;
        }
        case PAWN:
        {
            if(color_to_move == COLOR_WHITE) {
                // One square
                if(y < 7 && !(get_piece(x, y + 1, b) && get_piece(x, y + 1, b)->color == color_to_move)) {
                    move.to.y = y + 1;
                    moves[i] = move;
                    i++;
                }
                // Two squares
                if(y == 1
                    && !(get_piece(x, 2, b) && get_piece(x, 2, b)->color == color_to_move)
                    && !(get_piece(x, 3, b) && get_piece(x, 3, b)->color == color_to_move)) {
                    move.to.y = 3;
                    moves[i] = move;
                    i++;
                }
            } else {
                // One square
                if(y > 0 && !(get_piece(x, y - 1, b) && get_piece(x, y - 1, b)->color == color_to_move)) {
                    move.to.y = y - 1;
                    moves[i] = move;
                    i++;
                }
                // Two squares
                if(y == 6
                    && !(get_piece(x, 5, b) && get_piece(x, 5, b)->color == color_to_move)
                    && !(get_piece(x, 4, b) && get_piece(x, 4, b)->color == color_to_move)) {
                    move.to.y = 4;
                    moves[i] = move;
                    i++;
                }
            }
            break;
        }
        case BISHOP:
        {
            // Top left
            u32 x1 = x;
            u32 y1 = y;
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
        case KNIGHT:
        {
            // Left 1
            if(x > 1 && y > 0 && !(get_piece(x - 2, y - 1, b) && get_piece(x - 2, y - 1, b)->color == color_to_move)) {
                move.to.x = x - 2;
                move.to.y = y - 1;
                moves[i] = move;
                i++;
            }
            // Left 2
            if(x > 1 && y < 7 && !(get_piece(x - 2, y + 1, b) && get_piece(x - 2, y + 1, b)->color == color_to_move)) {
                move.to.x = x - 2;
                move.to.y = y + 1;
                moves[i] = move;
                i++;
            }
            // Top 1
            if(x > 0 && y < 6 && !(get_piece(x - 1, y + 2, b) && get_piece(x - 1, y + 2, b)->color == color_to_move)) {
                move.to.x = x - 1;
                move.to.y = y + 2;
                moves[i] = move;
                i++;
            }
            // Top 2
            if(x < 7 && y < 6 && !(get_piece(x + 1, y + 2, b) && get_piece(x + 1, y + 2, b)->color == color_to_move)) {
                move.to.x = x + 1;
                move.to.y = y + 2;
                moves[i] = move;
                i++;
            }
            // Right 1
            if(x < 6 && y < 7 && !(get_piece(x + 2, y + 1, b) && get_piece(x + 2, y + 1, b)->color == color_to_move)) {
                move.to.x = x + 2;
                move.to.y = y + 1;
                moves[i] = move;
                i++;
            }
            // Right 2
            if(x < 6 && y > 0 && !(get_piece(x + 2, y - 1, b) && get_piece(x + 2, y - 1, b)->color == color_to_move)) {
                move.to.x = x + 2;
                move.to.y = y - 1;
                moves[i] = move;
                i++;
            }
            // Bottom 1
            if(x < 7 && y > 1 && !(get_piece(x + 1, y - 2, b) && get_piece(x + 1, y - 2, b)->color == color_to_move)) {
                move.to.x = x + 1;
                move.to.y = y - 2;
                moves[i] = move;
                i++;
            }
            // Bottom 2
            if(x > 0 && y > 1 && !(get_piece(x - 1, y - 2, b) && get_piece(x - 1, y - 2, b)->color == color_to_move)) {
                move.to.x = x - 1;
                move.to.y = y - 2;
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
void generate_moves(u32 color_to_move, Move *out_moves, u32 *count, Board *b) {
    Move moves[256];
    u32 move_count = 0;

    for(u32 x = 0; x < 8; x++) {
        for(u32 y = 0; y < 8; y++) {
            u32 m = 0;
            generate_moves_for_piece(x, y, COLOR_WHITE, b, moves, &m);
            move_count += m;
        }
    }

    if(out_moves) {
        memcpy(out_moves, moves, sizeof(moves));
    }
    if(count) {
        *count = move_count;
    }
}

i32 negamax(u32 depth) {
    if(depth == 0) {
        
    }
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

int main() {
    setup_board(&board);
    memcpy(&search_board, &board, sizeof(board));

    Move moves[256];
    generate_moves(COLOR_WHITE, NULL, NULL, &board);

    print_board(&board);
    return 0;
}