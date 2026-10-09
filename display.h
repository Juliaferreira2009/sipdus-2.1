#ifndef DISPLAY_H
#define DISPLAY_H

#include <lvgl.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include <math.h>

// =====================================================
// CONFIGURACAO GERAL
// =====================================================

#define SCREEN_WIDTH       320
#define SCREEN_HEIGHT      240
#define BUFFER_LINES       20
#define MAX_MEASUREMENTS   20

// =====================================================
// CORES
// =====================================================

#define COLOR_YELLOW       0xE3C82B
#define COLOR_WHITE        0xFFFFFF
#define COLOR_BLACK        0x000000
#define COLOR_BACKGROUND   0xF7F7F7
#define COLOR_TEXT         0x333333
#define COLOR_TEXT_LIGHT   0x666666
#define COLOR_TEXT_GRAY    0x777777

// =====================================================
// SPLASH / CALIBRACAO
// =====================================================

#define SPLASH_MS          10000UL

// Spinner nativo do LVGL
#define SPINNER_SIZE       64
#define SPINNER_TIME       2000
#define SPINNER_ARC_LENGTH 60

// =====================================================
// TOUCH
// =====================================================

#define XPT2046_IRQ        36
#define XPT2046_MOSI       32
#define XPT2046_MISO       39
#define XPT2046_CLK        25
#define XPT2046_CS         33

#define TOUCH_X_MIN        200
#define TOUCH_X_MAX        3700
#define TOUCH_Y_MIN        240
#define TOUCH_Y_MAX        3800

// =====================================================
// VARIAVEIS GLOBAIS DO INO
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

// =====================================================
// BUFFER LVGL
// =====================================================

static lv_disp_draw_buf_t draw_buf;

static lv_color_t buf1[
    SCREEN_WIDTH * BUFFER_LINES
];

// =====================================================
// TELAS
// =====================================================

static lv_obj_t* scr_splash  = NULL;
static lv_obj_t* scr_main    = NULL;
static lv_obj_t* scr_history = NULL;

// =====================================================
// OBJETOS DO SPLASH
// =====================================================

static lv_obj_t* spinner = NULL;
static lv_obj_t* label_loading = NULL;
static lv_obj_t* label_subtitle = NULL;

// =====================================================
// ELEMENTOS DA TELA PRINCIPAL
// =====================================================

static lv_obj_t* label_bpm     = NULL;
static lv_obj_t* label_spo2    = NULL;
static lv_obj_t* label_glucose = NULL;
static lv_obj_t* label_counter = NULL;

// =====================================================
// HISTORICO
// =====================================================

static float glucose_history[MAX_MEASUREMENTS] = {0};

static int measurement_count = 0;

static lv_obj_t* history_title   = NULL;
static lv_obj_t* history_average = NULL;
static lv_obj_t* history_chart   = NULL;

static lv_chart_series_t* history_series = NULL;

// =====================================================
// CONTROLE DO SPLASH
// =====================================================

static unsigned long splash_start_ms = 0;
static bool splash_finished = false;

// =====================================================
// ESTADO DA INTERFACE
// =====================================================

static bool history_screen_open = false;
static bool final_screen_shown = false;

static int last_history_count = -1;
static int last_counter_value = -1;

static int last_bpm_displayed = -999;
static int last_spo2_displayed = -999;
static int last_glucose_displayed = -999;

// =====================================================
// TOUCH
// =====================================================

static bool touch_was_pressed = false;
static bool history_touch_request = false;

// =====================================================
// DECLARACOES
// =====================================================

void update_history_chart();
void update_history_average();
void update_measurement_counter();
void update_history_screen();
void check_measurement_completion();
void open_history_screen();

void update_sensor_values(int bpm, float spo2);
void add_glucose_measurement(float glucose);

// =====================================================
// DISPLAY FLUSH
// =====================================================

void my_disp_flush(
    lv_disp_drv_t* disp,
    const lv_area_t* area,
    lv_color_t* color_p
)
{
    uint32_t width =
        area->x2 - area->x1 + 1;

    uint32_t height =
        area->y2 - area->y1 + 1;

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

void touchscreen_read(
    lv_indev_drv_t* indev_driver,
    lv_indev_data_t* data
)
{
    (void)indev_driver;

    if (touchscreen.touched())
    {
        TS_Point point = touchscreen.getPoint();

        int x = map(
            point.x,
            TOUCH_X_MIN,
            TOUCH_X_MAX,
            0,
            SCREEN_WIDTH - 1
        );

        int y = map(
            point.y,
            TOUCH_Y_MIN,
            TOUCH_Y_MAX,
            0,
            SCREEN_HEIGHT - 1
        );

        x = constrain(x, 0, SCREEN_WIDTH - 1);
        y = constrain(y, 0, SCREEN_HEIGHT - 1);

        if (!touch_was_pressed)
        {
            Serial.println();
            Serial.println("========== TOUCH ==========");

            Serial.print("RAW X: ");
            Serial.println(point.x);

            Serial.print("RAW Y: ");
            Serial.println(point.y);

            Serial.print("TELA X: ");
            Serial.println(x);

            Serial.print("TELA Y: ");
            Serial.println(y);

            Serial.println("===========================");
        }

        touch_was_pressed = true;

        // Deteccao de toque no botao HISTORICO.
        if (
            scr_main != NULL &&
            !history_screen_open &&
            x >= 225 &&
            y >= 0 &&
            y <= 45
        )
        {
            history_touch_request = true;
        }

        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PRESSED;
    }
    else
    {
        data->state = LV_INDEV_STATE_RELEASED;
        touch_was_pressed = false;
    }
}

// =====================================================
// ABRIR HISTORICO
// =====================================================

void open_history_screen()
{
    if (scr_history == NULL)
    {
        return;
    }

    if (history_screen_open)
    {
        return;
    }

    Serial.println();
    Serial.println(">>> ABRINDO HISTORICO <<<");

    history_screen_open = true;
    last_history_count = -1;

    update_history_chart();
    update_history_average();

    lv_scr_load(scr_history);

    Serial.print("Afericoes atuais: ");
    Serial.println(measurement_count);

    Serial.println("===========================");
}

// =====================================================
// EVENTO BOTAO HISTORICO
// =====================================================

static void history_button_event(lv_event_t* event)
{
    if (
        lv_event_get_code(event) ==
        LV_EVENT_CLICKED
    )
    {
        Serial.println(
            "Botao HISTORICO pressionado."
        );

        open_history_screen();
    }
}

// =====================================================
// EVENTO VOLTAR
// =====================================================

static void back_button_event(lv_event_t* event)
{
    if (
        lv_event_get_code(event) !=
        LV_EVENT_CLICKED
    )
    {
        return;
    }

    Serial.println(
        "Voltando para tela principal."
    );

    history_screen_open = false;

    lv_scr_load(scr_main);
}

// =====================================================
// RESET DO HISTORICO
// =====================================================

void resetHistorico()
{
    measurement_count = 0;

    for (
        int i = 0;
        i < MAX_MEASUREMENTS;
        i++
    )
    {
        glucose_history[i] = 0.0f;
    }

    last_history_count = -1;
    last_counter_value = -1;

    last_bpm_displayed = -999;
    last_spo2_displayed = -999;
    last_glucose_displayed = -999;

    final_screen_shown = false;

    latest_bpm = 0;
    latest_spo2 = 0;
    latest_glucose = 0;

    // Limpar grafico.
    if (
        history_series != NULL &&
        history_chart != NULL
    )
    {
        for (
            int i = 0;
            i < MAX_MEASUREMENTS;
            i++
        )
        {
            lv_chart_set_value_by_id(
                history_chart,
                history_series,
                i,
                LV_CHART_POINT_NONE
            );
        }

        lv_chart_refresh(history_chart);
    }

    // Limpar media.
    if (history_average != NULL)
    {
        lv_label_set_text(
            history_average,
            "Media: -- mg/dL"
        );
    }

    // Limpar contador.
    if (label_counter != NULL)
    {
        lv_label_set_text(
            label_counter,
            "Afericoes: 0/20"
        );
    }

    // Limpar valores exibidos.
    if (label_bpm != NULL)
    {
        lv_label_set_text(label_bpm, "--");
    }

    if (label_spo2 != NULL)
    {
        lv_label_set_text(label_spo2, "--");
    }

    if (label_glucose != NULL)
    {
        lv_label_set_text(label_glucose, "--");
    }

    Serial.println("Historico resetado.");
}

// =====================================================
// EVENTO NOVA SESSAO
// =====================================================

static void reset_button_event(lv_event_t* event)
{
    if (
        lv_event_get_code(event) !=
        LV_EVENT_CLICKED
    )
    {
        return;
    }

    Serial.println("Iniciando nova sessao.");

    resetHistorico();

    history_screen_open = false;

    lv_scr_load(scr_main);

    update_measurement_counter();
}

// =====================================================
// CRIAR BOTAO
// =====================================================

lv_obj_t* create_button(
    lv_obj_t* parent,
    const char* text,
    int width,
    int height
)
{
    lv_obj_t* button = lv_btn_create(parent);

    lv_obj_set_size(
        button,
        width,
        height
    );

    lv_obj_set_style_radius(
        button,
        12,
        0
    );

    lv_obj_set_style_bg_color(
        button,
        lv_color_hex(COLOR_YELLOW),
        0
    );

    lv_obj_set_style_border_width(
        button,
        0,
        0
    );

    lv_obj_clear_flag(
        button,
        LV_OBJ_FLAG_SCROLLABLE
    );

    lv_obj_add_flag(
        button,
        LV_OBJ_FLAG_CLICKABLE
    );

    lv_obj_t* label = lv_label_create(button);

    lv_label_set_text(label, text);

    lv_obj_set_style_text_font(
        label,
        &lv_font_montserrat_14,
        0
    );

    lv_obj_set_style_text_color(
        label,
        lv_color_hex(COLOR_WHITE),
        0
    );

    lv_obj_center(label);

    return button;
}

// =====================================================
// ICONE CORACAO
// =====================================================

void draw_heart_icon(lv_obj_t* parent)
{
    static lv_point_t points[] =
    {
        {17, 27},
        {5, 15},
        {5, 9},
        {9, 5},
        {14, 5},
        {17, 9},
        {20, 5},
        {25, 5},
        {29, 9},
        {29, 15},
        {17, 27}
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
    lv_obj_center(line);
}

// =====================================================
// ICONE GOTA
// =====================================================

void draw_drop_icon(lv_obj_t* parent)
{
    static lv_point_t points[] =
    {
        {17, 3},
        {9, 14},
        {7, 19},
        {8, 24},
        {12, 28},
        {17, 30},
        {22, 28},
        {26, 24},
        {27, 19},
        {25, 14},
        {17, 3}
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
    lv_obj_center(line);
}

// =====================================================
// ICONE O2
// =====================================================

void draw_oxygen_icon(lv_obj_t* parent)
{
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

    lv_obj_set_style_border_width(circle, 2, 0);

    lv_obj_set_style_border_color(
        circle,
        lv_color_hex(COLOR_WHITE),
        0
    );

    lv_obj_clear_flag(
        circle,
        LV_OBJ_FLAG_CLICKABLE
    );

    lv_obj_clear_flag(
        circle,
        LV_OBJ_FLAG_SCROLLABLE
    );

    lv_obj_t* label = lv_label_create(parent);

    lv_label_set_text(label, "O2");

    lv_obj_set_style_text_font(
        label,
        &lv_font_montserrat_10,
        0
    );

    lv_obj_set_style_text_color(
        label,
        lv_color_hex(COLOR_WHITE),
        0
    );

    lv_obj_center(label);
}

// =====================================================
// CRIAR CARD
// =====================================================

lv_obj_t* create_card(
    lv_obj_t* parent,
    int icon_type,
    const char* title,
    const char* unit,
    int y
)
{
    lv_obj_t* card = lv_obj_create(parent);

    lv_obj_set_size(card, 290, 48);

    lv_obj_align(
        card,
        LV_ALIGN_TOP_MID,
        0,
        y
    );

    lv_obj_set_style_radius(card, 14, 0);

    lv_obj_set_style_bg_color(
        card,
        lv_color_hex(COLOR_WHITE),
        0
    );

    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_shadow_width(card, 0, 0);

    lv_obj_clear_flag(
        card,
        LV_OBJ_FLAG_SCROLLABLE
    );

    lv_obj_t* icon_bg = lv_obj_create(card);

    lv_obj_set_size(icon_bg, 34, 34);

    lv_obj_align(
        icon_bg,
        LV_ALIGN_LEFT_MID,
        7,
        0
    );

    lv_obj_set_style_radius(icon_bg, 9, 0);

    lv_obj_set_style_bg_color(
        icon_bg,
        lv_color_hex(COLOR_YELLOW),
        0
    );

    lv_obj_set_style_border_width(icon_bg, 0, 0);

    lv_obj_clear_flag(
        icon_bg,
        LV_OBJ_FLAG_SCROLLABLE
    );

    lv_obj_clear_flag(
        icon_bg,
        LV_OBJ_FLAG_CLICKABLE
    );

    if (icon_type == 0)
    {
        draw_heart_icon(icon_bg);
    }
    else if (icon_type == 1)
    {
        draw_oxygen_icon(icon_bg);
    }
    else
    {
        draw_drop_icon(icon_bg);
    }

    lv_obj_t* lbl_title = lv_label_create(card);

    lv_label_set_text(lbl_title, title);

    lv_obj_set_style_text_font(
        lbl_title,
        &lv_font_montserrat_14,
        0
    );

    lv_obj_set_style_text_color(
        lbl_title,
        lv_color_hex(0x444444),
        0
    );

    lv_obj_align(
        lbl_title,
        LV_ALIGN_LEFT_MID,
        50,
        0
    );

    lv_obj_t* lbl_value = lv_label_create(card);

    lv_label_set_text(lbl_value, "--");

    lv_obj_set_style_text_font(
        lbl_value,
        &lv_font_montserrat_20,
        0
    );

    lv_obj_set_style_text_color(
        lbl_value,
        lv_color_hex(COLOR_YELLOW),
        0
    );

    lv_obj_align(
        lbl_value,
        LV_ALIGN_RIGHT_MID,
        -42,
        -5
    );

    lv_obj_t* lbl_unit = lv_label_create(card);

    lv_label_set_text(lbl_unit, unit);

    lv_obj_set_style_text_font(
        lbl_unit,
        &lv_font_montserrat_10,
        0
    );

    lv_obj_set_style_text_color(
        lbl_unit,
        lv_color_hex(COLOR_TEXT_GRAY),
        0
    );

    lv_obj_align(
        lbl_unit,
        LV_ALIGN_RIGHT_MID,
        -8,
        10
    );

    return lbl_value;
}

// =====================================================
// SPLASH / CALIBRACAO COM SPINNER NATIVO LVGL
// =====================================================

void lv_create_splash()
{
    scr_splash = lv_obj_create(NULL);

    lv_obj_set_size(
        scr_splash,
        SCREEN_WIDTH,
        SCREEN_HEIGHT
    );

    lv_obj_set_style_bg_color(
        scr_splash,
        lv_color_hex(COLOR_WHITE),
        LV_PART_MAIN
    );

    lv_obj_set_style_bg_opa(
        scr_splash,
        LV_OPA_COVER,
        LV_PART_MAIN
    );

    lv_obj_set_style_border_width(
        scr_splash,
        0,
        0
    );

    lv_obj_clear_flag(
        scr_splash,
        LV_OBJ_FLAG_SCROLLABLE
    );

    // =================================================
    // SPINNER NATIVO DO SEGUNDO CODIGO
    // =================================================

    spinner = lv_spinner_create(
        scr_splash,
        SPINNER_TIME,
        SPINNER_ARC_LENGTH
    );

    lv_obj_set_size(
        spinner,
        SPINNER_SIZE,
        SPINNER_SIZE
    );

    // Posicao adaptada para a tela 320 x 240.
    lv_obj_align(
        spinner,
        LV_ALIGN_CENTER,
        0,
        -65
    );

    // Cor do arco de fundo.
    lv_obj_set_style_arc_color(
        spinner,
        lv_color_hex(0xE3C82B),
        LV_PART_MAIN
    );

    // Cor do arco animado.
    lv_obj_set_style_arc_color(
        spinner,
        lv_color_hex(0xBFA624),
        LV_PART_INDICATOR
    );

    // Espessura dos arcos.
    lv_obj_set_style_arc_width(
        spinner,
        6,
        LV_PART_MAIN
    );

    lv_obj_set_style_arc_width(
        spinner,
        6,
        LV_PART_INDICATOR
    );

    // =================================================
    // TITULO
    // =================================================

    label_loading = lv_label_create(scr_splash);

    lv_label_set_text(
        label_loading,
        "CALIBRANDO..."
    );

    lv_obj_set_style_text_font(
        label_loading,
        &lv_font_montserrat_26,
        0
    );

    lv_obj_align(
        label_loading,
        LV_ALIGN_CENTER,
        0,
        -10
    );

    lv_obj_set_style_text_color(
        label_loading,
        lv_color_hex(COLOR_YELLOW),
        0
    );

    // =================================================
    // SUBTITULO
    // =================================================

    label_subtitle = lv_label_create(scr_splash);

    lv_label_set_text(
        label_subtitle,
        "Aguarde enquanto o dispositivo\n"
        "realiza a calibracao"
    );

    lv_obj_set_style_text_font(
        label_subtitle,
        &lv_font_montserrat_14,
        0
    );

    lv_obj_set_style_text_align(
        label_subtitle,
        LV_TEXT_ALIGN_CENTER,
        0
    );

    lv_obj_set_style_text_color(
        label_subtitle,
        lv_color_hex(COLOR_TEXT_LIGHT),
        0
    );

    lv_obj_align(
        label_subtitle,
        LV_ALIGN_CENTER,
        0,
        30
    );
}

// =====================================================
// TELA PRINCIPAL
// =====================================================

void lv_create_main()
{
    scr_main = lv_obj_create(NULL);

    lv_obj_set_style_bg_color(
        scr_main,
        lv_color_hex(COLOR_BACKGROUND),
        0
    );

    lv_obj_clear_flag(
        scr_main,
        LV_OBJ_FLAG_SCROLLABLE
    );

    // Titulo.
    lv_obj_t* header = lv_label_create(scr_main);

    lv_label_set_text(
        header,
        "MONITORAMENTO"
    );

    lv_obj_set_style_text_font(
        header,
        &lv_font_montserrat_20,
        0
    );

    lv_obj_set_style_text_color(
        header,
        lv_color_hex(COLOR_TEXT),
        0
    );

    lv_obj_align(
        header,
        LV_ALIGN_TOP_LEFT,
        14,
        10
    );

    // Contador.
    label_counter = lv_label_create(scr_main);

    lv_label_set_text(
        label_counter,
        "Afericoes: 0/20"
    );

    lv_obj_set_style_text_font(
        label_counter,
        &lv_font_montserrat_12,
        0
    );

    lv_obj_set_style_text_color(
        label_counter,
        lv_color_hex(COLOR_TEXT_GRAY),
        0
    );

    lv_obj_align(
        label_counter,
        LV_ALIGN_TOP_LEFT,
        16,
        34
    );

    // Botao historico.
    lv_obj_t* btn_history = create_button(
        scr_main,
        "HISTORICO",
        85,
        30
    );

    lv_obj_align(
        btn_history,
        LV_ALIGN_TOP_RIGHT,
        -8,
        6
    );

    lv_obj_add_event_cb(
        btn_history,
        history_button_event,
        LV_EVENT_CLICKED,
        NULL
    );

    // Linha amarela.
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

    lv_obj_set_style_border_width(line, 0, 0);

    lv_obj_clear_flag(
        line,
        LV_OBJ_FLAG_SCROLLABLE
    );

    // Cards.
    label_bpm = create_card(
        scr_main,
        0,
        "Batimentos",
        "bpm",
        57
    );

    label_spo2 = create_card(
        scr_main,
        1,
        "Oximetria",
        "%",
        111
    );

    label_glucose = create_card(
        scr_main,
        2,
        "Glicemia",
        "mg/dL",
        165
    );
}

// =====================================================
// TELA HISTORICO
// =====================================================

void lv_create_history()
{
    scr_history = lv_obj_create(NULL);

    lv_obj_set_style_bg_color(
        scr_history,
        lv_color_hex(COLOR_BACKGROUND),
        0
    );

    lv_obj_clear_flag(
        scr_history,
        LV_OBJ_FLAG_SCROLLABLE
    );

    // Titulo.
    history_title = lv_label_create(scr_history);

    lv_label_set_text(
        history_title,
        "HISTORICO"
    );

    lv_obj_set_style_text_font(
        history_title,
        &lv_font_montserrat_20,
        0
    );

    lv_obj_set_style_text_color(
        history_title,
        lv_color_hex(COLOR_TEXT),
        0
    );

    lv_obj_align(
        history_title,
        LV_ALIGN_TOP_LEFT,
        14,
        8
    );

    // Media.
    history_average = lv_label_create(scr_history);

    lv_label_set_text(
        history_average,
        "Media: -- mg/dL"
    );

    lv_obj_set_style_text_font(
        history_average,
        &lv_font_montserrat_14,
        0
    );

    lv_obj_set_style_text_color(
        history_average,
        lv_color_hex(COLOR_YELLOW),
        0
    );

    lv_obj_align(
        history_average,
        LV_ALIGN_TOP_LEFT,
        14,
        34
    );

    // Grafico.
    history_chart = lv_chart_create(scr_history);

    lv_obj_set_size(
        history_chart,
        290,
        90
    );

    lv_obj_align(
        history_chart,
        LV_ALIGN_TOP_MID,
        0,
        55
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

    lv_obj_set_style_bg_color(
        history_chart,
        lv_color_hex(COLOR_WHITE),
        0
    );

    lv_obj_set_style_border_width(
        history_chart,
        0,
        0
    );

    lv_obj_set_style_radius(
        history_chart,
        12,
        0
    );

    lv_obj_set_style_line_width(
        history_chart,
        3,
        LV_PART_ITEMS
    );

    history_series = lv_chart_add_series(
        history_chart,
        lv_color_hex(COLOR_YELLOW),
        LV_CHART_AXIS_PRIMARY_Y
    );

    for (
        int i = 0;
        i < MAX_MEASUREMENTS;
        i++
    )
    {
        lv_chart_set_value_by_id(
            history_chart,
            history_series,
            i,
            LV_CHART_POINT_NONE
        );
    }

    // Botao voltar.
    lv_obj_t* btn_back = create_button(
        scr_history,
        "VOLTAR",
        82,
        28
    );

    lv_obj_align(
        btn_back,
        LV_ALIGN_BOTTOM_LEFT,
        12,
        -8
    );

    lv_obj_add_event_cb(
        btn_back,
        back_button_event,
        LV_EVENT_CLICKED,
        NULL
    );

    // Botao nova sessao.
    lv_obj_t* btn_reset = create_button(
        scr_history,
        "NOVA SESSAO",
        105,
        28
    );

    lv_obj_align(
        btn_reset,
        LV_ALIGN_BOTTOM_RIGHT,
        -12,
        -8
    );

    lv_obj_add_event_cb(
        btn_reset,
        reset_button_event,
        LV_EVENT_CLICKED,
        NULL
    );
}

// =====================================================
// CONTADOR DE AFERICOES
// =====================================================

void update_measurement_counter()
{
    if (label_counter == NULL)
    {
        return;
    }

    if (measurement_count == last_counter_value)
    {
        return;
    }

    last_counter_value = measurement_count;

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
}

// =====================================================
// ATUALIZAR GRAFICO
// =====================================================

void update_history_chart()
{
    if (
        history_chart == NULL ||
        history_series == NULL
    )
    {
        return;
    }

    for (
        int i = 0;
        i < MAX_MEASUREMENTS;
        i++
    )
    {
        lv_chart_set_value_by_id(
            history_chart,
            history_series,
            i,
            LV_CHART_POINT_NONE
        );
    }

    for (
        int i = 0;
        i < measurement_count &&
        i < MAX_MEASUREMENTS;
        i++
    )
    {
        int glucose = (int)glucose_history[i];

        glucose = constrain(glucose, 50, 240);

        lv_chart_set_value_by_id(
            history_chart,
            history_series,
            i,
            glucose
        );
    }

    lv_chart_refresh(history_chart);
}

// =====================================================
// ATUALIZAR MEDIA
// =====================================================

void update_history_average()
{
    if (history_average == NULL)
    {
        return;
    }

    char buffer[50];

    if (measurement_count <= 0)
    {
        snprintf(
            buffer,
            sizeof(buffer),
            "Media: -- mg/dL"
        );
    }
    else
    {
        float total = 0.0f;

        for (
            int i = 0;
            i < measurement_count &&
            i < MAX_MEASUREMENTS;
            i++
        )
        {
            total += glucose_history[i];
        }

        float average = total / measurement_count;

        snprintf(
            buffer,
            sizeof(buffer),
            "Media: %.1f mg/dL",
            average
        );
    }

    lv_label_set_text(
        history_average,
        buffer
    );
}

// =====================================================
// ATUALIZAR BPM / SPO2
// =====================================================

void update_sensor_values(
    int bpm,
    float spo2
)
{
    if (
        label_bpm != NULL &&
        bpm != last_bpm_displayed
    )
    {
        char buffer[16];

        snprintf(
            buffer,
            sizeof(buffer),
            "%d",
            bpm
        );

        lv_label_set_text(label_bpm, buffer);

        last_bpm_displayed = bpm;
    }

    int spo2_int = (int)spo2;

    if (
        label_spo2 != NULL &&
        spo2_int != last_spo2_displayed
    )
    {
        char buffer[16];

        snprintf(
            buffer,
            sizeof(buffer),
            "%d",
            spo2_int
        );

        lv_label_set_text(label_spo2, buffer);

        last_spo2_displayed = spo2_int;
    }

    latest_bpm = bpm;
    latest_spo2 = spo2;

    update_measurement_counter();
}

// =====================================================
// ADICIONAR GLICOSE
// =====================================================

void add_glucose_measurement(float glucose)
{
    int glucose_int = (int)glucose;

    if (
        label_glucose != NULL &&
        glucose_int != last_glucose_displayed
    )
    {
        char buffer[16];

        snprintf(
            buffer,
            sizeof(buffer),
            "%d",
            glucose_int
        );

        lv_label_set_text(label_glucose, buffer);

        last_glucose_displayed = glucose_int;
    }

    latest_glucose = glucose;

    // Salvar ate 20 afericoes.
    if (measurement_count < MAX_MEASUREMENTS)
    {
        glucose_history[measurement_count] = glucose;

        measurement_count++;

        Serial.print("Afericao registrada: ");
        Serial.print(measurement_count);
        Serial.print("/20 - Glicose: ");
        Serial.println(glucose);
    }

    update_measurement_counter();

    if (history_screen_open)
    {
        update_history_chart();
        update_history_average();
    }
}

// =====================================================
// ATUALIZAR TELA HISTORICO
// =====================================================

void update_history_screen()
{
    if (!history_screen_open)
    {
        return;
    }

    if (measurement_count != last_history_count)
    {
        last_history_count = measurement_count;

        update_history_chart();
        update_history_average();
    }
}

// =====================================================
// VERIFICAR 20 AFERICOES
// =====================================================

bool isMeasurementComplete()
{
    return measurement_count >= MAX_MEASUREMENTS;
}

void check_measurement_completion()
{
    if (
        isMeasurementComplete() &&
        !final_screen_shown
    )
    {
        final_screen_shown = true;

        Serial.println(
            "20 afericoes concluidas."
        );
    }
}

// =====================================================
// SETUP LVGL
// =====================================================

void lvgl_setup()
{
    Serial.println("Inicializando display...");

    // TFT.
    tft.begin();
    tft.setRotation(0);

    // LVGL.
    lv_init();

    // Touch.
    touchscreenSPI.begin(
        XPT2046_CLK,
        XPT2046_MISO,
        XPT2046_MOSI,
        XPT2046_CS
    );

    touchscreen.begin(touchscreenSPI);
    touchscreen.setRotation(0);

    // Buffer LVGL.
    lv_disp_draw_buf_init(
        &draw_buf,
        buf1,
        NULL,
        SCREEN_WIDTH * BUFFER_LINES
    );

    // Driver do display.
    static lv_disp_drv_t disp_drv;

    lv_disp_drv_init(&disp_drv);

    disp_drv.hor_res = SCREEN_WIDTH;
    disp_drv.ver_res = SCREEN_HEIGHT;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;

    lv_disp_drv_register(&disp_drv);

    // Driver do touch.
    static lv_indev_drv_t indev_drv;

    lv_indev_drv_init(&indev_drv);

    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchscreen_read;

    lv_indev_drv_register(&indev_drv);

    // Criar telas.
    lv_create_splash();
    lv_create_main();
    lv_create_history();

    // Mostrar splash.
    lv_scr_load(scr_splash);

    splash_start_ms = millis();
    splash_finished = false;

    Serial.println("Display inicializado.");
}

// =====================================================
// LOOP DO DISPLAY
// =====================================================

void display_loop()
{
    // Durante os 10 segundos, o spinner nativo
    // e animado automaticamente pelo LVGL.
    if (!splash_finished)
    {
        if (
            millis() - splash_start_ms >=
            SPLASH_MS
        )
        {
            // Remover o spinner ao terminar.
            if (spinner != NULL)
            {
                lv_obj_del(spinner);
                spinner = NULL;
            }

            lv_scr_load(scr_main);

            splash_finished = true;

            Serial.println(
                "Tela principal carregada."
            );
        }

        return;
    }

    // Solicitação de abertura do historico pelo touch.
    if (history_touch_request)
    {
        history_touch_request = false;

        if (!history_screen_open)
        {
            open_history_screen();
        }
    }

    // Atualizacoes normais.
    update_measurement_counter();
    update_history_screen();
    check_measurement_completion();

    // IMPORTANTE:
    // Nao chamar lv_timer_handler() aqui.
    // O .ino ja chama lv_timer_handler() a cada 10 ms.
    // Nao usar delay() aqui.
}

// =====================================================
// FUNCOES PUBLICAS AUXILIARES
// =====================================================

static void addMeasurementToHistory(float glucose)
{
    add_glucose_measurement(glucose);
}

static void add_glucose_history(float glucose)
{
    add_glucose_measurement(glucose);
}

static void updateDisplay(
    int bpm,
    float spo2,
    float glucose
)
{
    update_sensor_values(bpm, spo2);
    add_glucose_measurement(glucose);
}

static int getMeasurementCount()
{
    return measurement_count;
}

static float getFinalAverage()
{
    if (measurement_count <= 0)
    {
        return 0.0f;
    }

    float total = 0.0f;

    for (
        int i = 0;
        i < measurement_count &&
        i < MAX_MEASUREMENTS;
        i++
    )
    {
        total += glucose_history[i];
    }

    return total / measurement_count;
}

#endif
