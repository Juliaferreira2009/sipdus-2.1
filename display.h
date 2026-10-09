#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <lvgl.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include <math.h>

// ============================================================
// CONFIGURACAO
// ============================================================

#define SCREEN_WIDTH   320
#define SCREEN_HEIGHT  240

// Apenas um buffer para economizar RAM
#define BUFFER_LINES 20

#define MAX_MEASUREMENTS 20

// ============================================================
// CORES
// ============================================================

// Tela principal
#define COLOR_YELLOW        0xE3C82B
#define COLOR_YELLOW_DARK    0xBFA624
#define COLOR_WHITE         0xFFFFFF
#define COLOR_BLACK         0x000000
#define COLOR_BACKGROUND    0xF7F7F7
#define COLOR_TEXT          0x333333
#define COLOR_TEXT_LIGHT    0x666666
#define COLOR_TEXT_GRAY     0x777777

// Tela de calibracao
// SOMENTE AZUL E BRANCO
#define COLOR_BLUE          0x1976D2

// ============================================================
// CALIBRACAO
// ============================================================

#define SPLASH_MS               10000UL

#define LOADING_SIZE            10
#define LOADING_RADIUS          24

#define LOADING_CENTER_X        160
#define LOADING_CENTER_Y        165

#define LOADING_INTERVAL_MS     80

#define LOADING_RING_SIZE       58
#define LOADING_RING_WIDTH      3

// ============================================================
// TOUCHSCREEN
// ============================================================

#define XPT2046_IRQ   36
#define XPT2046_MOSI  32
#define XPT2046_MISO  39
#define XPT2046_CLK   25
#define XPT2046_CS    33

#define TOUCH_X_MIN   200
#define TOUCH_X_MAX   3700
#define TOUCH_Y_MIN   240
#define TOUCH_Y_MAX   3800

// ============================================================
// HARDWARE
// ============================================================

static SPIClass touchscreenSPI = SPIClass(VSPI);

static XPT2046_Touchscreen touchscreen(
    XPT2046_CS,
    XPT2046_IRQ
);

static TFT_eSPI tft = TFT_eSPI();

// ============================================================
// LVGL
// ============================================================

static lv_disp_draw_buf_t draw_buf;

static lv_color_t buf1[
    SCREEN_WIDTH * BUFFER_LINES
];

// ============================================================
// TELAS
// ============================================================

static lv_obj_t *scr_splash = NULL;
static lv_obj_t *scr_main = NULL;
static lv_obj_t *scr_history = NULL;

// ============================================================
// TELA PRINCIPAL
// ============================================================

static lv_obj_t *label_bpm = NULL;
static lv_obj_t *label_spo2 = NULL;
static lv_obj_t *label_glucose = NULL;
static lv_obj_t *label_bpm_status = NULL;
static lv_obj_t *label_spo2_status = NULL;
static lv_obj_t *label_glucose_status = NULL;
static lv_obj_t *label_counter = NULL;

// ============================================================
// HISTORICO
// ============================================================

static lv_obj_t *history_title = NULL;
static lv_obj_t *history_average = NULL;
static lv_obj_t *history_average_status = NULL;
static lv_obj_t *history_chart = NULL;

static lv_chart_series_t *history_series = NULL;

// ============================================================
// CALIBRACAO
// ============================================================

static lv_obj_t *loading_ring = NULL;
static lv_obj_t *loading_dot = NULL;

// ============================================================
// ESTADOS
// ============================================================

static unsigned long splash_start_ms = 0;
static unsigned long last_loading_update = 0;

static int loading_angle = 0;

static bool splash_finished = false;
static bool history_screen_open = false;
static bool final_screen_shown = false;

static bool touch_was_pressed = false;

// ============================================================
// HISTORICO
// ============================================================

static float glucose_history[MAX_MEASUREMENTS];

static int measurement_count = 0;

static float final_average = 0.0;

// ============================================================
// CONTROLE DE ATUALIZACAO
// ============================================================

static int last_display_bpm = -999;
static int last_display_spo2 = -999;
static int last_display_glucose = -999;
static int last_display_measurement_count = -1;

// ============================================================
// VARIAVEIS DO .INO
// ============================================================

extern int latest_bpm;
extern float latest_spo2;
extern float latest_glucose;

// ============================================================
// PROTOTIPOS
// ============================================================

static void open_history_screen();
static void update_measurement_counter();
static void update_history_chart();
static float calculate_average();
static void update_history_average_status();
static const char *get_glucose_status(float glucose);


// ============================================================
// DISPLAY FLUSH
// ============================================================

static void my_disp_flush(
    lv_disp_drv_t *disp,
    const lv_area_t *area,
    lv_color_t *color_p
)
{
    uint32_t w =
        area->x2 - area->x1 + 1;

    uint32_t h =
        area->y2 - area->y1 + 1;

    tft.startWrite();

    tft.setAddrWindow(
        area->x1,
        area->y1,
        w,
        h
    );

    tft.pushColors(
        (uint16_t *)&color_p->full,
        w * h,
        true
    );

    tft.endWrite();

    lv_disp_flush_ready(disp);
}


// ============================================================
// TOUCHSCREEN
// ============================================================

static void touchscreen_read(
    lv_indev_drv_t *indev_driver,
    lv_indev_data_t *data
)
{
    if (touchscreen.touched())
    {
        TS_Point p =
            touchscreen.getPoint();

        int x = map(
            p.x,
            TOUCH_X_MIN,
            TOUCH_X_MAX,
            0,
            SCREEN_WIDTH - 1
        );

        int y = map(
            p.y,
            TOUCH_Y_MIN,
            TOUCH_Y_MAX,
            0,
            SCREEN_HEIGHT - 1
        );

        x = constrain(
            x,
            0,
            SCREEN_WIDTH - 1
        );

        y = constrain(
            y,
            0,
            SCREEN_HEIGHT - 1
        );

        data->state =
            LV_INDEV_STATE_PRESSED;

        data->point.x = x;
        data->point.y = y;
    }
    else
    {
        data->state =
            LV_INDEV_STATE_RELEASED;
    }
}


// ============================================================
// BOTAO
// ============================================================

static lv_obj_t *create_button(
    lv_obj_t *parent,
    const char *text,
    int x,
    int y,
    int width,
    int height
)
{
    lv_obj_t *btn =
        lv_btn_create(parent);

    lv_obj_set_size(
        btn,
        width,
        height
    );

    lv_obj_set_pos(
        btn,
        x,
        y
    );

    lv_obj_set_style_radius(
        btn,
        10,
        0
    );

    lv_obj_set_style_bg_color(
        btn,
        lv_color_hex(COLOR_YELLOW),
        0
    );

    lv_obj_set_style_bg_opa(
        btn,
        LV_OPA_COVER,
        0
    );

    lv_obj_set_style_border_width(
        btn,
        0,
        0
    );

    lv_obj_t *label =
        lv_label_create(btn);

    lv_label_set_text(
        label,
        text
    );

    lv_obj_set_style_text_color(
        label,
        lv_color_hex(COLOR_BLACK),
        0
    );

    lv_obj_set_style_text_font(
        label,
        &lv_font_montserrat_12,
        0
    );

    lv_obj_center(label);

    return btn;
}


// ============================================================
// ICONE CORACAO
// ============================================================
//
// Nao utiliza LV_SYMBOL_HEART.
// O desenho e feito com objetos LVGL.
// ============================================================

static void draw_heart(
    lv_obj_t *parent,
    int x,
    int y
)
{
    // Circulo esquerdo
    lv_obj_t *left =
        lv_obj_create(parent);

    lv_obj_set_size(
        left,
        17,
        17
    );

    lv_obj_set_pos(
        left,
        x + 1,
        y
    );

    lv_obj_set_style_radius(
        left,
        LV_RADIUS_CIRCLE,
        0
    );

    lv_obj_set_style_bg_color(
        left,
        lv_color_hex(COLOR_WHITE),
        0
    );

    lv_obj_set_style_bg_opa(
        left,
        LV_OPA_COVER,
        0
    );

    lv_obj_set_style_border_width(
        left,
        0,
        0
    );


    // Circulo direito
    lv_obj_t *right =
        lv_obj_create(parent);

    lv_obj_set_size(
        right,
        17,
        17
    );

    lv_obj_set_pos(
        right,
        x + 13,
        y
    );

    lv_obj_set_style_radius(
        right,
        LV_RADIUS_CIRCLE,
        0
    );

    lv_obj_set_style_bg_color(
        right,
        lv_color_hex(COLOR_WHITE),
        0
    );

    lv_obj_set_style_bg_opa(
        right,
        LV_OPA_COVER,
        0
    );

    lv_obj_set_style_border_width(
        right,
        0,
        0
    );


    // Corpo central
    lv_obj_t *body =
        lv_obj_create(parent);

    lv_obj_set_size(
        body,
        27,
        25
    );

    lv_obj_set_pos(
        body,
        x + 2,
        y + 8
    );

    lv_obj_set_style_radius(
        body,
        5,
        0
    );

    lv_obj_set_style_bg_color(
        body,
        lv_color_hex(COLOR_WHITE),
        0
    );

    lv_obj_set_style_bg_opa(
        body,
        LV_OPA_COVER,
        0
    );

    lv_obj_set_style_border_width(
        body,
        0,
        0
    );

    lv_obj_set_style_transform_angle(
        body,
        450,
        0
    );
}


// ============================================================
// ICONE O2
// ============================================================

static void draw_o2(
    lv_obj_t *parent,
    int x,
    int y
)
{
    lv_obj_t *label =
        lv_label_create(parent);

    lv_label_set_text(
        label,
        "O2"
    );

    lv_obj_set_style_text_color(
        label,
        lv_color_hex(COLOR_WHITE),
        0
    );

    lv_obj_set_style_text_font(
        label,
        &lv_font_montserrat_16,
        0
    );

    lv_obj_set_pos(
        label,
        x,
        y
    );
}


// ============================================================
// ICONE GOTA
// ============================================================

static void draw_drop(
    lv_obj_t *parent,
    int x,
    int y
)
{
    // Corpo da gota
    lv_obj_t *drop =
        lv_obj_create(parent);

    lv_obj_set_size(
        drop,
        25,
        30
    );

    lv_obj_set_pos(
        drop,
        x + 1,
        y
    );

    lv_obj_set_style_radius(
        drop,
        12,
        0
    );

    lv_obj_set_style_bg_color(
        drop,
        lv_color_hex(COLOR_WHITE),
        0
    );

    lv_obj_set_style_bg_opa(
        drop,
        LV_OPA_COVER,
        0
    );

    lv_obj_set_style_border_width(
        drop,
        0,
        0
    );

    lv_obj_set_style_transform_angle(
        drop,
        450,
        0
    );
}


// ============================================================
// CARD
// ============================================================

static lv_obj_t *create_card(
    lv_obj_t *parent,
    int x,
    int y,
    int width,
    int height
)
{
    lv_obj_t *card =
        lv_obj_create(parent);

    lv_obj_set_size(
        card,
        width,
        height
    );

    lv_obj_set_pos(
        card,
        x,
        y
    );

    lv_obj_set_style_radius(
        card,
        12,
        0
    );

    lv_obj_set_style_bg_color(
        card,
        lv_color_hex(COLOR_WHITE),
        0
    );

    lv_obj_set_style_bg_opa(
        card,
        LV_OPA_COVER,
        0
    );

    lv_obj_set_style_border_width(
        card,
        0,
        0
    );

    lv_obj_set_style_shadow_width(
        card,
        5,
        0
    );

    lv_obj_set_style_shadow_opa(
        card,
        LV_OPA_20,
        0
    );

    return card;
}


// ============================================================
// LABEL DE STATUS (AMARELO ESCURO)
// ============================================================

static lv_obj_t *create_status_label(lv_obj_t *parent)
{
    lv_obj_t *status = lv_label_create(parent);

    lv_label_set_text(status, "Sem leitura");
    lv_obj_set_width(status, 86);
    lv_obj_set_height(status, 30);
    lv_label_set_long_mode(status, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(
        status,
        lv_color_hex(COLOR_YELLOW_DARK),
        0
    );
    lv_obj_set_style_text_font(
        status,
        &lv_font_montserrat_10,
        0
    );
    lv_obj_set_style_text_align(
        status,
        LV_TEXT_ALIGN_CENTER,
        0
    );
    lv_obj_set_pos(status, 2, 84);
    return status;
}


// ============================================================
// SPLASH / CALIBRANDO
// ============================================================

static void lv_create_splash()
{
    scr_splash =
        lv_obj_create(NULL);

    // FUNDO BRANCO
    lv_obj_set_style_bg_color(
        scr_splash,
        lv_color_hex(COLOR_WHITE),
        0
    );

    lv_obj_set_style_bg_opa(
        scr_splash,
        LV_OPA_COVER,
        0
    );


    // --------------------------------------------------------
    // TITULO
    // --------------------------------------------------------

    lv_obj_t *title =
        lv_label_create(scr_splash);

    lv_label_set_text(
        title,
        "CALIBRANDO..."
    );

    // AZUL
    lv_obj_set_style_text_color(
        title,
        lv_color_hex(COLOR_BLUE),
        0
    );

    lv_obj_set_style_text_font(
        title,
        &lv_font_montserrat_24,
        0
    );

    lv_obj_align(
        title,
        LV_ALIGN_CENTER,
        0,
        -65
    );


    // --------------------------------------------------------
    // TEXTO
    // --------------------------------------------------------

    lv_obj_t *subtitle =
        lv_label_create(scr_splash);

    lv_label_set_text(
        subtitle,
        "Nosso dispositivo para melhor precisao"
    );

    // AMARELO ESCURO: mesma paleta do dispositivo
    lv_obj_set_style_text_color(
        subtitle,
        lv_color_hex(COLOR_YELLOW_DARK),
        0
    );

    lv_obj_set_style_text_font(
        subtitle,
        &lv_font_montserrat_12,
        0
    );

    lv_obj_set_style_text_align(
        subtitle,
        LV_TEXT_ALIGN_CENTER,
        0
    );

    lv_obj_set_width(
        subtitle,
        290
    );

    lv_obj_align(
        subtitle,
        LV_ALIGN_CENTER,
        0,
        -28
    );


    // --------------------------------------------------------
    // CIRCULO AZUL
    // --------------------------------------------------------

    loading_ring =
        lv_obj_create(scr_splash);

    lv_obj_set_size(
        loading_ring,
        LOADING_RING_SIZE,
        LOADING_RING_SIZE
    );

    lv_obj_set_style_radius(
        loading_ring,
        LV_RADIUS_CIRCLE,
        0
    );

    // Sem preenchimento
    lv_obj_set_style_bg_opa(
        loading_ring,
        LV_OPA_TRANSP,
        0
    );

    // Contorno azul
    lv_obj_set_style_border_width(
        loading_ring,
        LOADING_RING_WIDTH,
        0
    );

    lv_obj_set_style_border_color(
        loading_ring,
        lv_color_hex(COLOR_BLUE),
        0
    );

    lv_obj_set_style_border_opa(
        loading_ring,
        LV_OPA_COVER,
        0
    );

    lv_obj_set_pos(
        loading_ring,
        LOADING_CENTER_X -
            (LOADING_RING_SIZE / 2),

        LOADING_CENTER_Y -
            (LOADING_RING_SIZE / 2)
    );


    // --------------------------------------------------------
    // BOLINHA AZUL
    // --------------------------------------------------------

    loading_dot =
        lv_obj_create(scr_splash);

    lv_obj_set_size(
        loading_dot,
        LOADING_SIZE,
        LOADING_SIZE
    );

    lv_obj_set_style_radius(
        loading_dot,
        LV_RADIUS_CIRCLE,
        0
    );

    lv_obj_set_style_bg_color(
        loading_dot,
        lv_color_hex(COLOR_BLUE),
        0
    );

    lv_obj_set_style_bg_opa(
        loading_dot,
        LV_OPA_COVER,
        0
    );

    lv_obj_set_style_border_width(
        loading_dot,
        0,
        0
    );


    // Posicao inicial
    float radians =
        -1.5707963f;

    int x =
        LOADING_CENTER_X +
        (int)(
            cosf(radians) *
            LOADING_RADIUS
        );

    int y =
        LOADING_CENTER_Y +
        (int)(
            sinf(radians) *
            LOADING_RADIUS
        );

    x -= LOADING_SIZE / 2;
    y -= LOADING_SIZE / 2;

    lv_obj_set_pos(
        loading_dot,
        x,
        y
    );

    loading_angle = 0;

    last_loading_update =
        millis();

    lv_scr_load(
        scr_splash
    );
}


// ============================================================
// ANIMACAO DA CALIBRACAO
// ============================================================

static void update_loading_animation()
{
    if (loading_dot == NULL)
        return;

    if (loading_ring == NULL)
        return;

    unsigned long now =
        millis();

    if (
        now - last_loading_update <
        LOADING_INTERVAL_MS
    )
    {
        return;
    }

    last_loading_update =
        now;

    loading_angle += 30;

    if (loading_angle >= 360)
    {
        loading_angle -= 360;
    }

    const float PI_VALUE =
        3.14159265f;

    float radians =
        ((float)loading_angle *
         PI_VALUE) /
        180.0f;

    int x =
        LOADING_CENTER_X +
        (int)(
            cosf(radians) *
            LOADING_RADIUS
        );

    int y =
        LOADING_CENTER_Y +
        (int)(
            sinf(radians) *
            LOADING_RADIUS
        );

    x -= LOADING_SIZE / 2;
    y -= LOADING_SIZE / 2;

    x = constrain(
        x,
        0,
        SCREEN_WIDTH - LOADING_SIZE
    );

    y = constrain(
        y,
        0,
        SCREEN_HEIGHT - LOADING_SIZE
    );

    // SOMENTE MOVE A BOLINHA
    // O CIRCULO FICA PARADO
    lv_obj_set_pos(
        loading_dot,
        x,
        y
    );
}


// ============================================================
// CONTADOR
// ============================================================

static void update_measurement_counter()
{
    if (label_counter == NULL)
        return;

    char buffer[32];

    snprintf(
        buffer,
        sizeof(buffer),
        "Afericoes: %d/20",
        measurement_count
    );

    lv_label_set_text(
        label_counter,
        buffer
    );

    last_display_measurement_count =
        measurement_count;
}


// ============================================================
// TELA PRINCIPAL
// ============================================================

static void lv_create_main()
{
    scr_main =
        lv_obj_create(NULL);

    lv_obj_set_style_bg_color(
        scr_main,
        lv_color_hex(COLOR_BACKGROUND),
        0
    );

    lv_obj_set_style_bg_opa(
        scr_main,
        LV_OPA_COVER,
        0
    );


    // --------------------------------------------------------
    // TITULO
    // --------------------------------------------------------

    lv_obj_t *title =
        lv_label_create(scr_main);

    lv_label_set_text(
        title,
        "MONITORAMENTO"
    );

    lv_obj_set_style_text_color(
        title,
        lv_color_hex(COLOR_TEXT),
        0
    );

    lv_obj_set_style_text_font(
        title,
        &lv_font_montserrat_20,
        0
    );

    lv_obj_set_pos(
        title,
        15,
        10
    );


    // --------------------------------------------------------
    // CONTADOR
    // --------------------------------------------------------

    label_counter =
        lv_label_create(scr_main);

    lv_label_set_text(
        label_counter,
        "Afericoes: 0/20"
    );

    lv_obj_set_style_text_color(
        label_counter,
        lv_color_hex(COLOR_TEXT_LIGHT),
        0
    );

    lv_obj_set_style_text_font(
        label_counter,
        &lv_font_montserrat_12,
        0
    );

    lv_obj_set_pos(
        label_counter,
        15,
        38
    );


    // --------------------------------------------------------
    // HISTORICO
    // --------------------------------------------------------

    lv_obj_t *history_btn =
        create_button(
            scr_main,
            "HISTORICO",
            225,
            8,
            80,
            34
        );

    lv_obj_add_event_cb(
        history_btn,
        [](lv_event_t *e)
        {
            open_history_screen();
        },
        LV_EVENT_CLICKED,
        NULL
    );


    // --------------------------------------------------------
    // LINHA
    // --------------------------------------------------------

    lv_obj_t *line =
        lv_obj_create(scr_main);

    lv_obj_set_size(
        line,
        290,
        3
    );

    lv_obj_set_pos(
        line,
        15,
        58
    );

    lv_obj_set_style_bg_color(
        line,
        lv_color_hex(COLOR_YELLOW),
        0
    );

    lv_obj_set_style_bg_opa(
        line,
        LV_OPA_COVER,
        0
    );

    lv_obj_set_style_border_width(
        line,
        0,
        0
    );


    // ========================================================
    // CARD BPM
    // ========================================================

    lv_obj_t *card_bpm =
        create_card(
            scr_main,
            15,
            70,
            90,
            150
        );

    lv_obj_t *icon_bpm =
        lv_obj_create(card_bpm);

    lv_obj_set_size(
        icon_bpm,
        50,
        50
    );

    lv_obj_set_pos(
        icon_bpm,
        20,
        10
    );

    lv_obj_set_style_radius(
        icon_bpm,
        LV_RADIUS_CIRCLE,
        0
    );

    lv_obj_set_style_bg_color(
        icon_bpm,
        lv_color_hex(COLOR_YELLOW),
        0
    );

    lv_obj_set_style_bg_opa(
        icon_bpm,
        LV_OPA_COVER,
        0
    );

    lv_obj_set_style_border_width(
        icon_bpm,
        0,
        0
    );

    draw_heart(
        icon_bpm,
        10,
        8
    );


    lv_obj_t *bpm_title =
        lv_label_create(card_bpm);

    lv_label_set_text(
        bpm_title,
        "BPM"
    );

    lv_obj_set_style_text_color(
        bpm_title,
        lv_color_hex(COLOR_TEXT_LIGHT),
        0
    );

    lv_obj_set_style_text_font(
        bpm_title,
        &lv_font_montserrat_12,
        0
    );

    lv_obj_align(
        bpm_title,
        LV_ALIGN_CENTER,
        0,
        28
    );


    label_bpm =
        lv_label_create(card_bpm);

    lv_label_set_text(
        label_bpm,
        "--"
    );

    lv_obj_set_style_text_color(
        label_bpm,
        lv_color_hex(COLOR_YELLOW),
        0
    );

    lv_obj_set_style_text_font(
        label_bpm,
        &lv_font_montserrat_24,
        0
    );

    lv_obj_align(
        label_bpm,
        LV_ALIGN_CENTER,
        0,
        52
    );

    label_bpm_status = create_status_label(card_bpm);


    // ========================================================
    // CARD SPO2
    // ========================================================

    lv_obj_t *card_spo2 =
        create_card(
            scr_main,
            115,
            70,
            90,
            150
        );

    lv_obj_t *icon_spo2 =
        lv_obj_create(card_spo2);

    lv_obj_set_size(
        icon_spo2,
        50,
        50
    );

    lv_obj_set_pos(
        icon_spo2,
        20,
        10
    );

    lv_obj_set_style_radius(
        icon_spo2,
        LV_RADIUS_CIRCLE,
        0
    );

    lv_obj_set_style_bg_color(
        icon_spo2,
        lv_color_hex(COLOR_YELLOW),
        0
    );

    lv_obj_set_style_bg_opa(
        icon_spo2,
        LV_OPA_COVER,
        0
    );

    lv_obj_set_style_border_width(
        icon_spo2,
        0,
        0
    );

    draw_o2(
        icon_spo2,
        12,
        14
    );


    lv_obj_t *spo2_title =
        lv_label_create(card_spo2);

    lv_label_set_text(
        spo2_title,
        "O2"
    );

    lv_obj_set_style_text_color(
        spo2_title,
        lv_color_hex(COLOR_TEXT_LIGHT),
        0
    );

    lv_obj_set_style_text_font(
        spo2_title,
        &lv_font_montserrat_12,
        0
    );

    lv_obj_align(
        spo2_title,
        LV_ALIGN_CENTER,
        0,
        28
    );


    label_spo2 =
        lv_label_create(card_spo2);

    lv_label_set_text(
        label_spo2,
        "--%"
    );

    lv_obj_set_style_text_color(
        label_spo2,
        lv_color_hex(COLOR_YELLOW),
        0
    );

    lv_obj_set_style_text_font(
        label_spo2,
        &lv_font_montserrat_24,
        0
    );

    lv_obj_align(
        label_spo2,
        LV_ALIGN_CENTER,
        0,
        52
    );

    label_spo2_status = create_status_label(card_spo2);


    // ========================================================
    // CARD GLICEMIA
    // ========================================================

    lv_obj_t *card_glucose =
        create_card(
            scr_main,
            215,
            70,
            90,
            150
        );

    lv_obj_t *icon_glucose =
        lv_obj_create(card_glucose);

    lv_obj_set_size(
        icon_glucose,
        50,
        50
    );

    lv_obj_set_pos(
        icon_glucose,
        20,
        10
    );

    lv_obj_set_style_radius(
        icon_glucose,
        LV_RADIUS_CIRCLE,
        0
    );

    lv_obj_set_style_bg_color(
        icon_glucose,
        lv_color_hex(COLOR_YELLOW),
        0
    );

    lv_obj_set_style_bg_opa(
        icon_glucose,
        LV_OPA_COVER,
        0
    );

    lv_obj_set_style_border_width(
        icon_glucose,
        0,
        0
    );

    draw_drop(
        icon_glucose,
        11,
        9
    );


    lv_obj_t *glucose_title =
        lv_label_create(card_glucose);

    lv_label_set_text(
        glucose_title,
        "GLICEMIA"
    );

    lv_obj_set_style_text_color(
        glucose_title,
        lv_color_hex(COLOR_TEXT_LIGHT),
        0
    );

    lv_obj_set_style_text_font(
        glucose_title,
        &lv_font_montserrat_12,
        0
    );

    lv_obj_align(
        glucose_title,
        LV_ALIGN_CENTER,
        0,
        28
    );


    label_glucose =
        lv_label_create(card_glucose);

    lv_label_set_text(
        label_glucose,
        "--"
    );

    lv_obj_set_style_text_color(
        label_glucose,
        lv_color_hex(COLOR_YELLOW),
        0
    );

    lv_obj_set_style_text_font(
        label_glucose,
        &lv_font_montserrat_24,
        0
    );

    lv_obj_align(
        label_glucose,
        LV_ALIGN_CENTER,
        0,
        52
    );

    label_glucose_status = create_status_label(card_glucose);
}


// ============================================================
// CLASSIFICACAO VISUAL DOS INDICADORES
// ============================================================

static void update_status_labels(int bpm, float spo2, float glucose)
{
    if (label_bpm_status != NULL)
    {
        const char *status = "Sem leitura";
        if (bpm > 0 && bpm < 60) status = "Faixa baixa";
        else if (bpm >= 60 && bpm <= 100) status = "Faixa normal";
        else if (bpm > 100) status = "Faixa alta";
        lv_label_set_text(label_bpm_status, status);
        lv_obj_set_style_text_color(label_bpm_status, lv_color_hex(COLOR_YELLOW_DARK), 0);
    }

    if (label_spo2_status != NULL)
    {
        const char *status = "Sem leitura";
        if (isfinite(spo2) && spo2 > 0.0f && spo2 < 95.0f) status = "Faixa baixa";
        else if (isfinite(spo2) && spo2 >= 95.0f) status = "Faixa normal";
        lv_label_set_text(label_spo2_status, status);
        lv_obj_set_style_text_color(label_spo2_status, lv_color_hex(COLOR_YELLOW_DARK), 0);
    }

    if (label_glucose_status != NULL)
    {
        const char *status = "Sem leitura";

        // Referencias visuais simplificadas para uma medicao
        // fora de jejum; nao sao um diagnostico clinico.
        if (isfinite(glucose) && glucose > 0.0f && glucose < 70.0f)
            status = "Faixa baixa";
        else if (isfinite(glucose) && glucose >= 70.0f && glucose < 140.0f)
            status = "Faixa de referencia";
        else if (isfinite(glucose) && glucose >= 140.0f && glucose < 200.0f)
            status = "Faixa elevada";
        else if (isfinite(glucose) && glucose >= 200.0f)
            status = "Faixa alta";

        lv_label_set_text(label_glucose_status, status);
        lv_obj_set_style_text_color(label_glucose_status, lv_color_hex(COLOR_YELLOW_DARK), 0);
    }
}


// ============================================================
// ATUALIZAR VALORES
// ============================================================

static void display_update_values(
    int bpm,
    float spo2,
    float glucose
)
{
    // BPM: numero sempre na cor original (amarelo).
    if (label_bpm != NULL)
    {
        if (bpm > 0)
        {
            if (bpm != last_display_bpm)
            {
                char buffer[20];
                snprintf(buffer, sizeof(buffer), "%d", bpm);
                lv_label_set_text(label_bpm, buffer);
                last_display_bpm = bpm;
            }
        }
        else if (last_display_bpm != -1)
        {
            lv_label_set_text(label_bpm, "--");
            last_display_bpm = -1;
        }
    }


    // SPO2: numero sempre na cor original (amarelo).
    if (label_spo2 != NULL)
    {
        if (isfinite(spo2) && spo2 > 0.0f)
        {
            int spo2_int = (int)roundf(spo2);
            if (spo2_int != last_display_spo2)
            {
                char buffer[20];
                snprintf(buffer, sizeof(buffer), "%d%%", spo2_int);
                lv_label_set_text(label_spo2, buffer);
                last_display_spo2 = spo2_int;
            }
        }
        else if (last_display_spo2 != -1)
        {
            lv_label_set_text(label_spo2, "--%");
            last_display_spo2 = -1;
        }
    }


    // GLICEMIA: numero sempre na cor original (amarelo).
    if (label_glucose != NULL)
    {
        if (isfinite(glucose) && glucose > 0.0f)
        {
            int glucose_int = (int)roundf(glucose);

            if (glucose_int != last_display_glucose)
            {
                char buffer[20];
                snprintf(buffer, sizeof(buffer), "%d", glucose_int);
                lv_label_set_text(label_glucose, buffer);
                last_display_glucose = glucose_int;
            }
        }
        else if (last_display_glucose != -1)
        {
            lv_label_set_text(label_glucose, "--");
            last_display_glucose = -1;
        }
    }

    // As frases de status permanecem sempre azuis.
    update_status_labels(bpm, spo2, glucose);
}


// ============================================================
// ADICIONAR GLICEMIA AO HISTORICO
// ============================================================

static void display_add_glucose(
    float glucose
)
{
    if (
        measurement_count >=
        MAX_MEASUREMENTS
    )
    {
        return;
    }

    glucose_history[
        measurement_count
    ] = glucose;

    measurement_count++;

    update_measurement_counter();
}


// ============================================================
// CALCULAR MEDIA
// ============================================================

static float calculate_average()
{
    if (measurement_count <= 0)
    {
        return 0.0;
    }

    float total = 0.0;

    for (
        int i = 0;
        i < measurement_count;
        i++
    )
    {
        total +=
            glucose_history[i];
    }

    return total /
           measurement_count;
}


// ============================================================
// CLASSIFICACAO VISUAL APROXIMADA DA MEDIA
// ============================================================
// Faixas orientativas para adultos, usando como referencia
// valores comuns apos refeicao. O SIPDUS e um prototipo e nao
// substitui medicao clinica nem diagnostica diabetes.

static const char *get_glucose_status(float glucose)
{
    if (glucose < 70.0f)
    {
        return "Faixa baixa";
    }
    else if (glucose < 140.0f)
    {
        return "Faixa usual";
    }
    else if (glucose < 200.0f)
    {
        return "Faixa elevada";
    }

    return "Faixa alta";
}


static void update_history_average_status()
{
    if (history_average_status == NULL)
    {
        return;
    }

    if (measurement_count <= 0)
    {
        lv_label_set_text(history_average_status, "Aguardando afericoes");
    }
    else
    {
        final_average = calculate_average();
        lv_label_set_text(
            history_average_status,
            get_glucose_status(final_average)
        );
    }

    lv_obj_set_style_text_color(
        history_average_status,
        lv_color_hex(COLOR_YELLOW_DARK),
        0
    );
}


// ============================================================
// ATUALIZAR GRAFICO
// ============================================================

static void update_history_chart()
{
    if (history_chart == NULL)
    {
        return;
    }

    if (history_series == NULL)
    {
        history_series =
            lv_chart_add_series(
                history_chart,
                lv_color_hex(COLOR_YELLOW),
                LV_CHART_AXIS_PRIMARY_Y
            );
    }

    lv_chart_set_all_value(
        history_chart,
        history_series,
        LV_CHART_POINT_NONE
    );

    for (
        int i = 0;
        i < measurement_count;
        i++
    )
    {
        lv_chart_set_value_by_id(
            history_chart,
            history_series,
            (int)roundf(
                glucose_history[i]
            ),
            i
        );
    }

    lv_chart_refresh(
        history_chart
    );
}


// ============================================================
// ABRIR HISTORICO
// ============================================================

static void open_history_screen()
{
    if (history_screen_open)
    {
        return;
    }

    if (scr_history == NULL)
    {
        return;
    }

    history_screen_open = true;

    final_average =
        calculate_average();

    update_history_chart();


    if (history_average != NULL)
    {
        if (measurement_count == 0)
        {
            lv_label_set_text(
                history_average,
                "Media: -- mg/dL"
            );
        }
        else
        {
            char buffer[40];

            snprintf(
                buffer,
                sizeof(buffer),
                "Media: %.0f mg/dL",
                final_average
            );

            lv_label_set_text(
                history_average,
                buffer
            );
        }
    }

    update_history_average_status();

    lv_scr_load(
        scr_history
    );
}


// ============================================================
// VOLTAR
// ============================================================

static void back_to_main(
    lv_event_t *e
)
{
    history_screen_open = false;

    if (scr_main != NULL)
    {
        lv_scr_load(
            scr_main
        );
    }
}


// ============================================================
// RESETAR HISTORICO
// ============================================================

static void resetHistorico(
    lv_event_t *e
)
{
    if (e != NULL && lv_event_get_code(e) != LV_EVENT_CLICKED)
    {
        return;
    }

    Serial.println("Iniciando nova sessao.");
    final_screen_shown = false;
    measurement_count = 0;

    final_average = 0.0;

    last_display_bpm = -999;
    last_display_spo2 = -999;
    last_display_glucose = -999;
    last_display_measurement_count = -1;

    if (label_bpm_status != NULL) lv_label_set_text(label_bpm_status, "Sem leitura");
    if (label_spo2_status != NULL) lv_label_set_text(label_spo2_status, "Sem leitura");
    if (label_glucose_status != NULL) lv_label_set_text(label_glucose_status, "Sem leitura");
    if (label_bpm != NULL) lv_label_set_text(label_bpm, "--");
    if (label_spo2 != NULL) lv_label_set_text(label_spo2, "--%");
    if (label_glucose != NULL) lv_label_set_text(label_glucose, "--");

    for (
        int i = 0;
        i < MAX_MEASUREMENTS;
        i++
    )
    {
        glucose_history[i] = 0.0;
    }

    update_measurement_counter();


    if (history_average != NULL)
    {
        lv_label_set_text(
            history_average,
            "Media: -- mg/dL"
        );
    }

    update_history_average_status();

    if (
        history_chart != NULL &&
        history_series != NULL
    )
    {
        lv_chart_set_all_value(
            history_chart,
            history_series,
            LV_CHART_POINT_NONE
        );

        lv_chart_refresh(
            history_chart
        );
    }

    history_screen_open = false;

    if (scr_main != NULL)
    {
        lv_scr_load(
            scr_main
        );
    }
}


// ============================================================
// TELA HISTORICO
// ============================================================

static void lv_create_history()
{
    scr_history =
        lv_obj_create(NULL);

    lv_obj_set_style_bg_color(
        scr_history,
        lv_color_hex(COLOR_BACKGROUND),
        0
    );

    lv_obj_set_style_bg_opa(
        scr_history,
        LV_OPA_COVER,
        0
    );


    // --------------------------------------------------------
    // TITULO
    // --------------------------------------------------------

    history_title =
        lv_label_create(
            scr_history
        );

    lv_label_set_text(
        history_title,
        "HISTORICO"
    );

    lv_obj_set_style_text_color(
        history_title,
        lv_color_hex(COLOR_TEXT),
        0
    );

    lv_obj_set_style_text_font(
        history_title,
        &lv_font_montserrat_20,
        0
    );

    lv_obj_set_pos(
        history_title,
        15,
        8
    );


    // --------------------------------------------------------
    // MEDIA
    // --------------------------------------------------------

    history_average =
        lv_label_create(
            scr_history
        );

    lv_label_set_text(
        history_average,
        "Media: -- mg/dL"
    );

    lv_obj_set_style_text_color(
        history_average,
        lv_color_hex(COLOR_YELLOW),
        0
    );

    lv_obj_set_style_text_font(
        history_average,
        &lv_font_montserrat_14,
        0
    );

    lv_obj_set_pos(
        history_average,
        15,
        34
    );

    // CLASSIFICACAO DA MEDIA EM AMARELO
    history_average_status =
        lv_label_create(
            scr_history
        );

    lv_label_set_text(
        history_average_status,
        "Aguardando afericoes"
    );

    lv_obj_set_style_text_color(
        history_average_status,
        lv_color_hex(COLOR_YELLOW_DARK),
        0
    );

    lv_obj_set_style_text_font(
        history_average_status,
        &lv_font_montserrat_12,
        0
    );

    lv_obj_set_pos(
        history_average_status,
        15,
        53
    );


    // --------------------------------------------------------
    // GRAFICO
    // --------------------------------------------------------

    history_chart =
        lv_chart_create(
            scr_history
        );

    lv_obj_set_size(
        history_chart,
        290,
        88
    );

    lv_obj_set_pos(
        history_chart,
        15,
        78
    );

    lv_chart_set_type(
        history_chart,
        LV_CHART_TYPE_LINE
    );

    lv_chart_set_point_count(
        history_chart,
        MAX_MEASUREMENTS
    );

    lv_chart_set_range(
        history_chart,
        LV_CHART_AXIS_PRIMARY_Y,
        50,
        240
    );

    lv_chart_set_div_line_count(
        history_chart,
        4,
        5
    );

    lv_obj_set_style_bg_color(
        history_chart,
        lv_color_hex(COLOR_WHITE),
        0
    );

    lv_obj_set_style_bg_opa(
        history_chart,
        LV_OPA_COVER,
        0
    );

    lv_obj_set_style_border_width(
        history_chart,
        0,
        0
    );

    lv_obj_set_style_radius(
        history_chart,
        10,
        0
    );


    history_series =
        lv_chart_add_series(
            history_chart,
            lv_color_hex(COLOR_YELLOW),
            LV_CHART_AXIS_PRIMARY_Y
        );

    lv_chart_set_all_value(
        history_chart,
        history_series,
        LV_CHART_POINT_NONE
    );


    // --------------------------------------------------------
    // VOLTAR
    // --------------------------------------------------------

    lv_obj_t *back_btn =
        create_button(
            scr_history,
            "VOLTAR",
            15,
            180,
            135,
            42
        );

    lv_obj_add_event_cb(
        back_btn,
        back_to_main,
        LV_EVENT_CLICKED,
        NULL
    );


    // --------------------------------------------------------
    // RESETAR
    // --------------------------------------------------------

    lv_obj_t *reset_btn =
        create_button(
            scr_history,
            "NOVA SESSAO",
            170,
            180,
            135,
            42
        );

    lv_obj_add_event_cb(
        reset_btn,
        resetHistorico,
        LV_EVENT_CLICKED,
        NULL
    );
}


// ============================================================
// FINALIZAR 20 AFERICOES
// ============================================================

static void display_measurement_complete()
{
    final_screen_shown = true;

    final_average =
        calculate_average();

    if (history_average != NULL)
    {
        char buffer[40];

        snprintf(
            buffer,
            sizeof(buffer),
            "Media: %.0f mg/dL",
            final_average
        );

        lv_label_set_text(
            history_average,
            buffer
        );
    }

    update_history_average_status();
}


// ============================================================
// ATUALIZAR SENSOR
// ============================================================

static void display_sensor_update()
{
    display_update_values(
        latest_bpm,
        latest_spo2,
        latest_glucose
    );
}


// ============================================================
// SETUP DO DISPLAY
// ============================================================

static void lvgl_setup()
{
    // --------------------------------------------------------
    // TFT
    // --------------------------------------------------------

    tft.begin();

    tft.setRotation(0);

    tft.fillScreen(
        TFT_WHITE
    );


    // --------------------------------------------------------
    // TOUCH
    // --------------------------------------------------------

    touchscreenSPI.begin(
        XPT2046_CLK,
        XPT2046_MISO,
        XPT2046_MOSI,
        XPT2046_CS
    );

    touchscreen.begin(
        touchscreenSPI
    );

    touchscreen.setRotation(0);


    // --------------------------------------------------------
    // LVGL
    // --------------------------------------------------------

    lv_init();


    // --------------------------------------------------------
    // BUFFER
    // --------------------------------------------------------

    lv_disp_draw_buf_init(
        &draw_buf,
        buf1,
        NULL,
        SCREEN_WIDTH *
        BUFFER_LINES
    );


    // --------------------------------------------------------
    // DISPLAY DRIVER
    // --------------------------------------------------------

    static lv_disp_drv_t disp_drv;

    lv_disp_drv_init(
        &disp_drv
    );

    disp_drv.hor_res =
        SCREEN_WIDTH;

    disp_drv.ver_res =
        SCREEN_HEIGHT;

    disp_drv.flush_cb =
        my_disp_flush;

    disp_drv.draw_buf =
        &draw_buf;

    lv_disp_drv_register(
        &disp_drv
    );


    // --------------------------------------------------------
    // TOUCH DRIVER
    // --------------------------------------------------------

    static lv_indev_drv_t indev_drv;

    lv_indev_drv_init(
        &indev_drv
    );

    indev_drv.type =
        LV_INDEV_TYPE_POINTER;

    indev_drv.read_cb =
        touchscreen_read;

    lv_indev_drv_register(
        &indev_drv
    );


    // --------------------------------------------------------
    // CRIAR TELAS
    // --------------------------------------------------------

    lv_create_splash();

    lv_create_main();

    lv_create_history();


    // --------------------------------------------------------
    // ESTADO INICIAL
    // --------------------------------------------------------

    splash_start_ms =
        millis();

    splash_finished = false;

    history_screen_open = false;

    final_screen_shown = false;

    measurement_count = 0;

    final_average = 0.0;
}


// ============================================================
// LOOP DO DISPLAY
// ============================================================

static void display_loop()
{
    // ========================================================
    // CALIBRACAO
    // ========================================================

    if (!splash_finished)
    {
        update_loading_animation();

        if (
            millis() -
            splash_start_ms >=
            SPLASH_MS
        )
        {
            splash_finished = true;


            // Remove a bolinha
            if (loading_dot != NULL)
            {
                lv_obj_del(
                    loading_dot
                );

                loading_dot = NULL;
            }


            // Remove o circulo
            if (loading_ring != NULL)
            {
                lv_obj_del(
                    loading_ring
                );

                loading_ring = NULL;
            }


            // Vai para tela principal
            if (scr_main != NULL)
            {
                lv_scr_load(
                    scr_main
                );
            }
        }

        return;
    }


    // ========================================================
    // ATUALIZA VALORES
    // ========================================================

    display_sensor_update();


    // ========================================================
    // FALLBACK DO HISTORICO
    // ========================================================
    //
    // Permite abrir HISTORICO mesmo se o evento LVGL
    // nao for detectado.
    //

    if (
        scr_main != NULL &&
        !history_screen_open
    )
    {
        if (touchscreen.touched())
        {
            TS_Point p =
                touchscreen.getPoint();

            int x = map(
                p.x,
                TOUCH_X_MIN,
                TOUCH_X_MAX,
                0,
                SCREEN_WIDTH - 1
            );

            int y = map(
                p.y,
                TOUCH_Y_MIN,
                TOUCH_Y_MAX,
                0,
                SCREEN_HEIGHT - 1
            );

            x = constrain(
                x,
                0,
                SCREEN_WIDTH - 1
            );

            y = constrain(
                y,
                0,
                SCREEN_HEIGHT - 1
            );


            // Area do HISTORICO
            if (
                x >= 225 &&
                x <= 305 &&
                y >= 8 &&
                y <= 42
            )
            {
                if (!touch_was_pressed)
                {
                    touch_was_pressed = true;

                    open_history_screen();
                }
            }
        }
        else
        {
            touch_was_pressed = false;
        }
    }
}


// ============================================================
// FUNCOES PUBLICAS
// ============================================================

// Compatibilidade com Sensor.h de versoes anteriores.
static void update_sensor_values(int bpm, float spo2)
{
    display_update_values(bpm, spo2, latest_glucose);
}

static void add_glucose_measurement(float glucose)
{
    latest_glucose = glucose;
    display_update_values(latest_bpm, latest_spo2, glucose);
}


static void addMeasurementToHistory(
    float glucose
)
{
    display_add_glucose(
        glucose
    );
}


static void add_glucose_history(
    float glucose
)
{
    display_add_glucose(
        glucose
    );
}


static void updateDisplay(
    int bpm,
    float spo2,
    float glucose
)
{
    display_update_values(
        bpm,
        spo2,
        glucose
    );
}


static int getMeasurementCount()
{
    return measurement_count;
}


static float getFinalAverage()
{
    return calculate_average();
}

#endif
