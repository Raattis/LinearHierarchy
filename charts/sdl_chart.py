import pygame, math
from pygame import Rect

def pause():
    get_out = False
    while not get_out:
        while not pygame.event.peek(pygame.KEYDOWN):
              pygame.time.delay(10)
        for e in pygame.event.get(pygame.KEYDOWN):
            if e.key == pygame.K_ESCAPE:
                pygame.quit()
                exit(0)
            elif e.key in [pygame.K_SPACE, pygame.K_LCTRL, pygame.K_RCTRL, pygame.K_RETURN]:
                get_out = True
                break
        pygame.event.clear()
    

def make_chart(surface, name, headers, series_names, series, min_series = None, max_series = None):
    global image_counter
    
    if do_normalize:
        temp = []
        for row in range(0, len(series)):
            temp.append([])
            for col in range(0, len(series[0])):
                temp[row].append(series[row][col] / headers[col])
        series = temp
        
        if min_series != None and max_series != None:
            temp_min = []
            temp_max = []
            for row in range(0, len(min_series)):
                temp_min.append([])
                temp_max.append([])
                for col in range(0, len(min_series[0])):
                    temp_min[row].append(min_series[row][col] / headers[col])
                    temp_max[row].append(max_series[row][col] / headers[col])
            min_series = temp_min
            max_series = temp_max
    
    maxValue = 0
    if min_series != None and max_series != None and do_min_max:
        for row in range(0, len(max_series)):
            if series_names[row] in ignore_from_max or series_names[row] in ignore_series:
                continue
            for col in range(0, len(max_series[row])):
                if(max_series[row][col] > maxValue):
                    maxValue = max_series[row][col]
    else:
        for row in range(0, len(series)):
            if series_names[row] in ignore_from_max or series_names[row] in ignore_series:
                continue
            for col in range(0, len(series[row])):
                if(series[row][col] > maxValue):
                    maxValue = series[row][col]
    
    if(maxValue == 0):
        return
    
    distances = []
    if True:
        minDistance = ((math.log10(float(headers[0]))))
        deltaDistance = ((math.log10(float(headers[-1])))) - minDistance
        for h in headers:
            distances.append((math.log10(float(h)) - minDistance) / deltaDistance)
        
    height_label_digits = 0
    height_whisker_count = 8
    step_size = maxValue / float(height_whisker_count)
    if step_size > 200:
       step_size = math.ceil(step_size / 200.0) * 200.0 
    elif step_size > 100:
       step_size = math.ceil(step_size / 100.0) * 100.0 
    elif step_size > 50:
       step_size = math.ceil(step_size / 50.0) * 50.0 
    elif step_size > 10:
       step_size = math.ceil(step_size / 10.0) * 10.0 
    elif step_size > 1:
       step_size = math.ceil(step_size)
    elif step_size > 0.1:
       step_size = math.ceil(step_size * 10.0) / 10.0
       height_label_digits = 1
    elif step_size > 0.01:
       step_size = math.ceil(step_size * 100.0) / 100.0
       height_label_digits = 2
    else:
       step_size = math.ceil(step_size * 1000.0) / 1000.0
       height_label_digits = 3

    while(step_size * (height_whisker_count-1) > maxValue):
        height_whisker_count -= 1
    if(step_size * (height_whisker_count-1) < maxValue * 0.98):
        height_whisker_count += 1
    
    maxValue = step_size * (height_whisker_count-1)
    
    surface.fill((255,255,255))

    offset = 5
    # border whiskers
    if True:
        length = 5
        top = chart_margins.top - offset
        left = chart_margins.left - offset

        text = None
        if "normaliz" in name:
            text = label_font.render("duration per node (µs/node)", True, (10,10,10))
        else:
            text = label_font.render("duration (µs)", True, (10,10,10))
        text = pygame.transform.rotate(text, 90)
        surface.blit(text, (7, chart_margins.centery - text.get_height() * 0.5))

        for i in range(0, height_whisker_count):
            value = step_size * i
            if(step_size >= 1):
                value = int(value)
            y = (1 - value / maxValue) * chart_margins.height + chart_margins.top
            
            pygame.draw.aaline(surface, (200,200,200), (left, y), (chart_margins.right, y), 1)

            pygame.draw.aaline(surface, (0,0,0), (left - length, y), (left, y), 1)
            
            t = '{0:.{1}f}'.format(value, height_label_digits)
            text = label_font.render(t, True, (10,10,10))
            surface.blit(text, (left - text.get_width() - length - 3, y - text.get_height() * 0.5))
            
    # bottom whiskers
    if True:
        length = 5
        top = chart_margins.bottom + offset
        left = chart_margins.left - offset
        
        text = label_font.render("number of nodes", True, (10,10,10))
        surface.blit(text, (chart_margins.centerx - text.get_width() * 0.5, resolution[1] - text.get_height() - 10))

        for i in range(0, len(headers)):
            x = left + distances[i] * chart_margins.width
            y = top
            pygame.draw.aaline(surface, (0,0,0), (x, y), (x, y + length), 1)

            text = label_font.render(str(headers[i]), True, (10,10,10))
            text = pygame.transform.rotate(text, 90)
            surface.blit(text, (x - text.get_width() * 0.5, y + length + 3))

    # border lines
    if True:
        topleft = (chart_margins.left - offset, chart_margins.top - offset)
        bottomleft = (chart_margins.left - offset, chart_margins.bottom + offset)
        bottomright = (chart_margins.right + offset, chart_margins.bottom + offset)
        pygame.draw.aalines(surface, (0,0,0), False, (topleft, bottomleft, bottomright), 1)

    # min to max lines
    if min_series != None and max_series != None and do_min_max:
        for j in range(0, len(min_series)):
            if(series_names[j] in ignore_series):
                continue
            min_ser = min_series[j]
            max_ser = max_series[j]
            length = 5
            top = chart_margins.bottom + offset
            left = chart_margins.left - offset
            
            text = label_font.render("number of nodes", True, (10,10,10))
            surface.blit(text, (chart_margins.centerx - text.get_width() * 0.5, resolution[1] - text.get_height() - 10))

            for i in range(0, len(min_ser)):
                x = distances[i] * float(chart_margins.width) + float(chart_margins.left) + (j-3)*3
                y1 = (1 - (min_ser[i] / maxValue)) * float(chart_margins.height) + float(chart_margins.top)
                y2 = (1 - (max_ser[i] / maxValue)) * float(chart_margins.height) + float(chart_margins.top)
                pygame.draw.aaline(surface, colors[series_names[j]], (x, y1), (x, y2), 1)
                pygame.draw.aaline(surface, colors[series_names[j]], (x - 10, y1), (x + 10, y1), 1)
                pygame.draw.aaline(surface, colors[series_names[j]], (x - 10, y2), (x + 10, y2), 1)
        

    # data lines
    for j in range(0, len(series)):
        if(series_names[j] in ignore_series):
            continue
        ser = series[j]
        points = []
        for i in range(0, len(ser)):
            x = distances[i] * float(chart_margins.width) + float(chart_margins.left)
            y = (1 - (ser[i] / maxValue)) * float(chart_margins.height) + float(chart_margins.top)
            points.append((x, y))
            
        pygame.draw.aalines(surface, colors[series_names[j]], False, points, 1)
        move_points(points, 1,0)
        pygame.draw.aalines(surface, colors[series_names[j]], False, points, 1)
        move_points(points, 0,1)
        pygame.draw.aalines(surface, colors[series_names[j]], False, points, 1)
        move_points(points, 0,-1)
        pygame.draw.aalines(surface, colors[series_names[j]], False, points, 1)
        move_points(points, -1,1)
        
        symbol = symbols[series_names[j]]
        for p in points:
            surface.blit(symbol, (p[0] - symbol.get_width() * 0.5, p[1] - symbol.get_height() * 0.5))
    
    # title
    if True:
        surface.fill((255, 255, 255), Rect(0,0, resolution[0], chart_margins.top - 10))
        text = title_font.render(name, True, (10,10,10))
        surface.blit(text, (resolution[0] * 0.5 - text.get_width() * 0.5, 5))

    # legend
    legend_row = 0
    for i in range(0, len(series_names)):
        if series_names[i] in ignore_series:
            continue
        x = chart_margins.right + 40.0
        y = chart_margins.centery - 15.0 * len(series_names) + 30.0 * legend_row
        legend_row += 1
        text = legend_font.render(series_names[i], True, (10,10,10))
        surface.blit(text, (x,y - text.get_height() * 0.5))
        pygame.draw.line(surface, colors[series_names[i]], (x - 25, y), (x - 5, y), 3)
        
        symbol = symbols[series_names[i]]
        surface.blit(symbol, (x - 15 - symbol.get_width() * 0.5, y - symbol.get_height() * 0.5))

    pygame.display.flip()

    try:
        image_counter += 1
    except:
        image_counter = 1

    pygame.image.save(surface, "{prefix}_-_{counter:02d}_-_{name}.png".format(counter = image_counter, name = name, prefix = name_prefix))
    
    #pause()

def move_points(points, x, y):
    for i in range(len(points)):
        points[i] = (points[i][0] + x, points[i][1] + y)

def parse_floats(series):
    for row in range(0, len(series)):
        for col in range(0, len(series[0])):
            series[row][col] = float(series[row][col].replace(',','.'))
            
def parse_ints(headers):
    for col in range(0, len(headers)):
        headers[col] = int(headers[col])

def make_point(image_name, color):
    surface = pygame.image.load('./points/' + image_name + '.png')
    surface.convert_alpha()

    c = pygame.Color(color[0], color[1], color[2])
    for x in range(surface.get_width()):
        for y in range(surface.get_height()):
            c.a = surface.get_at((x,y)).a
            surface.set_at((x,y), c)
    surface = pygame.transform.smoothscale(surface, (int(resolution[1]*0.015),int(resolution[1]*0.015)))
    return surface

def main():
    global resolution, chart_margins, legend_font, label_font, title_font, subtitle_font, colors, symbols, name_prefix, ignore_series, ignore_from_max, do_normalize, do_min_max
    
    resolution = (1440,1080)
    left_margin = 100
    top_margin = 100
    right_margin = 280
    bottom_margin = 100
    chart_margins = Rect(left_margin, top_margin, resolution[0] - left_margin - right_margin, resolution[1] - top_margin - bottom_margin )

    pygame.init()
    screen = pygame.display.set_mode(resolution)
    pygame.display.flip()
    
    legend_font = pygame.font.SysFont("arial", 30)
    label_font = pygame.font.SysFont("arial", 18)
    title_font = pygame.font.SysFont("arial", 26)
    subtitle_font = pygame.font.SysFont("arial", 18)

    colors = {}
    colors["Flat"] = (10,60,10)
    colors["Flat cached"] = (80,200,20)
    colors["Flat cold"] = (20,100,220)
    colors["Pooled Pointer"] = (180,20,10)
    colors["Naive Pointer"] = (250,120,120)
    colors["Pooled Multiway"] = (250,20,250)
    colors["Naive Multiway"] = (130,10,150)
    
    symbols = {}
    symbols["Flat"] = make_point("triangle", colors["Flat"])
    symbols["Flat cached"] = make_point("circle", colors["Flat cached"])
    symbols["Flat cold"] = make_point("square", colors["Flat cold"])
    symbols["Pooled Pointer"] = make_point("ellipse", colors["Pooled Pointer"])
    symbols["Naive Pointer"] = make_point("salmiakki", colors["Naive Pointer"])
    symbols["Pooled Multiway"] = make_point("flipped_triangle", colors["Pooled Multiway"])
    symbols["Naive Multiway"] = make_point("rectangle", colors["Naive Multiway"])

    lines = None

    input_file = None
    import os
    for file in os.listdir('.'):
        if file.endswith('.csv'):
            input_file = file
            break

    name_prefix = input_file[:-4]
    
    with open(input_file, 'r') as f:
        lines = f.readlines()

    table = []
    for line in lines:
        if(len(line) == 1):
            continue
        table.append(line.rstrip().split('\t'))
        print(len(table), table[-1])

    series_row_count = 7
    for i in range(2, len(table), 8):
        chart_name = table[i + 1][0]
        print(chart_name)

        data_series_headers = table[i + 0][2:]
        data_column_count = len(data_series_headers)

        #print(data_series_headers)
        
        data_series_names = []
        data_series = []
        min_data_series = []
        max_data_series = []
        
        for row in range(i + 1, i + 1 + series_row_count):
            # Get names from the first column
            data_series_names.append(table[row][1])
            
            # Get values from 2nd column onward
            data_series.append(table[row][2:2+data_column_count])

            # Get min values from columns after values
            min_data_series.append(table[row][2+data_column_count:2+2*data_column_count])
            
            # Get max values from columns after min values
            max_data_series.append(table[row][2+2*data_column_count:2+3*data_column_count])
        
        #print("DATAAA", data_series)
        #print("MIIIIN", min_data_series)
        #print("MAAAAX", max_data_series)
        #print("HEEAAD", data_series_headers)
        parse_floats(data_series)
        parse_floats(min_data_series)
        parse_floats(max_data_series)
        parse_ints(data_series_headers)

        ignore_series = []
        ignore_from_max = []
        do_normalize = False
        do_min_max = True
        
        if chart_name == "Nth Node":
            ignore_series = ["Flat", "Flat cold", "Flat cold"]
            
            make_chart(screen, chart_name + "", data_series_headers, data_series_names, data_series, min_data_series, max_data_series)

            do_normalize = True
            make_chart(screen, chart_name + " normalized", data_series_headers, data_series_names, data_series, min_data_series, max_data_series)
            
        elif "Find node" == chart_name:
            do_min_max = False
            ignore_series = ["Flat cached", "Flat cold"]
            make_chart(screen, chart_name, data_series_headers, data_series_names, data_series, min_data_series, max_data_series)
            
            do_min_max = True
            
            make_chart(screen, chart_name, data_series_headers, data_series_names, data_series, min_data_series, max_data_series)

            do_normalize = True
            make_chart(screen, chart_name + " normalized", data_series_headers, data_series_names, data_series, min_data_series, max_data_series)
            do_normalize = False
            
        elif "Transform" in chart_name:
            ignore_series = ["Flat cached", "Flat cold"]
            make_chart(screen, chart_name + "", data_series_headers, data_series_names, data_series, min_data_series, max_data_series)

            do_normalize = True
            make_chart(screen, chart_name + " normalized", data_series_headers, data_series_names, data_series, min_data_series, max_data_series)
            
        elif "Find max depth" in chart_name:
            ignore_series = ["Flat cached", "Flat cold"]
            make_chart(screen, chart_name + "", data_series_headers, data_series_names, data_series, min_data_series, max_data_series)

            do_normalize = True
            make_chart(screen, chart_name + " normalized", data_series_headers, data_series_names, data_series, min_data_series, max_data_series)
            do_normalize = False
            
            ignore_from_max = ["Naive Pointer", "Pooled Pointer", "Naive Multiway", "Pooled Multiway"]
            make_chart(screen, chart_name + " zoomed to flat", data_series_headers, data_series_names, data_series, min_data_series, max_data_series)
        else:
            do_min_max = False
            ignore_from_max = ["Flat cold"]
            if chart_name == "Leaf travel":
                ignore_from_max = ["Flat cold", "Flat"]
            
            make_chart(screen, chart_name, data_series_headers, data_series_names, data_series, min_data_series, max_data_series)
            ignore_series = ["Flat", "Flat cold", "Naive Multiway", "Naive Pointer"]

            do_min_max = True
            
            make_chart(screen, chart_name, data_series_headers, data_series_names, data_series, min_data_series, max_data_series)

            ignore_from_max = ["Flat", "Flat cached", "Pooled Multiway"]
            if chart_name == "Erase":
                ignore_from_max = ["Pooled Multiway"]
                
            if chart_name != "Leaf travel":
                make_chart(screen, chart_name + " zoomed", data_series_headers, data_series_names, data_series, min_data_series, max_data_series)
            
            do_normalize = True
            make_chart(screen, chart_name + " normalized", data_series_headers, data_series_names, data_series, min_data_series, max_data_series)
            do_normalize = False
            
            ignore_from_max = []
            ignore_series = ["Flat", "Flat cold", "Naive Multiway", "Pooled Multiway"]
            
            make_chart(screen, chart_name + ", for pointer trees", data_series_headers, data_series_names, data_series, min_data_series, max_data_series)

            ignore_series = ["Flat", "Flat cold", "Naive Pointer", "Pooled Pointer"]
            
            make_chart(screen, chart_name + ", for multiway trees", data_series_headers, data_series_names, data_series, min_data_series, max_data_series)

            if chart_name == "Find count":
                continue
            
            ignore_series = ["Naive Pointer", "Pooled Pointer", "Naive Multiway", "Pooled Multiway"]
            make_chart(screen, chart_name + ", for flat trees", data_series_headers, data_series_names, data_series, min_data_series, max_data_series)
            
            if chart_name == "Erase":
                continue
            
            ignore_from_max = ["Flat cold"]
            make_chart(screen, chart_name + ", for flat trees", data_series_headers, data_series_names, data_series, min_data_series, max_data_series)
            


try:
    main()
finally:
    pygame.quit()




