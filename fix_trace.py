from random import randint
with open("input/nondomains/mcf") as f:
    #append a rand int from 0-7 to the end of each line with space berfire it
    lines = [line.strip() + " " + str(0) + "\n" for line in f.readlines()]
with open("input/domains/mcf-2", "w") as f:
    f.writelines(lines)