#include "application.hpp"

#include <iostream>

#include <nanogui/opengl.h>

#include "cube_texture.hpp"
#include "light-field-renderer.hpp"

#include <unistd.h>

constexpr float Pi = 3.14159f;
#define ENABLE_TEXTURED_CUBE_CANVAS 0
#define TEXTURE_CANVAS_FILE_PATH ("/shop-1-row/Original Camera_00_00_400.000000_400.000000_30_36.jpg")

Application::Application() : 
    //Screen(nanogui::Vector2i(1470, 750), "Light Field Renderer", true, false, false, false, false, 3U, 1U), 
    Screen(nanogui::Vector2i(1470, 750), "Light Field Renderer", true, false, /* depth buffer */ true, true, false, 3U, 1U), 
    cfg(std::make_shared<Config>())
{
    inc_ref();

    auto theme = new nanogui::Theme(this->nvg_context());
    theme->m_window_fill_focused = nanogui::Color(45, 255);
    theme->m_window_fill_unfocused = nanogui::Color(45, 255);
    theme->m_drop_shadow = nanogui::Color(0, 0);

    nanogui::Window *window;
    nanogui::Button* b;
    nanogui::Widget* panel;
    nanogui::Label* label;

    window = new nanogui::Window(this, "Render");
    window->set_position(nanogui::Vector2i(410, 10));
    window->set_layout(new nanogui::GroupLayout());
    window->set_theme(theme);

    light_field_renderer = new LightFieldRenderer(window, cfg);
    light_field_renderer->set_visible(true);

    b = new nanogui::Button(window->button_panel(), "", FA_CAMERA);
    b->set_tooltip("Save Screenshot");
    b->set_callback([this]
    {
        std::string path = nanogui::file_dialog({{"tga", ""}}, true);
        if (path.empty()) return;
        light_field_renderer->saveNextRender(path);
    });

    window = new nanogui::Window(this, "Menu");
    window->set_position({ 10, 10 });
    window->set_layout(new nanogui::GroupLayout(15, 6, 15, 0));
    window->set_theme(theme);

    nanogui::PopupButton *info = new nanogui::PopupButton(window->button_panel(), "", FA_QUESTION);
    info->set_font_size(15);
    info->set_tooltip("Info");
    nanogui::Popup *info_panel = info->popup();

    info_panel->set_layout(new nanogui::GroupLayout());

    auto addText = [&info_panel](std::string text, std::string font = "sans", int font_size = 18) 
    {
        auto row = new Widget(info_panel);
        row->set_layout(new nanogui::BoxLayout(nanogui::Orientation::Vertical, nanogui::Alignment::Middle, 0, 10));
        new nanogui::Label(row, text, font, font_size);
    };

    auto addControl = [&info_panel](std::string keys, std::string desc) 
    {
        auto row = new nanogui::Widget(info_panel);
        row->set_layout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal, nanogui::Alignment::Fill, 0, 10));
        auto desc_widget = new nanogui::Label(row, keys, "sans-bold");
        desc_widget->set_fixed_width(140);
        new nanogui::Label(row, desc);
        return desc_widget;
    };

    addText("Light Field Renderer", "sans-bold", 24);
    addText("github.com/linusmossberg/light-field-renderer", "sans", 16);
    addText(" ");

    addControl("W", "Move forward");
    addControl("A", "Move left");
    addControl("S", "Move back");
    addControl("D", "Move right");
    addControl("SPACE", "Move up");
    addControl("CTRL", "Move down");
    addControl("SCROLL", "Change focus distance");
    addControl("SHIFT + CLICK", "Autofocus");
    addControl("MOUSE DRAG", "Mouse mode");

    panel = new nanogui::Widget(window);
    panel->set_layout(new nanogui::GridLayout(nanogui::Orientation::Horizontal, 2, nanogui::Alignment::Fill, 0, 5));

    label = new nanogui::Label(panel, "Light Field", "sans-bold");
    label->set_fixed_width(86);

    b = new nanogui::Button(panel, "Open", FA_FOLDER_OPEN);
    b->set_fixed_size({ 270, 20 });
    b->set_tooltip("Select any file from the light field folder.");
    b->set_callback([this]
    {
        char cwd[PATH_MAX] = "";
        getcwd(cwd, sizeof(cwd));

        std::string path_cfg = std::string(cwd) + "/shop-1-row/config.cfg";
        int path_cfg_exists = access (path_cfg.c_str(), F_OK);

        std::string path;
        if (path_cfg_exists == 0) {
            path = path_cfg;
        }
        else {
            path = nanogui::file_dialog
            (
                { 
                    {"cfg", ""}, {"jpeg", ""}, { "jpg","" }, {"png",""}, 
                    {"tga",""}, {"bmp",""}, {"psd",""}, {"gif",""}, 
                    {"hdr",""}, {"pic",""}, {"pnm", ""} 
                }, false
            );
        }

        if (path.empty()) return;

        try
        {
            cfg->open(path);
            light_field_renderer->open();

            light_field_renderer->resize();
            perform_layout();
        }
        catch (const std::exception &ex)
        {
            std::cout << ex.what() << std::endl;
        }
    });


    panel = new nanogui::Widget(window);
    panel->set_layout(new nanogui::GridLayout(nanogui::Orientation::Horizontal, 4, nanogui::Alignment::Fill));
    label = new nanogui::Label(panel, "Render Size", "sans-bold");
    label->set_fixed_width(86);

    float_box_rows.push_back(PropertyBoxRow(panel, { &cfg->width, &cfg->height }, "", "px", 0, 1.0f, "", 180));

    b = new nanogui::Button(panel, "Set");
    b->set_font_size(16);
    b->set_fixed_size({ 90, 20 });
    b->set_callback([this, window]
        { 
            light_field_renderer->resize();
            perform_layout();
        }
    );

    sliders.emplace_back(window, &cfg->exposure, "Exposure", "EV", 1);

    new nanogui::Label(window, "Optics", "sans-bold", 20);
    sliders.emplace_back(window, &cfg->focal_length, "Focal Length", "mm", 2);
    sliders.emplace_back(window, &cfg->sensor_width, "Sensor Width", "mm", 2);
    sliders.emplace_back(window, &cfg->focus_distance, "Focus Distance", "m", 2);

    sliders.emplace_back(window, &cfg->f_stop, "F-Stop", "", 2);
    sliders.emplace_back(window, &cfg->aperture_falloff, "Filter Falloff", "", 2);

    panel = new Widget(window);
    panel->set_layout(new nanogui::GridLayout(nanogui::Orientation::Horizontal, 3, nanogui::Alignment::Fill, 0, 5));

    label = new nanogui::Label(panel, "Option", "sans-bold");
    label->set_fixed_width(86);

    nanogui::Button* normalize = new nanogui::Button(panel, "Normalize Aperture");
    normalize->set_fixed_size({ 125, 20 });
    normalize->set_font_size(14);
    normalize->set_tooltip("Normalize aperture filter weights per pixel. Disabling this results in vignetting.");
    normalize->set_flags(nanogui::Button::Flags::ToggleButton);
    normalize->set_pushed(light_field_renderer->normalize_aperture);
    normalize->set_change_callback([this](bool state)
    {
        light_field_renderer->normalize_aperture = state;
    });

    nanogui::Button* breathing = new nanogui::Button(panel, "Focus Breathing");
    breathing->set_fixed_size({ 125, 20 });
    breathing->set_font_size(14);
    breathing->set_tooltip("Factor in the focus distance to the image distance according to the thin-lens equation.");
    breathing->set_flags(nanogui::Button::Flags::ToggleButton);
    breathing->set_pushed(light_field_renderer->focus_breathing);
    breathing->set_change_callback([this](bool state)
    {
        light_field_renderer->focus_breathing = state;
    });

    new nanogui::Label(window, "Navigation", "sans-bold", 20);

    panel = new nanogui::Widget(window);
    panel->set_layout(new nanogui::GridLayout(nanogui::Orientation::Horizontal, 4, nanogui::Alignment::Fill, 0, 5));

    label = new nanogui::Label(panel, "Mode", "sans-bold");
    label->set_fixed_width(86);

    nanogui::Button* free = new nanogui::Button(panel, "Free", FA_STREET_VIEW);
    free->set_flags(nanogui::Button::Flags::ToggleButton);
    free->set_pushed(light_field_renderer->navigation == LightFieldRenderer::Navigation::FREE);
    free->set_fixed_size({ 83, 20 });
    free->set_font_size(16);
    free->set_tooltip("The camera is rotated freely with the mouse.");

    nanogui::Button* target = new nanogui::Button(panel, "Target", FA_BULLSEYE);
    target->set_flags(nanogui::Button::Flags::ToggleButton);
    target->set_pushed(light_field_renderer->navigation == LightFieldRenderer::Navigation::TARGET);
    target->set_fixed_size({ 83, 20 });
    target->set_font_size(16);
    target->set_tooltip("The camera looks at the target coordinate and the mouse controls the camera position.");

    nanogui::Button* animate = new nanogui::Button(panel, "Animate", FA_BEZIER_CURVE);
    animate->set_flags(nanogui::Button::Flags::ToggleButton);
    animate->set_pushed(light_field_renderer->navigation == LightFieldRenderer::Navigation::ANIMATE);
    animate->set_fixed_size({ 83, 20 });
    animate->set_font_size(16);

    free->set_change_callback
    (
        [this, free, target, animate](bool state)
        {
            light_field_renderer->navigation = LightFieldRenderer::Navigation::FREE;
            free->set_pushed(true);
            target->set_pushed(false);
            animate->set_pushed(false);
        }
    );
    target->set_change_callback
    (
        [this, free, target, animate](bool state)
        {
            light_field_renderer->navigation = LightFieldRenderer::Navigation::TARGET;
            free->set_pushed(false);
            target->set_pushed(true);
            animate->set_pushed(false);
        }
    );
    animate->set_change_callback
    (
        [this, free, target, animate](bool state)
        {
            light_field_renderer->navigation = LightFieldRenderer::Navigation::ANIMATE;
            free->set_pushed(false);
            target->set_pushed(false);
            animate->set_pushed(true);
        }
    );

    float_box_rows.push_back(PropertyBoxRow(window, { &cfg->x, &cfg->y, &cfg->z }, "Position", "m", 3, 0.1f));
    float_box_rows.push_back(PropertyBoxRow(window, { &cfg->yaw, &cfg->pitch }, "Rotation", "�", 1, 1.0f));
    float_box_rows.push_back(PropertyBoxRow(window, { &cfg->target_x, &cfg->target_y, &cfg->target_z }, "Target", "m", 3, 1.0f));

    sliders.emplace_back(window, &cfg->speed, "Speed", "m/s", 2);

    panel = new nanogui::Widget(window);
    panel->set_layout(new nanogui::GridLayout(nanogui::Orientation::Horizontal, 2, nanogui::Alignment::Fill, 0, 5));

    label = new nanogui::Label(panel, " ", "sans-bold");
    label->set_fixed_width(86);

    nanogui::PopupButton *popup_btn = new nanogui::PopupButton(panel, "Animation Settings");
    popup_btn->set_font_size(16);
    popup_btn->set_fixed_size({ 270, 20 });
    nanogui::Popup *popup = popup_btn->popup();
    popup->set_layout(new nanogui::GroupLayout());

    sliders.emplace_back(popup, &cfg->animation_sway, "Sway", "", 2);
    sliders.emplace_back(popup, &cfg->animation_depth, "Depth", "", 2);
    sliders.emplace_back(popup, &cfg->animation_cycles, "Cycles", "", 0);
    sliders.emplace_back(popup, &cfg->animation_scale, "Scale", "", 2);
    sliders.emplace_back(popup, &cfg->animation_duration, "Loop Duration", "s", 2);

    new nanogui::Label(window, "Autofocus", "sans-bold", 20);

    panel = new Widget(window);
    panel->set_layout(new nanogui::GridLayout(nanogui::Orientation::Horizontal, 4, nanogui::Alignment::Fill, 0, 5));

    label = new nanogui::Label(panel, "Option", "sans-bold");
    label->set_fixed_width(86);

    nanogui::Button* focus_af = new nanogui::Button(panel, "Focus");
    focus_af->set_fixed_size({ 83, 20 });
    focus_af->set_font_size(14);
    focus_af->set_tooltip("Can also be initiated by using shift+click in the render view");
    focus_af->set_callback([this]()
    {
        light_field_renderer->autofocus_click = true;
    });

    nanogui::Button* continuous_af = new nanogui::Button(panel, "Continuous");
    continuous_af->set_flags(nanogui::Button::Flags::ToggleButton);
    continuous_af->set_font_size(14);
    continuous_af->set_pushed(light_field_renderer->continuous_autofocus);
    continuous_af->set_fixed_size({ 83, 20 });
    continuous_af->set_change_callback([this](bool state)
    {
        light_field_renderer->continuous_autofocus = state;
    });

    nanogui::Button* visualize_af = new nanogui::Button(panel, "Visualize");
    visualize_af->set_flags(nanogui::Button::Flags::ToggleButton);
    visualize_af->set_pushed(light_field_renderer->visualize_autofocus);
    visualize_af->set_font_size(14);
    visualize_af->set_fixed_size({ 83, 20 });
    visualize_af->set_change_callback([this](bool state)
    {
        light_field_renderer->visualize_autofocus = state;
    });

    float_box_rows.push_back(PropertyBoxRow(
        window, { &cfg->autofocus_x, &cfg->autofocus_y }, "Screen Point", "", 2, 0.01f, 
        "Can also be set using shift+click in the render view")
    );
    float_box_rows.push_back(PropertyBoxRow(
        window, { &cfg->template_size }, "Template Size", "px", 0, 2.0f, 
        "Size of the focus region.")
    );
    float_box_rows.push_back(PropertyBoxRow(
        window, { &cfg->search_scale }, "Search Scale", "", 1, 0.1f, 
        "Scale of search region relative to the focus region")
    );

    label = new nanogui::Label(window, "Light Slab", "sans-bold", 20);
    label->set_tooltip("Scale of rectified light fields. Has no effect on unrectified light fields.");
    sliders.emplace_back(window, &cfg->st_width, "ST Width", "m", 1);
    sliders.emplace_back(window, &cfg->st_distance, "ST Distance", "m", 1);

    //----- new window for GLES 3.1 Canvas -----
#if ENABLE_TEXTURED_CUBE_CANVAS
    // locate the default textue image file.
    char cwd[PATH_MAX] = "";
    getcwd(cwd, sizeof(cwd));

    std::string texture_file_path = std::string(cwd) + TEXTURE_CANVAS_FILE_PATH;
        
    int texture_file_exists = access (texture_file_path.c_str(), F_OK);

    // only create texture window if the texture file exists
    if (texture_file_exists == 0) {
        nanogui::Window *texture_window = new nanogui::Window(this, "Textured Cube");
        texture_window->set_position(nanogui::Vector2i(700, 15));
        texture_window->set_layout(new nanogui::GroupLayout());
        
        // texture canvas
        m_textureCanvas = new MyTextureCanvas(texture_window, texture_file_path.c_str());
        m_textureCanvas->set_background_color(nanogui::Color(32, 32, 128, 255));
        m_textureCanvas->set_fixed_size({500, 500});

        nanogui::Widget *texture_tools = new nanogui::Widget(texture_window);
        texture_tools->set_layout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal,
                                        nanogui::Alignment::Middle, 0, 5));
        nanogui::Button *t_b0 = new nanogui::Button(texture_tools, "Random Background");
        t_b0->set_callback([this]() {
            m_textureCanvas->set_background_color(
                nanogui::Vector4i(rand() % 256, rand() % 256, rand() % 256, 255));
        });

        nanogui::Button *t_b1 = new nanogui::Button(texture_tools, "Random Rotation");
        t_b1->set_callback([this]() {
            m_textureCanvas->set_rotation((float) Pi * rand() / (float) RAND_MAX);
        });
    }
#endif //ENABLE_TEXTURED_CUBE_CANVAS

    perform_layout();
}

bool Application::keyboard_event(int key, int scancode, int action, int modifiers) 
{
    if (Screen::keyboard_event(key, scancode, action, modifiers))
    {
        return true;
    }
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) 
    {
        set_visible(false);
        return true;
    }
    light_field_renderer->keyboardEvent(key, scancode, action, modifiers);
    return false;
}

void Application::draw(NVGcontext *ctx)
{
    for (auto &s : sliders)
    {
        s.updateValue();
    }

    for (auto &t : float_box_rows)
    {
        t.updateValues();
    }

    Screen::draw(ctx);
}
