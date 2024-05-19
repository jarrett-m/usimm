with open("input/domains/perl/core_0", 'r') as f:
    #Tell me how many 'W's and 'R's are in the file
    w = 0
    r = 0
    count = 0
    for line in f:
        w += line.count('W')
        r += line.count('R')

    #Print the ratio of 'W's to 'R's
    print(w)
    print(r)
    print(w/(r+w))