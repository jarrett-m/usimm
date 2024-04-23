from random import randint
with open("input/nondomains/fluid") as f:
    #append a rand int from 0-7 to the end of each line with space berfire it
    lines = [line.strip() + " " + str(randint(0, 7)) + "\n" for line in f.readlines()]
with open("input/domains/fluid", "w") as f:
    f.writelines(lines)