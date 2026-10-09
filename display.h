#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <lvgl.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include <math.h>

// =====================================================
// CONFIGURACAO
// =====================================================

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define BUFFER_LINES 20
#define MAX_MEASUREMENTS 30
#define SPLASH_MS 10000UL

#define COLOR_YELLOW       0xE3C82B
#define COLOR_YELLOW_DARK  0xBFA624
#define COLOR_WHITE        0xFFFFFF
#define COLOR_BLACK        0x000000
#define COLOR_BACKGROUND   0xF5F6FA
#define COLOR_TEXT         0x25283D
#define COLOR_TEXT_LIGHT   0x686D82
#define COLOR_TEXT_GRAY    0x9297A8
#define COLOR_BORDER       0xE6E8F0
#define COLOR_CARD         0xFFFFFF
#define COLOR_BLUE         0x1976D2

// =====================================================
// TOUCHSCREEN
// =====================================================

#define XPT2046_IRQ  36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK  25
#define XPT2046_CS   33

#define TOUCH_X_MIN 200
#define TOUCH_X_MAX 3700
#define TOUCH_Y_MIN 240
#define TOUCH_Y_MAX 3800

// =====================================================
// VARIAVEIS EXTERNAS DO .INO
// =====================================================

extern int latest_bpm;
extern float latest_spo2;
extern float latest_glucose;

// =====================================================
// HARDWARE
// =====================================================

static SPIClass touchscreenSPI = SPIClass(VSPI);

static XPT2046_Touchscreen touchscreen(
    XPT2046_CS,
    XPT2046_IRQ
);

static TFT_eSPI tft = TFT_eSPI();

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[SCREEN_WIDTH * BUFFER_LINES];

// =====================================================
// TELAS
// =====================================================

static lv_obj_t* scr_splash = NULL;
static lv_obj_t* scr_main = NULL;
static lv_obj_t* scr_history = NULL;

static lv_obj_t* spinner = NULL;

static lv_obj_t* label_loading = NULL;
static lv_obj_t* label_loading_subtitle = NULL;

static lv_obj_t* label_bpm = NULL;
static lv_obj_t* label_spo2 = NULL;
static lv_obj_t* label_glucose = NULL;

static lv_obj_t* label_bpm_status = NULL;
static lv_obj_t* label_spo2_status = NULL;
static lv_obj_t* label_glucose_status = NULL;

static lv_obj_t* label_counter = NULL;

static lv_obj_t* history_count_label = NULL;
static lv_obj_t* history_average = NULL;
static lv_obj_t* history_chart = NULL;
static lv_obj_t* history_empty_label = NULL;

static lv_chart_series_t* history_series = NULL;

// =====================================================
// HISTORICO
// =====================================================

static float glucose_history[MAX_MEASUREMENTS] = {0};

static int measurement_count = 0;

static unsigned long splash_start_ms = 0;

static bool history_screen_open = false;
static bool final_screen_shown = false;
static bool splash_finished = false;
static bool touch_was_pressed = false;

static int last_counter_value = -1;
static int last_history_count = -1;

static int last_bpm_displayed = -999;
static int last_spo2_displayed = -999;
static int last_glucose_displayed = -999;

// =====================================================
// PROTOTIPOS
// =====================================================

static void lv_create_splash();
static void lv_create_main();
static void lv_create_history();

static void update_history_chart();
static void update_history_average();
static void update_measurement_counter();
static void update_history_screen();

static void open_history_screen();
static void resetHistorico();

static void update_sensor_values(int bpm, float spo2);
static void add_glucose_measurement(float glucose);

static bool isMeasurementComplete();
static void check_measurement_completion();

// =====================================================
// DRIVER DO DISPLAY
// =====================================================

static void my_disp_flush(
    lv_disp_drv_t* disp,
    const lv_area_t* area,
    lv_color_t* color_p
) {
    uint32_t width = area->x2 - area->x1 + 1;
    uint32_t height = area->y2 - area->y1 + 1;

    tft.startWrite();

    tft.setAddrWindow(
        area->x1,
        area->y1,
        width,
        height
    );

    tft.pushColors(
        reinterpret_cast<uint16_t*>(color_p),
        width * height,
        true
    );

    tft.endWrite();

    lv_disp_flush_ready(disp);
}

// =====================================================
// TOUCHSCREEN
// =====================================================

static void touchscreen_read(
    lv_indev_drv_t* indev_driver,
    lv_indev_data_t* data
) {
    (void)indev_driver;

    if (touchscreen.touched()) {
        TS_Point p = touchscreen.getPoint();

        int x = map(
            p.x,
            TOUCH_X_MIN,
            TOUCH_X_MAX,
            SCREEN_WIDTH - 1,
            0
        );

        int y = map(
            p.y,
            TOUCH_Y_MIN,
            TOUCH_Y_MAX,
            0,
            SCREEN_HEIGHT - 1
        );

        x = constrain(x, 0, SCREEN_WIDTH - 1);
        y = constrain(y, 0, SCREEN_HEIGHT - 1);

        if (!touch_was_pressed) {
            Serial.printf(
                "TOUCH RAW: X=%d Y=%d | LVGL: X=%d Y=%d\n",
                p.x, p.y, x, y
            );

            touch_was_pressed = true;
        }

        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PRESSED;

    } else {
        data->state = LV_INDEV_STATE_RELEASED;
        touch_was_pressed = false;
    }
}

// =====================================================
// ESTILOS
// =====================================================

static void style_label(
    lv_obj_t* label,
    uint32_t color,
    const lv_font_t* font
) {
    lv_obj_set_style_text_color(
        label,
        lv_color_hex(color),
        0
    );

    lv_obj_set_style_text_font(
        label,
        font,
        0
    );
}

static void remove_scroll(lv_obj_t* obj) {
    lv_obj_clear_flag(
        obj,
        LV_OBJ_FLAG_SCROLLABLE
    );
}

static void style_card(
    lv_obj_t* card,
    uint32_t background,
    int radius
) {
    lv_obj_set_style_bg_color(
        card,
        lv_color_hex(background),
        0
    );

    lv_obj_set_style_bg_opa(
        card,
        LV_OPA_COVER,
        0
    );

    lv_obj_set_style_radius(
        card,
        radius,
        0
    );

    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_shadow_width(card, 0, 0);

    remove_scroll(card);
}

// =====================================================
// BOTOES
// =====================================================

static lv_obj_t* create_button(
    lv_obj_t* parent,
    const char* text,
    int width,
    int height
) {
    lv_obj_t* button = lv_btn_create(parent);

    lv_obj_set_size(button, width, height);

    lv_obj_set_style_radius(button, 10, 0);

    lv_obj_set_style_bg_color(
        button,
        lv_color_hex(COLOR_YELLOW),
        LV_PART_MAIN
    );

    lv_obj_set_style_bg_opa(
        button,
        LV_OPA_COVER,
        LV_PART_MAIN
    );

    lv_obj_set_style_border_width(
        button,
        0,
        LV_PART_MAIN
    );

    lv_obj_set_style_shadow_width(
        button,
        0,
        LV_PART_MAIN
    );

    lv_obj_set_style_bg_color(
        button,
        lv_color_hex(COLOR_YELLOW_DARK),
        LV_PART_MAIN | LV_STATE_PRESSED
    );

    remove_scroll(button);

    lv_obj_t* label = lv_label_create(button);

    lv_label_set_text(label, text);

    style_label(
        label,
        COLOR_TEXT,
        &lv_font_montserrat_14
    );

    lv_obj_center(label);

    return button;
}

// =====================================================
// ICONE DE CORACAO
// =====================================================

static void draw_heart_icon(lv_obj_t* parent) {
    static lv_point_t points[] = {
        {17, 27}, {5, 15}, {5, 9}, {9, 5},
        {14, 5}, {17, 9}, {20, 5}, {25, 5},
        {29, 9}, {29, 15}, {17, 27}
    };

    lv_obj_t* line = lv_line_create(parent);

    lv_line_set_points(
        line,
        points,
        sizeof(points) / sizeof(points[0])
    );

    lv_obj_set_style_line_color(
        line,
        lv_color_hex(COLOR_WHITE),
        0
    );

    lv_obj_set_style_line_width(line, 2, 0);
    lv_obj_set_style_line_rounded(line, true, 0);

    lv_obj_align(line, LV_ALIGN_CENTER, -2, 0);
}

// =====================================================
// ICONE DE OXIGENIO
// =====================================================

static void draw_oxygen_icon(lv_obj_t* parent) {
    lv_obj_t* circle = lv_obj_create(parent);

    lv_obj_set_size(circle, 27, 27);
    lv_obj_center(circle);

    lv_obj_set_style_radius(
        circle,
        LV_RADIUS_CIRCLE,
        0
    );

    lv_obj_set_style_bg_opa(
        circle,
        LV_OPA_TRANSP,
        0
    );

    lv_obj_set_style_border_color(
        circle,
        lv_color_hex(COLOR_WHITE),
        0
    );

    lv_obj_set_style_border_width(circle, 2, 0);

    remove_scroll(circle);

    lv_obj_t* label = lv_label_create(circle);

    lv_label_set_text(label, "O2");

    style_label(
        label,
        COLOR_WHITE,
        &lv_font_montserrat_10
    );

    lv_obj_center(label);
}

// =====================================================
// ICONE DE GOTA
// =====================================================

static void draw_drop_icon(lv_obj_t* parent) {
    static lv_point_t points[] = {
        {17, 3}, {9, 14}, {7, 19}, {8, 24},
        {12, 28}, {17, 30}, {22, 28}, {26, 24},
        {27, 19}, {25, 14}, {17, 3}
    };

    lv_obj_t* line = lv_line_create(parent);

    lv_line_set_points(
        line,
        points,
        sizeof(points) / sizeof(points[0])
    );

    lv_obj_set_style_line_color(
        line,
        lv_color_hex(COLOR_WHITE),
        0
    );

    lv_obj_set_style_line_width(line, 2, 0);
    lv_obj_set_style_line_rounded(line, true, 0);

    lv_obj_align(line, LV_ALIGN_CENTER, -2, 0);
}

// =====================================================
// CARD DE MONITORAMENTO
// =====================================================

static lv_obj_t* create_card(
    lv_obj_t* parent,
    int icon_type,
    const char* title,
    const char* unit,
    int y,
    lv_obj_t** status_out
) {
    lv_obj_t* card = lv_obj_create(parent);

    lv_obj_set_size(card, 290, 48);

    lv_obj_align(
        card,
        LV_ALIGN_TOP_MID,
        0,
        y
    );

    style_card(card, COLOR_CARD, 14);

    lv_obj_t* icon_bg = lv_obj_create(card);

    lv_obj_set_size(icon_bg, 34, 34);

    lv_obj_align(
        icon_bg,
        LV_ALIGN_LEFT_MID,
        0,
        0
    );

    lv_obj_set_style_bg_color(
        icon_bg,
        lv_color_hex(COLOR_YELLOW),
        0
    );

    lv_obj_set_style_bg_opa(
        icon_bg,
        LV_OPA_COVER,
        0
    );

    lv_obj_set_style_border_width(icon_bg, 0, 0);
    lv_obj_set_style_radius(icon_bg, 10, 0);

    remove_scroll(icon_bg);

    if (icon_type == 0) {
        draw_heart_icon(icon_bg);
    } else if (icon_type == 1) {
        draw_oxygen_icon(icon_bg);
    } else {
        draw_drop_icon(icon_bg);
    }

    lv_obj_t* title_label = lv_label_create(card);

    lv_label_set_text(title_label, title);

    style_label(
        title_label,
        COLOR_TEXT,
        &lv_font_montserrat_14
    );

    lv_obj_align(
        title_label,
        LV_ALIGN_LEFT_MID,
        43,
        -8
    );

    lv_obj_t* status_label = lv_label_create(card);

    lv_label_set_text(status_label, "Sem leitura");

    style_label(
        status_label,
        COLOR_BLUE,
        &lv_font_montserrat_10
    );

    lv_obj_align(
        status_label,
        LV_ALIGN_LEFT_MID,
        43,
        11
    );

    if (status_out != NULL) {
        *status_out = status_label;
    }

    lv_obj_t* value_label = lv_label_create(card);

    lv_label_set_text(value_label, "--");

    style_label(
        value_label,
        COLOR_YELLOW_DARK,
        &lv_font_montserrat_20
    );

    lv_obj_align(
        value_label,
        LV_ALIGN_RIGHT_MID,
        -38,
        -3
    );

    lv_obj_t* unit_label = lv_label_create(card);

    lv_label_set_text(unit_label, unit);

    style_label(
        unit_label,
        COLOR_TEXT_GRAY,
        &lv_font_montserrat_10
    );

    lv_obj_align(
        unit_label,
        LV_ALIGN_RIGHT_MID,
        -2,
        12
    );

    return value_label;
}

// =====================================================
// TELA DE CALIBRACAO
// =====================================================

static void lv_create_splash() {
    scr_splash = lv_obj_create(NULL);

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

    lv_obj_set_style_border_width(scr_splash, 0, 0);

    remove_scroll(scr_splash);

    label_loading = lv_label_create(scr_splash);

    lv_label_set_text(
        label_loading,
        "CALIBRANDO..."
    );

    style_label(
        label_loading,
        COLOR_YELLOW_DARK,
        &lv_font_montserrat_26
    );

    lv_obj_align(
        label_loading,
        LV_ALIGN_CENTER,
        0,
        -65
    );

    spinner = lv_spinner_create(
        scr_splash,
        2000,
        60
    );

    lv_obj_set_size(spinner, 64, 64);

    lv_obj_align(
        spinner,
        LV_ALIGN_CENTER,
        0,
        -5
    );

    lv_obj_set_style_arc_color(
        spinner,
        lv_color_hex(COLOR_YELLOW),
        LV_PART_MAIN
    );

    lv_obj_set_style_arc_color(
        spinner,
        lv_color_hex(COLOR_YELLOW_DARK),
        LV_PART_INDICATOR
    );

    lv_obj_set_style_arc_width(spinner, 6, LV_PART_MAIN);
    lv_obj_set_style_arc_width(spinner, 6, LV_PART_INDICATOR);

    label_loading_subtitle = lv_label_create(scr_splash);

    lv_label_set_text(
        label_loading_subtitle,
        "Aguarde enquanto o dispositivo\nrealiza a calibracao"
    );

    lv_obj_set_style_text_align(
        label_loading_subtitle,
        LV_TEXT_ALIGN_CENTER,
        0
    );

    style_label(
        label_loading_subtitle,
        COLOR_TEXT_LIGHT,
        &lv_font_montserrat_14
    );

    lv_obj_align(
        label_loading_subtitle,
        LV_ALIGN_CENTER,
        0,
        62
    );
}

// =====================================================
// CALLBACKS DOS BOTOES
// =====================================================

static void history_button_event(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    open_history_screen();
}

static void back_button_event(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    history_screen_open = false;

    if (scr_main != NULL) {
        lv_scr_load(scr_main);
    }
}

static void reset_button_event(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    resetHistorico();

    history_screen_open = false;

    if (scr_main != NULL) {
        lv_scr_load(scr_main);
    }

    update_measurement_counter();
}

// =====================================================
// TELA PRINCIPAL
// =====================================================

static void lv_create_main() {
    scr_main = lv_obj_create(NULL);

    lv_obj_set_style_bg_color(
        scr_main,
        lv_color_hex(COLOR_BACKGROUND),
        0
    );

    lv_obj_set_style_bg_opa(scr_main, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(scr_main, 0, 0);

    remove_scroll(scr_main);

    lv_obj_t* title = lv_label_create(scr_main);

    lv_label_set_text(title, "MONITORAMENTO");

    style_label(
        title,
        COLOR_TEXT,
        &lv_font_montserrat_20
    );

    lv_obj_align(
        title,
        LV_ALIGN_TOP_LEFT,
        14,
        8
    );

    label_counter = lv_label_create(scr_main);

    lv_label_set_text(label_counter, "Afericoes: 0/30");

    style_label(
        label_counter,
        COLOR_TEXT_LIGHT,
        &lv_font_montserrat_12
    );

    lv_obj_align(
        label_counter,
        LV_ALIGN_TOP_LEFT,
        16,
        34
    );

    lv_obj_t* history_button = create_button(
        scr_main,
        "HISTORICO",
        85,
        30
    );

    lv_obj_align(
        history_button,
        LV_ALIGN_TOP_RIGHT,
        -8,
        6
    );

    lv_obj_add_event_cb(
        history_button,
        history_button_event,
        LV_EVENT_CLICKED,
        NULL
    );

    lv_obj_t* line = lv_obj_create(scr_main);

    lv_obj_set_size(line, 180, 3);

    lv_obj_align(
        line,
        LV_ALIGN_TOP_LEFT,
        16,
        50
    );

    lv_obj_set_style_bg_color(
        line,
        lv_color_hex(COLOR_YELLOW),
        0
    );

    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(line, 0, 0);
    lv_obj_set_style_radius(line, 2, 0);

    remove_scroll(line);

    label_bpm = create_card(
        scr_main,
        0,
        "Batimentos",
        "bpm",
        57,
        &label_bpm_status
    );

    label_spo2 = create_card(
        scr_main,
        1,
        "Oximetria",
        "%",
        111,
        &label_spo2_status
    );

    label_glucose = create_card(
        scr_main,
        2,
        "Glicemia",
        "mg/dL",
        165,
        &label_glucose_status
    );
}

// =====================================================
// TELA DE HISTORICO
// =====================================================

static void lv_create_history() {
    scr_history = lv_obj_create(NULL);

    lv_obj_set_style_bg_color(
        scr_history,
        lv_color_hex(COLOR_BACKGROUND),
        0
    );

    lv_obj_set_style_bg_opa(scr_history, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(scr_history, 0, 0);

    remove_scroll(scr_history);

    lv_obj_t* title = lv_label_create(scr_history);

    lv_label_set_text(title, "HISTORICO");

    style_label(
        title,
        COLOR_TEXT,
        &lv_font_montserrat_20
    );

    lv_obj_align(
        title,
        LV_ALIGN_TOP_LEFT,
        13,
        7
    );

    history_count_label = lv_label_create(scr_history);

    lv_label_set_text(history_count_label, "0/30");

    style_label(
        history_count_label,
        COLOR_YELLOW_DARK,
        &lv_font_montserrat_14
    );

    lv_obj_align(
        history_count_label,
        LV_ALIGN_TOP_RIGHT,
        -14,
        12
    );

    // Card da media
    lv_obj_t* average_card = lv_obj_create(scr_history);

    lv_obj_set_size(average_card, 296, 46);

    lv_obj_align(
        average_card,
        LV_ALIGN_TOP_MID,
        0,
        34
    );

    style_card(average_card, COLOR_WHITE, 12);

    lv_obj_t* accent = lv_obj_create(average_card);

    lv_obj_set_size(accent, 5, 28);

    lv_obj_align(
        accent,
        LV_ALIGN_LEFT_MID,
        0,
        0
    );

    lv_obj_set_style_bg_color(
        accent,
        lv_color_hex(COLOR_YELLOW),
        0
    );

    lv_obj_set_style_bg_opa(accent, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(accent, 0, 0);
    lv_obj_set_style_radius(accent, 3, 0);

    remove_scroll(accent);

    lv_obj_t* average_caption = lv_label_create(average_card);

    lv_label_set_text(
        average_caption,
        "MEDIA DAS AFERICOES"
    );

    style_label(
        average_caption,
        COLOR_TEXT_LIGHT,
        &lv_font_montserrat_10
    );

    lv_obj_align(
        average_caption,
        LV_ALIGN_LEFT_MID,
        14,
        -9
    );

    history_average = lv_label_create(average_card);

    lv_label_set_text(history_average, "-- mg/dL");

    style_label(
        history_average,
        COLOR_YELLOW_DARK,
        &lv_font_montserrat_20
    );

    lv_obj_align(
        history_average,
        LV_ALIGN_LEFT_MID,
        14,
        11
    );

    // Card do grafico
    lv_obj_t* chart_card = lv_obj_create(scr_history);

    lv_obj_set_size(chart_card, 296, 105);

    lv_obj_align(
        chart_card,
        LV_ALIGN_TOP_MID,
        0,
        86
    );

    style_card(chart_card, COLOR_WHITE, 12);

    lv_obj_t* chart_title = lv_label_create(chart_card);

    lv_label_set_text(
        chart_title,
        "EVOLUCAO DA GLICEMIA"
    );

    style_label(
        chart_title,
        COLOR_TEXT_LIGHT,
        &lv_font_montserrat_10
    );

    lv_obj_align(
        chart_title,
        LV_ALIGN_TOP_LEFT,
        3,
        0
    );

    history_chart = lv_chart_create(chart_card);

    lv_obj_set_size(history_chart, 276, 72);

    lv_obj_align(
        history_chart,
        LV_ALIGN_BOTTOM_MID,
        0,
        -1
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

    lv_chart_set_div_line_count(history_chart, 4, 5);

    lv_obj_set_style_bg_opa(
        history_chart,
        LV_OPA_TRANSP,
        LV_PART_MAIN
    );

    lv_obj_set_style_border_width(
        history_chart,
        0,
        LV_PART_MAIN
    );

    lv_obj_set_style_line_color(
        history_chart,
        lv_color_hex(COLOR_BORDER),
        LV_PART_MAIN
    );

    lv_obj_set_style_line_width(
        history_chart,
        1,
        LV_PART_MAIN
    );

    lv_obj_set_style_line_color(
        history_chart,
        lv_color_hex(COLOR_YELLOW_DARK),
        LV_PART_ITEMS
    );

    lv_obj_set_style_line_width(
        history_chart,
        3,
        LV_PART_ITEMS
    );

    lv_obj_set_style_line_rounded(
        history_chart,
        true,
        LV_PART_ITEMS
    );

    lv_obj_set_style_bg_color(
        history_chart,
        lv_color_hex(COLOR_YELLOW),
        LV_PART_INDICATOR
    );

    lv_obj_set_style_bg_opa(
        history_chart,
        LV_OPA_COVER,
        LV_PART_INDICATOR
    );

    lv_obj_set_style_radius(
        history_chart,
        LV_RADIUS_CIRCLE,
        LV_PART_INDICATOR
    );

    lv_obj_set_style_border_width(
        history_chart,
        0,
        LV_PART_INDICATOR
    );

    history_series = lv_chart_add_series(
        history_chart,
        lv_color_hex(COLOR_YELLOW_DARK),
        LV_CHART_AXIS_PRIMARY_Y
    );

    for (int i = 0; i < MAX_MEASUREMENTS; i++) {
        lv_chart_set_value_by_id(
            history_chart,
            history_series,
            i,
            LV_CHART_POINT_NONE
        );
    }

    history_empty_label = lv_label_create(chart_card);

    lv_label_set_text(
        history_empty_label,
        "Aguardando afericoes..."
    );

    style_label(
        history_empty_label,
        COLOR_TEXT_GRAY,
        &lv_font_montserrat_10
    );

    lv_obj_align(
        history_empty_label,
        LV_ALIGN_CENTER,
        0,
        13
    );

    // Botao VOLTAR
    lv_obj_t* back_button = create_button(
        scr_history,
        "VOLTAR",
        84,
        29
    );

    lv_obj_align(
        back_button,
        LV_ALIGN_BOTTOM_LEFT,
        12,
        -7
    );

    lv_obj_add_event_cb(
        back_button,
        back_button_event,
        LV_EVENT_CLICKED,
        NULL
    );

    // Botao NOVA SESSAO
    lv_obj_t* reset_button = create_button(
        scr_history,
        "NOVA SESSAO",
        112,
        29
    );

    lv_obj_align(
        reset_button,
        LV_ALIGN_BOTTOM_RIGHT,
        -12,
        -7
    );

    lv_obj_add_event_cb(
        reset_button,
        reset_button_event,
        LV_EVENT_CLICKED,
        NULL
    );
}

// =====================================================
// ABRIR HISTORICO
// =====================================================

static void open_history_screen() {
    if (scr_history == NULL) {
        Serial.println("ERRO: tela de historico nao criada.");
        return;
    }

    history_screen_open = true;
    last_history_count = -1;

    update_history_chart();
    update_history_average();

    if (history_count_label != NULL) {
        char text[20];

        snprintf(
            text,
            sizeof(text),
            "%d/%d",
            measurement_count,
            MAX_MEASUREMENTS
        );

        lv_label_set_text(history_count_label, text);
    }

    lv_scr_load(scr_history);

    Serial.printf(
        "Historico aberto: %d afericoes.\n",
        measurement_count
    );
}

// =====================================================
// CONTADOR
// =====================================================

static void update_measurement_counter() {
    if (label_counter == NULL) {
        return;
    }

    if (last_counter_value == measurement_count) {
        return;
    }

    char text[32];

    snprintf(
        text,
        sizeof(text),
        "Afericoes: %d/%d",
        measurement_count,
        MAX_MEASUREMENTS
    );

    lv_label_set_text(label_counter, text);

    last_counter_value = measurement_count;

    if (history_count_label != NULL) {
        char history_text[20];

        snprintf(
            history_text,
            sizeof(history_text),
            "%d/%d",
            measurement_count,
            MAX_MEASUREMENTS
        );

        lv_label_set_text(
            history_count_label,
            history_text
        );
    }
}

// =====================================================
// GRAFICO
// =====================================================

static void update_history_chart() {
    if (history_chart == NULL || history_series == NULL) {
        return;
    }

    for (int i = 0; i < MAX_MEASUREMENTS; i++) {
        lv_chart_set_value_by_id(
            history_chart,
            history_series,
            i,
            LV_CHART_POINT_NONE
        );
    }

    int total = measurement_count;

    if (total > MAX_MEASUREMENTS) {
        total = MAX_MEASUREMENTS;
    }

    for (int i = 0; i < total; i++) {
        int value = (int)roundf(glucose_history[i]);

        if (value < 50) value = 50;
        if (value > 240) value = 240;

        lv_chart_set_value_by_id(
            history_chart,
            history_series,
            i,
            value
        );
    }

    if (history_empty_label != NULL) {
        if (total == 0) {
            lv_obj_clear_flag(
                history_empty_label,
                LV_OBJ_FLAG_HIDDEN
            );
        } else {
            lv_obj_add_flag(
                history_empty_label,
                LV_OBJ_FLAG_HIDDEN
            );
        }
    }

    lv_chart_refresh(history_chart);
}

// =====================================================
// MEDIA DO HISTORICO
// =====================================================

static void update_history_average() {
    if (history_average == NULL) {
        return;
    }

    if (measurement_count <= 0) {
        lv_label_set_text(history_average, "-- mg/dL");
        return;
    }

    float sum = 0.0f;

    int total = measurement_count;

    if (total > MAX_MEASUREMENTS) {
        total = MAX_MEASUREMENTS;
    }

    for (int i = 0; i < total; i++) {
        sum += glucose_history[i];
    }

    float average = sum / total;

    char text[32];

    snprintf(
        text,
        sizeof(text),
        "%.1f mg/dL",
        average
    );

    lv_label_set_text(history_average, text);
}

// =====================================================
// ATUALIZAR TELA DE HISTORICO
// =====================================================

static void update_history_screen() {
    if (!history_screen_open) {
        return;
    }

    if (last_history_count != measurement_count) {
        update_history_chart();
        update_history_average();
        update_measurement_counter();

        last_history_count = measurement_count;
    }
}

// =====================================================
// FAIXA DE BPM
// Referencia geral para adultos em repouso
// =====================================================

static const char* get_bpm_status(int bpm) {
    if (bpm <= 0) {
        return "Sem leitura";
    }

    if (bpm < 60) {
        return "Faixa baixa";
    }

    if (bpm <= 100) {
        return "Faixa usual";
    }

    return "Faixa elevada";
}

// =====================================================
// FAIXA DE SPO2
// =====================================================

static const char* get_spo2_status(float spo2) {
    if (!isfinite(spo2) || spo2 <= 0 || spo2 > 100) {
        return "Sem leitura";
    }

    if (spo2 < 90) {
        return "Baixa";
    }

    if (spo2 < 95) {
        return "Abaixo do ideal";
    }

    return "Faixa usual";
}

// =====================================================
// FAIXA DE GLICEMIA ESTIMADA
// Referencia visual aproximada apos alimentacao
// =====================================================

static const char* get_glucose_status(float glucose) {
    if (!isfinite(glucose) || glucose <= 0) {
        return "Sem leitura";
    }

    if (glucose < 70) {
        return "Faixa baixa";
    }

    if (glucose < 140) {
        return "Faixa usual";
    }

    if (glucose < 200) {
        return "Faixa elevada";
    }

    return "Faixa alta";
}

// =====================================================
// ATUALIZAR STATUS AZUL
// =====================================================

static void set_status_text(
    lv_obj_t* label,
    const char* text
) {
    if (label == NULL) {
        return;
    }

    lv_label_set_text(label, text);

    lv_obj_set_style_text_color(
        label,
        lv_color_hex(COLOR_BLUE),
        0
    );
}

// =====================================================
// ATUALIZAR BPM E SPO2
// =====================================================

static void update_sensor_values(
    int bpm,
    float spo2
) {
    if (label_bpm != NULL && bpm != last_bpm_displayed) {
        char text[16];

        if (bpm > 0) {
            snprintf(text, sizeof(text), "%d", bpm);
        } else {
            snprintf(text, sizeof(text), "--");
        }

        lv_label_set_text(label_bpm, text);
        last_bpm_displayed = bpm;
    }

    set_status_text(
        label_bpm_status,
        get_bpm_status(bpm)
    );

    int spo2_int = (int)roundf(spo2);

    if (
        label_spo2 != NULL &&
        spo2_int != last_spo2_displayed
    ) {
        char text[16];

        if (isfinite(spo2) && spo2 > 0 && spo2 <= 100) {
            snprintf(text, sizeof(text), "%d", spo2_int);
        } else {
            snprintf(text, sizeof(text), "--");
        }

        lv_label_set_text(label_spo2, text);
        last_spo2_displayed = spo2_int;
    }

    set_status_text(
        label_spo2_status,
        get_spo2_status(spo2)
    );

    latest_bpm = bpm;
    latest_spo2 = spo2;

    update_measurement_counter();
}

// =====================================================
// REGISTRAR AFERICAO DE GLICEMIA
// =====================================================

static void add_glucose_measurement(float glucose) {
    latest_glucose = glucose;

    bool valid_glucose = isfinite(glucose) && glucose > 0;

    int glucose_int = valid_glucose
        ? (int)roundf(glucose)
        : -1;

    if (
        label_glucose != NULL &&
        glucose_int != last_glucose_displayed
    ) {
        char text[16];

        if (valid_glucose) {
            snprintf(text, sizeof(text), "%d", glucose_int);
        } else {
            snprintf(text, sizeof(text), "--");
        }

        lv_label_set_text(label_glucose, text);
        last_glucose_displayed = glucose_int;
    }

    set_status_text(
        label_glucose_status,
        get_glucose_status(glucose)
    );

    if (
        valid_glucose &&
        measurement_count < MAX_MEASUREMENTS
    ) {
        glucose_history[measurement_count] = glucose;
        measurement_count++;

        Serial.printf(
            "Afericao registrada: %d/%d - Glicemia estimada: %.1f\n",
            measurement_count,
            MAX_MEASUREMENTS,
            glucose
        );

        update_measurement_counter();

        if (history_screen_open) {
            update_history_chart();
            update_history_average();

            last_history_count = measurement_count;
        }
    }

    check_measurement_completion();
}

// =====================================================
// LIMITE DE AFERICOES
// =====================================================

static bool isMeasurementComplete() {
    return measurement_count >= MAX_MEASUREMENTS;
}

static void check_measurement_completion() {
    if (
        isMeasurementComplete() &&
        !final_screen_shown
    ) {
        final_screen_shown = true;

        Serial.println("Afericoes concluidas.");

        if (scr_history != NULL && !history_screen_open) {
            open_history_screen();
        }
    }
}

// =====================================================
// NOVA SESSAO
// =====================================================

static void resetHistorico() {
    measurement_count = 0;

    for (int i = 0; i < MAX_MEASUREMENTS; i++) {
        glucose_history[i] = 0.0f;
    }

    last_history_count = -1;
    last_counter_value = -1;

    last_bpm_displayed = -999;
    last_spo2_displayed = -999;
    last_glucose_displayed = -999;

    final_screen_shown = false;

    latest_bpm = 0;
    latest_spo2 = 0.0f;
    latest_glucose = 0.0f;

    if (label_bpm != NULL) {
        lv_label_set_text(label_bpm, "--");
    }

    if (label_spo2 != NULL) {
        lv_label_set_text(label_spo2, "--");
    }

    if (label_glucose != NULL) {
        lv_label_set_text(label_glucose, "--");
    }

    set_status_text(label_bpm_status, "Sem leitura");
    set_status_text(label_spo2_status, "Sem leitura");
    set_status_text(label_glucose_status, "Sem leitura");

    if (history_chart != NULL && history_series != NULL) {
        for (int i = 0; i < MAX_MEASUREMENTS; i++) {
            lv_chart_set_value_by_id(
                history_chart,
                history_series,
                i,
                LV_CHART_POINT_NONE
            );
        }

        lv_chart_refresh(history_chart);
    }

    if (history_average != NULL) {
        lv_label_set_text(history_average, "-- mg/dL");
    }

    if (history_empty_label != NULL) {
        lv_obj_clear_flag(
            history_empty_label,
            LV_OBJ_FLAG_HIDDEN
        );
    }

    if (history_count_label != NULL) {
        lv_label_set_text(history_count_label, "0/30");
    }

    if (label_counter != NULL) {
        lv_label_set_text(label_counter, "Afericoes: 0/30");
    }

    Serial.println("Nova sessao iniciada. Historico zerado.");
}

// =====================================================
// INICIALIZACAO DO DISPLAY E TOUCH
// =====================================================

static void lvgl_setup() {
    Serial.println("Inicializando display...");

    tft.begin();
    tft.setRotation(0);

    lv_init();

    touchscreenSPI.begin(
        XPT2046_CLK,
        XPT2046_MISO,
        XPT2046_MOSI,
        XPT2046_CS
    );

    touchscreen.begin(touchscreenSPI);
    touchscreen.setRotation(0);

    Serial.println("Touchscreen inicializado.");

    lv_disp_draw_buf_init(
        &draw_buf,
        buf1,
        NULL,
        SCREEN_WIDTH * BUFFER_LINES
    );

    static lv_disp_drv_t disp_drv;

    lv_disp_drv_init(&disp_drv);

    disp_drv.hor_res = SCREEN_WIDTH;
    disp_drv.ver_res = SCREEN_HEIGHT;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;

    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;

    lv_indev_drv_init(&indev_drv);

    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchscreen_read;

    lv_indev_drv_register(&indev_drv);

    lv_create_splash();
    lv_create_main();
    lv_create_history();

    lv_scr_load(scr_splash);

    splash_start_ms = millis();
    splash_finished = false;
    history_screen_open = false;
    touch_was_pressed = false;

    Serial.println("Display inicializado.");
}

// =====================================================
// LOOP DO DISPLAY
// =====================================================

static void display_loop() {
    if (!splash_finished) {
        if (millis() - splash_start_ms >= SPLASH_MS) {
            splash_finished = true;

            if (spinner != NULL) {
                lv_obj_del(spinner);
                spinner = NULL;
            }

            if (scr_main != NULL) {
                lv_scr_load(scr_main);
            }

            Serial.println("Tela principal carregada.");
        }

        return;
    }

    update_measurement_counter();
    update_history_screen();
    check_measurement_completion();
}

// =====================================================
// FUNCOES DE COMPATIBILIDADE COM SENSOR.H
// =====================================================

static void addMeasurementToHistory(float glucose) {
    add_glucose_measurement(glucose);
}

static void add_glucose_history(float glucose) {
    add_glucose_measurement(glucose);
}

static void updateDisplay(
    int bpm,
    float spo2,
    float glucose
) {
    update_sensor_values(bpm, spo2);
    add_glucose_measurement(glucose);
}

static int getMeasurementCount() {
    return measurement_count;
}

static float getFinalAverage() {
    if (measurement_count <= 0) {
        return 0.0f;
    }

    float sum = 0.0f;

    int total = measurement_count;

    if (total > MAX_MEASUREMENTS) {
        total = MAX_MEASUREMENTS;
    }

    for (int i = 0; i < total; i++) {
        sum += glucose_history[i];
    }

    return sum / total;
}

#endif
