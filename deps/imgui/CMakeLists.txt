add_library(imgui STATIC)
target_sources(imgui
    PUBLIC
        imgui/imgui.h
        imgui/imconfig.h
        imgui/imstb_rectpack.h
        imgui/imstb_textedit.h
        imgui/imstb_truetype.h
    PRIVATE
        imgui/imgui_internal.h
)

target_sources(imgui
    PRIVATE
        imgui/imgui.cpp
        imgui/imgui_demo.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_tables.cpp
        imgui/imgui_widgets.cpp    
)

target_include_directories(imgui PUBLIC .)

if (MSVC)
    target_compile_definitions(imgui PRIVATE "_USE_MATH_DEFINES")
endif()
