

#include "address_map_arm.h"
#include <stdlib.h>
#include <stdio.h>

#define STANDARD_X 320
#define STANDARD_Y 240
#define CAR_WIDTH 16
#define CAR_HEIGHT 20
#define OBSTACLE_WIDTH 16
#define OBSTACLE_HEIGHT 20
#define GAME_SPEED 4
#define TRACK_SEGMENTS 30
#define TRACK_WIDTH 120
#define CAR_Y (GAME_AREA_Y2 - CAR_HEIGHT - 2)
#define BLACK 0x0000  // OK: R=0, G=0, B=0
#define WHITE 0xFFFF  // OK: R=31, G=63, B=31
#define RED   0xF800  // OK: R=31, G=0, B=0
#define GREEN 0x07E0  // OK: R=0, G=63, B=0
#define BLUE  0x001F  // Faltava: R=0, G=0, B=31
#define GRAY  0x8410  // Correção: R=16, G=32, B=16 (aproximado cinza médio)

#define NUM_OBSTACLES 1
#define GAME_AREA_X1 60
#define GAME_AREA_X2 260
#define GAME_AREA_Y1 20
#define GAME_AREA_Y2 220
#define GAME_AREA_WIDTH (GAME_AREA_X2 - GAME_AREA_X1)
#define GAME_AREA_HEIGHT (GAME_AREA_Y2 - GAME_AREA_Y1)

int screen_x;
int screen_y;
int res_offset;
int col_offset;
int db;
int tunel = 0;

int car_x = 160;
int time_seconds = 0;
int beam_depth    = 40;   
int beam_spread   = 30;     
int km = 0;
int track_left[TRACK_SEGMENTS];
int track_right[TRACK_SEGMENTS];
int obstacle_x[NUM_OBSTACLES];
int obstacle_y[NUM_OBSTACLES];

void video_text(int, int, char *);
void video_box(int, int, int, int, short);
void clear_screen(void);
void clear_text(void);
void draw_car(int, int);
void draw_obstacle_car(int, int);
int  resample_rgb(int, int);
int  get_data_bits(int);
void delay(int);
int  random_range(int, int);
int rgb888_to_rgb565(int);

void init_track();
void scroll_track();
void draw_track();
int check_border_collision();
int check_obstacle_collision();
void draw_status();

int main(void) {

                             // segmentos
                          // pixels
    int segment_h     = GAME_AREA_HEIGHT / TRACK_SEGMENTS;  // = 200/30 ≃ 6 px
    int cone_height   = beam_depth * segment_h;        // ≃ 10*6 = 60 px
    int cone_top      = GAME_AREA_Y2 - cone_height;    // 220–60 = 160

    volatile int * video_resolution = (int *)(PIXEL_BUF_CTRL_BASE + 0x8);
    screen_x = *video_resolution & 0xFFFF;
    screen_y = (*video_resolution >> 16) & 0xFFFF;

    volatile int * rgb_status = (int *)(RGB_RESAMPLER_BASE);
    db = get_data_bits(*rgb_status & 0x3F);

    res_offset = (screen_x == 160) ? 1 : 0;
    col_offset = (db == 8) ? 1 : 0;
    srand(42);

    volatile int * key_ptr = (int *)KEY_BASE;

    while (1) {
        car_x = GAME_AREA_X1 + (GAME_AREA_WIDTH - CAR_WIDTH) / 2;
        time_seconds = 0;
        km = 0;

        init_track();

        // clear_screen();
        clear_text();
        video_text(10, 10, "FPGA STOCK CAR");
        video_text(10, 12, "KEY0 = Left | KEY1 = Right");
        video_text(10, 14, "Press KEY2 to Start");
        while ((*key_ptr & 0x4) != 0);
        while ((*key_ptr & 0x4) == 0);

        int frame = 0;
        int game_over = 0;

        while (!game_over) {
            if ((*key_ptr & 0x1) == 0) car_x -= 6;
            if ((*key_ptr & 0x2) == 0) car_x += 6;
            if (car_x < GAME_AREA_X1) car_x = GAME_AREA_X1;
            if (car_x > GAME_AREA_X2 - CAR_WIDTH) car_x = GAME_AREA_X2 - CAR_WIDTH;

            scroll_track();
            for (int i = 0; i < NUM_OBSTACLES; i++) {
                obstacle_y[i] += 8;
                if (obstacle_y[i] > STANDARD_Y) {
                    obstacle_y[i] = 0;
                    obstacle_x[i] = track_left[0] + random_range(10, TRACK_WIDTH - OBSTACLE_WIDTH - 10);
                }
            }

            if (check_border_collision() || check_obstacle_collision()) {
                game_over = 1;
                continue;
            }

            clear_screen();
            draw_track();
            draw_car(car_x, CAR_Y);
            for (int i = 0; i < NUM_OBSTACLES; i++) {
                if (tunel == 1) {
                    int center_x = car_x + CAR_WIDTH/2;
                    int left_beam  = center_x - beam_spread;
                    int right_beam = center_x + beam_spread;
                    int oy = obstacle_y[i];
                    int ox = obstacle_x[i];

                    // dentro do alcance vertical do farol
                    if (oy >= cone_top && oy <= GAME_AREA_Y2) {
                        // obstáculo horizontalmente dentro do cone?
                        if ( (ox + OBSTACLE_WIDTH) >= left_beam &&
                            ox               <= right_beam ) {
                            draw_obstacle_car(ox, oy);
                        }
                    }
                } else {
                    draw_obstacle_car(obstacle_x[i], obstacle_y[i]);
                }
            }
            draw_status();
            delay(100);
            frame++;
            if (frame % 10 == 0) {
                time_seconds++;
                km++;

                if (time_seconds%10 == 0)
                    tunel = !tunel; 
            }
        }
        tunel = 0;
        clear_screen();
        clear_text();
        video_text(10, 10, "GAME OVER");
        video_text(10, 12, "Press KEY2 to Restart");
        while ((*key_ptr & 0x4) != 0);
        while ((*key_ptr & 0x4) == 0);
    }

    return 0;
}

void init_track() {
    int mid = GAME_AREA_X1 + (GAME_AREA_WIDTH - TRACK_WIDTH) / 2;
    for (int i = 0; i < TRACK_SEGMENTS; i++) {
        track_left[i] = mid + random_range(-10, 10);
        if (track_left[i] < GAME_AREA_X1 + 10) track_left[i] = GAME_AREA_X1 + 10;
        if (track_left[i] > GAME_AREA_X2 - TRACK_WIDTH - 10) track_left[i] = GAME_AREA_X2 - TRACK_WIDTH - 10;
        track_right[i] = track_left[i] + TRACK_WIDTH;
    }
    for (int i = 0; i < NUM_OBSTACLES; i++) {
        obstacle_x[i] = track_left[0] + random_range(10, TRACK_WIDTH - OBSTACLE_WIDTH - 10);
        obstacle_y[i] = random_range(GAME_AREA_Y1, GAME_AREA_Y2 / 2);
    }
}

void scroll_track() {
    for (int i = TRACK_SEGMENTS - 1; i > 0; i--) {
        track_left[i] = track_left[i - 1];
        track_right[i] = track_right[i - 1];
    }
    int new_left = track_left[1] + random_range(-8, 8);
    if (new_left < GAME_AREA_X1 + 10) new_left = GAME_AREA_X1 + 10;
    if (new_left > GAME_AREA_X2 - TRACK_WIDTH - 10) new_left = GAME_AREA_X2 - TRACK_WIDTH - 10;
    track_left[0] = new_left;
    track_right[0] = new_left + TRACK_WIDTH;
}

void draw_track() {
    int segment_height = GAME_AREA_HEIGHT / TRACK_SEGMENTS;
    short wall_color      = resample_rgb(db, WHITE);
    short beam_bg_color   = resample_rgb(db, WHITE);
    short beam_wall_color = resample_rgb(db, BLACK);
    int start_seg = TRACK_SEGMENTS - beam_depth;
    for (int i = 0; i < TRACK_SEGMENTS; i++) {
        int y = GAME_AREA_Y1 + i * segment_height;

        if (tunel == 1) {
            // apenas os últimos beam_depth segmentos iluminados
            if (i < TRACK_SEGMENTS - beam_depth)
                continue;

            int center_x = car_x + CAR_WIDTH / 2;
            int d = i - (TRACK_SEGMENTS - beam_depth);  // 0..beam_depth-1
            // largura proporcional: 0 no longe, max no próximo ao carro
            int spread = (beam_spread * (d + 1)) / beam_depth;
            int left_beam  = center_x - spread;
            int right_beam = center_x + spread;

            // desenha fundo branco dentro do cone (farois)
           if (i >= start_seg) {
                int rel = i - start_seg;  
                // agora: faróis mais largos longe e mais estreitos perto do carro
                int spread_i = (beam_spread * (beam_depth - rel)) / beam_depth;
                int left_beam  = center_x - spread_i;
                int right_beam = center_x + spread_i;

                video_box(
                    left_beam,
                    y,
                    right_beam,
                    y + segment_height - 1,
                    beam_bg_color
                );
            }

            // desenha as paredes como linhas pretas dentro do cone
            if (track_left[i] >= left_beam && track_left[i] <= right_beam) {
                video_box(
                    track_left[i],
                    y,
                    track_left[i],
                    y + segment_height - 1,
                    beam_wall_color
                );
            }
            if (track_right[i] >= left_beam && track_right[i] <= right_beam) {
                video_box(
                    track_right[i],
                    y,
                    track_right[i],
                    y + segment_height - 1,
                    beam_wall_color
                );
            }

        } else {
            // modo normal, desenha toda a pista
            video_box(
                GAME_AREA_X1,
                y,
                track_left[i],
                y + segment_height - 1,
                wall_color
            );
            video_box(
                track_right[i],
                y,
                GAME_AREA_X2,
                y + segment_height - 1,
                wall_color
            );
        }
    }
}





int check_border_collision() {
    int segment = TRACK_SEGMENTS - 1;
    if (car_x < track_left[segment] || car_x + CAR_WIDTH > track_right[segment])
        return 1;
    return 0;
}

int check_obstacle_collision() {
    for (int i = 0; i < NUM_OBSTACLES; i++) {
        int x = obstacle_x[i];
        int y = obstacle_y[i];
        if (car_x < x + OBSTACLE_WIDTH && car_x + CAR_WIDTH > x &&
            CAR_Y < y + OBSTACLE_HEIGHT && CAR_Y + CAR_HEIGHT > y) {
            return 1;
        }
    }
    return 0;
}

void draw_status() {
    char buffer[40];
    sprintf(buffer, "TEMPO: %02d", time_seconds);
    video_text(1, 1, buffer);
    sprintf(buffer, "VELOCIDADE: %d", GAME_SPEED + km / 10);
    video_text(1, 2, buffer);
    sprintf(buffer, "KM: %02d.%02d", km / 10, km % 10);
    video_text(1, 3, buffer);
}

int random_range(int min, int max) {
    return min + (rand() % (max - min + 1));
}

void delay(int count) {
    volatile int i;
    for (i = 0; i < count; i++);
}

int resample_rgb(int num_bits, int color) {
    int r = (color >> 16) & 0xFF;
    int g = (color >> 8) & 0xFF;
    int b = color & 0xFF;

    if (num_bits == 8) {
        // Formato RGB332
        return ((r & 0xE0)     ) |
               ((g & 0xE0) >> 3) |
               ((b & 0xC0) >> 6);
    } else if (num_bits == 16) {
        // Formato RGB565
        return ((r & 0xF8) << 8) |
               ((g & 0xFC) << 3) |
               (b >> 3);
    } else {
        // Se o formato for desconhecido, retorna branco por segurança
        return 0xFFFF;
    }
}


int rgb888_to_rgb565(int color) {
    int r = (color >> 16) & 0xFF;
    int g = (color >> 8) & 0xFF;
    int b = color & 0xFF;

    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

int get_data_bits(int mode) {
    switch (mode) {
        case 0x0: return 1;
        case 0x7:
        case 0x11: return 8;
        case 0x12: return 9;
        case 0x14: return 16;
        case 0x17: return 24;
        case 0x19: return 30;
        case 0x31: return 8;
        case 0x32: return 12;
        case 0x33: return 16;
        case 0x37: return 32;
        case 0x39: return 40;
        default: return 16;
    }
}

void clear_screen(void) {
    video_box(0, 0, STANDARD_X, STANDARD_Y, resample_rgb(db, BLACK));
}

void clear_text(void) {
    for (int i = 0; i < 30; i++)
        video_text(1, i, "                              ");
}

void video_text(int x, int y, char * text_ptr) {
    int offset;
    volatile char * character_buffer = (char *)FPGA_CHAR_BASE;
    offset = (y << 7) + x;
    while (*(text_ptr)) {
        *(character_buffer + offset) = *(text_ptr);
        ++text_ptr;
        ++offset;
    }
}

void video_box(int x1, int y1, int x2, int y2, short pixel_color) {
    int pixel_buf_ptr = *(int *)PIXEL_BUF_CTRL_BASE;
    int pixel_ptr, row, col;
    int x_factor = 0x1 << (res_offset + col_offset);
    int y_factor = 0x1 << (res_offset);
    x1 = x1 / x_factor;
    x2 = x2 / x_factor;
    y1 = y1 / y_factor;
    y2 = y2 / y_factor;

    for (row = y1; row <= y2; row++)
        for (col = x1; col <= x2; ++col) {
            pixel_ptr = pixel_buf_ptr + (row << (10 - res_offset - col_offset)) + (col << 1);
            *(short *)pixel_ptr = pixel_color;
        }
}

void draw_car(int x, int y) {
    short color = resample_rgb(db,RED); 
    const unsigned char car[20][16] = {
    {0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,0,1,1,0,1,0,0,0,0,0},
    {0,0,0,0,1,1,0,1,1,0,1,1,0,0,0,0},
    {0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,0,0,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,0,0,1,1,0,0,0,0,0},
    {0,0,0,0,1,1,1,0,0,1,1,1,0,0,0,0},
    {0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0},
};


    for (int row = 0; row < 20; row++) {
        for (int col = 0; col < 16; col++) {
            if (car[row][col] == 1) {
                int x1 = x + col;
                int y1 = y + row;
                video_box(x1, y1, x1, y1, color);
            }
        }
    }
}

void draw_obstacle_car(int x, int y) {
    short color = resample_rgb(db, GREEN); 
    const unsigned char car[20][16] = {
    {0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,0,0,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,0,0,0,0,1,0,0,0,0,0},
    {0,0,0,0,0,1,0,0,0,0,1,0,0,0,0,0},
    {0,0,0,0,0,1,0,0,0,0,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,0,0,0,0,1,0,0,0,0,0},
    {0,0,0,0,0,1,0,0,0,0,1,0,0,0,0,0},
    {0,0,0,0,0,1,0,0,0,0,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
};

    for (int row = 0; row < 20; row++) {
        for (int col = 0; col < 16; col++) {
            if (car[row][col] == 1) {
                int x1 = x + col;
                int y1 = y + row;
                video_box(x1, y1, x1, y1, color);
            }
        }
    }
}
