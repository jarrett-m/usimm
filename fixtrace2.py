with open("input/domains/perl/core_0") as f:
    #read on 25,000 lines
    #lines = [line.strip() + " " + str(0) + "\n" for line in f.readlines() ]
    lines = [line.strip() + " " + str(0) + "\n" for line in f.readlines()[:10000000] ]

with open("input/domains/perl/core_0-2", "w") as f:
    f.writelines(lines)

with open("input/domains/perl/core_1") as f:
    #read on 25,000 lines
    #lines = [line.strip() + " " + str(0) + "\n" for line in f.readlines() ]
    lines = [line.strip() + " " + str(1) + "\n" for line in f.readlines()[:10000000] ]

with open("input/domains/perl/core_1-2", "w") as f:
    f.writelines(lines)

with open("input/domains/perl/core_2") as f:
    #read on 25,000 lines
    #lines = [line.strip() + " " + str(0) + "\n" for line in f.readlines() ]
    lines = [line.strip() + " " + str(2) + "\n" for line in f.readlines()[:10000000] ]

with open("input/domains/perl/core_2-2", "w") as f:
    f.writelines(lines)

with open("input/domains/perl/core_3") as f:
    #read on 25,000 lines
    #lines = [line.strip() + " " + str(0) + "\n" for line in f.readlines() ]
    lines = [line.strip() + " " + str(3) + "\n" for line in f.readlines()[:10000000] ]

with open("input/domains/perl/core_3-2", "w") as f:
    f.writelines(lines)