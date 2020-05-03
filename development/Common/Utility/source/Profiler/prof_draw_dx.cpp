#include "Profiler/prof.h"
#include "prof_internal.h"
#include "IRenderer.h"
//#include "Math/Vector.h"

namespace
{
    //! make coordinates from bottom (0) to top ()-1)
    uint32_t pixelFix(uint32_t currValue, uint32_t height)
    {
        return height - currValue;
    }

    typedef struct
    {
        float x0,y0;
        float sx,sy;
        int resX, resY;
    } GraphLocation;


    void graph_func(int id, int x0, int x1, float *values, void *data)
    {
        GraphLocation *loc = (GraphLocation *) data;
        int i, r,g,b;

        // trim out values that are under 0.2 ms to accelerate rendering
        while (x0 < x1 && (*values < 0.0002f))
        {
            ++x0; ++values;
        }
        while (x1 > x0 && (values[x1-1-x0] < 0.0002f)) --x1;

        eg::Color4_t color = eg::v4::ONE();
        if (id == 0)
        {
            color.set(1, 1, 1, 1);
        }
        else
        {
            if (x0 == x1)
            {
                return;
            }

            id = (id >> 8) + id;
            r = id * 37;
            g = id * 59;
            b = id * 45;

            float rf = ((r & 127) + 80)/255.0f;
            float gf = ((g & 127) + 80)/255.0f;
            float bf = ((b & 127) + 80)/255.0f;
            float af = 1;

            color.set(rf, gf, bf, af);
        }

        if (x0 == x1)
        {
            float x,y;
            x = loc->x0 + x0 * loc->sx;
            y = loc->y0 + values[0] * loc->sy;
            eg::Vector3 begPos(x, (float)pixelFix(static_cast<uint32_t>(loc->y0), static_cast<uint32_t>(loc->resY)), 0);
            eg::Vector3 endPos(x, (float)pixelFix(static_cast<uint32_t>(y), static_cast<uint32_t>(loc->resY)), 0);

            //eg::DrawLine(begPos, endPos, color, color);
        }

        for (i=0; i < x1-x0;)
        {
            float x,y;
            x = loc->x0 + (i+x0) * loc->sx;
            y = loc->y0 + values[i] * loc->sy;
            eg::Vector3 begPos(x, (float)pixelFix(static_cast<uint32_t>(y), static_cast<uint32_t>(loc->resY)), 0);
            i++;

            x = loc->x0 + (i+x0) * loc->sx;
            y = loc->y0 + values[i] * loc->sy;
            eg::Vector3 endPos(x, (float)pixelFix(static_cast<uint32_t>(y), static_cast<uint32_t>(loc->resY)), 0);
            //i++;

            //eg::DrawLine(begPos, endPos, color, color);
        }
    }


    void draw_rectangle(float x0, float y0, float x1, float y1, const eg::Color4_t& /*color*/)
    {
        // FACE_CULL is disabled so winding doesn't matter
        eg::Vector3 p0(x0, y0, 0);
        eg::Vector3 p1(x1, y0, 0);
        eg::Vector3 p2(x1, y1, 0);
        eg::Vector3 p3(x0, y1, 0);
        //eg::DrawLine(p0, p1, color, color);
        //eg::DrawLine(p1, p2, color, color);
        //eg::DrawLine(p2, p3, color, color);
        //eg::DrawLine(p3, p0, color, color);
    }

    // use factor to compute a glow amount
    int get_colors(float factor, float text_color_ret[3], float glow_color_ret[3], float *glow_alpha_ret)
    {
        const float GLOW_RANGE = 0.5f;
        const float GLOW_ALPHA_MAX = 0.5f;
        float glow_alpha;
        int i;
        float hot[3] = {1, 1.0, 0.9};
        float cold[3] = {0.15, 0.9, 0.15};

        float glow_cold[3] = {0.5f, 0.5f, 0};
        float glow_hot[3] = {1.0f, 1.0f, 0};

        if (factor < 0) factor = 0;
        if (factor > 1) factor = 1;

        for (i=0; i < 3; ++i)
        {
            text_color_ret[i] = cold[i] + (hot[i] - cold[i]) * factor;
        }

        // Figure out whether to start up the glow as well.
        glow_alpha = (factor - GLOW_RANGE) / (1 - GLOW_RANGE);
        if (glow_alpha < 0)
        {
            *glow_alpha_ret = 0;
            return 0;
        }

        for (i=0; i < 3; ++i)
        {
            glow_color_ret[i] = glow_cold[i] + (glow_hot[i] - glow_cold[i]) * factor;
        }

        *glow_alpha_ret = glow_alpha * GLOW_ALPHA_MAX;
        return 1;
    }


    // float to string conversion with sprintf() was
    // taking up 10-20% of the Prof_draw time, so I
    // wrote a faster float-to-string converter
    char int_to_string[100][4];
    char int_to_string_decimal[100][4];
    char int_to_string_mid_decimal[100][4];
    void int_to_string_init(void)
    {
        int i;
        for (i=0; i < 100; ++i)
        {
            sprintf(int_to_string[i], "%d", i);
            sprintf(int_to_string_decimal[i], ".%02d", i);
            sprintf(int_to_string_mid_decimal[i], "%d.%d", i/10, i % 10);
        }
    }

    char *formats[5] =
    {
        "%.0f",
        "%.1f",
        "%.2f",
        "%.3f",
        "%.4f",
    };

    void float_to_string(char *buf, float num, int precision)
    {
        int x,y;
        switch (precision)
        {
        case 2:
            if (num < 0 || num >= 100)
                break;
            x = static_cast<int>(num);
            y = static_cast<int>((num - x) * 100);
            strcpy(buf, int_to_string[x]);
            strcat(buf, int_to_string_decimal[y]);
            return;
        case 3:
            if (num < 0 || num >= 10)
                break;
            num *= 10;
            x = static_cast<int>(num);
            y = static_cast<int>((num - x) * 100);
            strcpy(buf, int_to_string_mid_decimal[x]);
            strcat(buf, int_to_string_decimal[y]+1);
            return;
        case 4:
            if (num < 0 || num >= 1)
                break;
            num *= 100;
            x = static_cast<int>(num);
            y = static_cast<int>((num - x) * 100);
            buf[0] = '0';
            strcpy(buf+1, int_to_string_decimal[x]);
            strcat(buf, int_to_string_decimal[y]+1);
            return;
        }
        sprintf(buf, formats[precision], num);
    }

    float textWidth(char *str)
    {
        if (str && *str)
        {
            return static_cast<float>(strlen(str) * 8);
        }

        return 0;
    }


} // namespace



Prof_extern_C void Prof_draw_graph_dx(float sx, float sy, float x_spacing, float y_spacing, int resX, int resY)
{
#ifdef Prof_ENABLED
    Prof_Begin(iprof_draw_graph)
    GraphLocation loc = { sx, sy, x_spacing, y_spacing * 1000, resX, resY};
    Prof_graph(128, graph_func, &loc);
    Prof_End
#endif // Prof_ENABLED
}


Prof_extern_C void Prof_draw_dx(float sx, float sy, float full_width, float height, float line_spacing, int precision)
{
#ifdef Prof_ENABLED
    Prof_Begin(iprof_draw)

    int i,j,n,o;
    float backup_sy;

    float field_width = textWidth("5555.55");
    //float name_width  = full_width - field_width * 3;
    float plus_width  = textWidth("+");
    eg::Color4_t color(eg::v4::ONE());

    int max_records;

    Prof_Report *pob;

    if (!int_to_string[0][0]) int_to_string_init();

    if (precision < 1) precision = 1;
    if (precision > 4) precision = 4;

    pob = Prof_create_report();

    for (i=0; i < NUM_TITLE; ++i)
    {
        if (pob->title[i])
        {
            float header_x0 = sx;
            float header_x1 = header_x0 + full_width;

            color.set(0.2f, 0.1f, 0.1f, 0.85f);
            if (i == 0)
            {
                color.set(0.1f, 0.3f, 0, 0.85f);
            }

            //draw_rectangle(header_x0, sy-4, header_x1, sy+line_spacing, color);
            draw_rectangle(header_x0, sy, header_x1, sy+line_spacing, color);

            color.set(0.8f, 0.1f, 0.1f, 1);
            if (i == 0)
            {
                color.set(0.6f, 0.4f, 0, 1);
            }

            //eg::Print(static_cast<uint32_t>(sx+2), static_cast<uint32_t>(sy+2), pob->title[i], color);

            sy += 1.5f*line_spacing;
            height -= abs(line_spacing)*1.5f;
        }
    }

    max_records = static_cast<int>(height / abs(line_spacing));

    o = 0;
    n = pob->num_record;
    if (n > max_records) n = max_records;
    if (pob->hilight >= o + n)
    {
        o = pob->hilight - n + 1;
    }

    backup_sy = sy;

    // Draw the background colors for the zone data.
    color.set(0, 0, 0, 0.85f);
    draw_rectangle(sx, sy, sx + full_width, sy + line_spacing, color);
    sy += line_spacing;

    for (i = 0; i < n; i++)
    {
        float y0, y1;

        if (i & 1)
        {
            color.set(0.1f, 0.1f, 0.2f, 0.85f);
        }
        else
        {
            color.set(0.1f, 0.1f, 0.3f, 0.85f);
        }
        if (i+o == pob->hilight)
        {
            color.set(0.3f, 0.3f, 0.1f, 0.85f);
        }

        y0 = sy;
        y1 = sy + line_spacing;

        draw_rectangle(sx, y0, sx + full_width, y1, color);
        sy += line_spacing;
    }

    sy = backup_sy;
    color.set(0.7f, 0.7f, 0.7f, 1);

    if (pob->header[0])
    {
        //eg::Print(static_cast<uint32_t>(sx+8), static_cast<uint32_t>(sy+2), pob->header[0], color);
    }

    for (j=1; j < NUM_HEADER; ++j)
    {
        if (pob->header[j])
        {
            //eg::Print(static_cast<uint32_t>(sx + name_width + field_width * (j-1) + field_width/2 - textWidth(pob->header[j])/2),
            //          static_cast<uint32_t>(sy+2),
            //          pob->header[j],
            //          color);
        }
    }

    sy += line_spacing;

    for (i = 0; i < n; i++)
    {
        char buf[256], *b = buf;
        Prof_Report_Record *r = &pob->record[i+o];
        float text_color[3], glow_color[3];
        float glow_alpha;
        float x = sx + textWidth(" ") * r->indent + plus_width/2;
        if (r->prefix)
        {
            buf[0] = r->prefix;
            ++b;
        }
        else
        {
            x += plus_width;
        }
        if (r->number)
            sprintf(b, "%s (%d)", r->name, r->number);
        else
            sprintf(b, "%s", r->name);
        if (get_colors(static_cast<float>(r->heat), text_color, glow_color, &glow_alpha))
        {
            color.set(glow_color[0], glow_color[1], glow_color[2], glow_alpha);
            //eg::Print(static_cast<uint32_t>(x+2), static_cast<uint32_t>(sy+2), buf, color);
        }
        color.set(text_color[0], text_color[1], text_color[2], 1);
        //eg::Print(static_cast<uint32_t>(x + 1), static_cast<uint32_t>(sy+2), buf, color);

        for (j=0; j < NUM_VALUES; ++j)
        {
            if (r->value_flag & (1 << j))
            {
                int pad;
                float_to_string(buf, static_cast<float>(r->values[j]), j == 2 ? 2 : precision);
                pad = static_cast<int>(field_width- plus_width - textWidth(buf));
                if (r->indent)
                {
                    pad += static_cast<int>(plus_width);
                }
                //eg::Print(static_cast<uint32_t>(sx + pad + name_width + field_width * j), static_cast<uint32_t>(sy+2), buf, color);
            }
        }

        sy += line_spacing;
    }

    Prof_free_report(pob);

    Prof_End
#endif // Prof_ENABLED
}

