# clear no-used item from the output of nm tool

fo = open("temp_clear", "w")

with open("temp") as f:
    line = f.readline()
    line_array = line.split()
    while line:
        if ("metal" in line_array[3]) or (line_array[3].startswith("_")) or ("IRQ" in line) or line_array[2] == "r" or line_array[2] == "b" or line_array[2] == "B" or line_array[2] == "d" or line_array[2] == "W" or "callback" in line_array[3] or "exit" in line_array[3] or "trap" in line_array[3] or "free" in line_array[3] or "main" in line_array[3] or "memc" in line_array[3] or "mems" in line_array[3] or "malloc" in line_array[3] or "use_hfxosc" in line_array[3] or "std" in line_array[3]:
            line = f.readline()
            line_array = line.split()
        else:
            fo.write(line)
            line = f.readline()
            line_array = line.split()
fo.close()
