#include "Math/Vector.h"
#include "App/MainConsole.h"
#include "Profiler/prof.h"
#include "prof_internal.h"



namespace
{

    void draw_rectangle(int x0, int y0, int x1, int y1, eg::MainConsole::Color color)
    {
        eg::MainConsole::SetColor(eg::MainConsole::Black, color);
        const char *space = "                                                                                                              ";
        for (int y = y0; y < y1; y++)
        {
            eg::MainConsole::ConPrintAt(x0, y, space,x1-x0);
        }


        /*
        // FACE_CULL is disabled so winding doesn't matter
        eg::Vector3 p0(x0, y0, 0);
        eg::Vector3 p1(x1, y0, 0);
        eg::Vector3 p2(x1, y1, 0);
        eg::Vector3 p3(x0, y1, 0);
        //eg::DrawLine(p0, p1, color, color);
        //eg::DrawLine(p1, p2, color, color);
        //eg::DrawLine(p2, p3, color, color);
        //eg::DrawLine(p3, p0, color, color);
        */

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

    float stringWidth(char *str)
    {
        if (str && *str)
        {
            return static_cast<float>(strlen(str));
        }

        return 0;
    }


} // namespace



Prof_extern_C void Prof_draw_console(float sx, float sy, float full_width, float height, float /*line_spacing*/, int precision)
{
#ifdef Prof_ENABLED
    eg::MainConsole::ClearConsole(eg::MainConsole::Black, eg::MainConsole::Black);

    Prof_Begin(iprof_draw)

    int i,j,n,o;

    float field_width = stringWidth("55555.5555");
    float name_width  = (full_width/8) - field_width * 3;
    float plus_width  = stringWidth("+");
    eg::Color4_t color(eg::v4::ONE());

    int max_records;

    Prof_Report *pob;

    if (!int_to_string[0][0]) int_to_string_init();

    if (precision < 1) precision = 1;
    if (precision > 4) precision = 4;

    pob = Prof_create_report();
    int psx = static_cast<int>(sx/8.0f);
    int psy = static_cast<int>(sy/8.0f);
    int pfull_width = static_cast<int>(full_width/8.0f);
    int pheight = static_cast<int>(height/8.0f);
    int pline_spacing = 1;
    eg::MainConsole::Color pcolor = eg::MainConsole::White;

    for (i=0; i < NUM_TITLE; ++i)
    {
        if (pob->title[i])
        {
            float header_x0 = static_cast<float>(psx);
            float header_x1 = header_x0 + pfull_width;

            pcolor = eg::MainConsole::DGray;
            if (i == 0)
            {
                pcolor = eg::MainConsole::Green;
            }

            draw_rectangle(static_cast<int>(header_x0), psy, static_cast<int>(header_x1), psy+pline_spacing, pcolor);

            pcolor = eg::MainConsole::Red;
            if (i == 0)
            {
                pcolor = eg::MainConsole::LYellow;
            }

            eg::MainConsole::SetColor(pcolor);
            eg::MainConsole::ConPrintAt(psx, psy, pob->title[i]);

            psy++;
            pheight--;
        }
    }

    max_records = pheight / abs(pline_spacing);

    o = 0;
    n = pob->num_record;
    if (n > max_records) n = max_records;
    if (pob->hilight >= o + n)
    {
        o = pob->hilight - n + 1;
    }

    float backup_sy = static_cast<float>(psy);

    // Draw the background colors for the zone data.
    pcolor = eg::MainConsole::Black;
    draw_rectangle(psx, psy, psx + pfull_width, psy + pline_spacing, pcolor);
    psy++;

    for (i = 0; i < n; i++)
    {
        int y0, y1;

        pcolor = eg::MainConsole::Gray;//LBlue;
        if (i & 1)
        {
            pcolor = eg::MainConsole::DGray;//Blue;
        }
        if (i+o == pob->hilight)
        {
            pcolor = eg::MainConsole::Red;
        }

        y0 = psy;
        y1 = psy + 1;

        draw_rectangle(psx, y0, psx + pfull_width, y1, pcolor);
        psy++;
    }

    psy = static_cast<int>(backup_sy);
    pcolor = eg::MainConsole::Gray;
    eg::MainConsole::SetColor(pcolor, eg::MainConsole::Black);

    if (pob->header[0])
    {
        eg::MainConsole::ConPrintAt(psx+1, psy, pob->header[0]);
    }

    for (j=1; j < NUM_HEADER; ++j)
    {
        if (pob->header[j])
        {
            eg::MainConsole::ConPrintAt(static_cast<int>(psx + name_width + field_width * (j-1) + field_width/2 - stringWidth(pob->header[j])/2),
                                        psy,
                                        pob->header[j]);
        }
    }
    psy++;

    pcolor = eg::MainConsole::Blue;
    for (i = 0; i < n; i++)
    {
        eg::MainConsole::Color bColor = eg::MainConsole::DGray;
        char buf[256], *b = buf;
        Prof_Report_Record *r = &pob->record[i+o];
        //float text_color[3], glow_color[3];
        //float glow_alpha;
        float x = sx + stringWidth(" ") * r->indent + plus_width/2;
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
        /*
        if (get_colors(r->heat, text_color, glow_color, &glow_alpha))
        {
            color.set(glow_color[0], glow_color[1], glow_color[2], glow_alpha);
            //eg::Print(x+2, sy+2, buf, color);
        }
        */
        //color.set(text_color[0], text_color[1], text_color[2], 1);
        bColor = eg::MainConsole::Gray;//LBlue;
        if (i & 1)
        {
            bColor = eg::MainConsole::DGray;//Blue;
        }
        if (i+o == pob->hilight)
        {
            bColor = eg::MainConsole::Red;
        }
        eg::MainConsole::SetColor(pcolor, bColor);
        eg::MainConsole::ConPrintAt(static_cast<int>(x), psy, buf);

        for (j=0; j < NUM_VALUES; ++j)
        {
            if (r->value_flag & (1 << j))
            {
                int pad;
                float_to_string(buf, static_cast<float>(r->values[j]), j == 2 ? 2 : precision);
                pad = static_cast<int>(field_width - plus_width - stringWidth(buf));
                if (r->indent)
                {
                    pad += static_cast<int>(plus_width);
                }
                eg::MainConsole::ConPrintAt(static_cast<int>(psx + pad + name_width + field_width * j), psy, buf);
            }
        }

        psy++;
    }

    Prof_free_report(pob);

    Prof_End
#endif // Prof_ENABLED
}

