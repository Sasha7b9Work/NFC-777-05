import os

directory = '.'

def ProcessLine(line):
    result = ""
    for c in line:
        if c == '\t':
            result = result + "    "
        else:
            result = result + c
    return result

def ProcessFile(name_file):
    lines = []
    with open(name_file, "r", encoding="utf-8") as file:
        for line in file:
            lines.append(line)
        file.close()
        with open(name_file, "w", encoding="utf-8") as file:
            not_verify = False
            for line in lines:
                line = ProcessLine(line)
                if not_verify:
                    file.write(line)
                else:
                    if line[0] == '/':
                        if line[1] == '/':
                            pass
                    else:
                        if line[0] == '\n':
                            pass
                        else:
                            file.write(line)
                            not_verify = True

for root, dirs, files in os.walk(directory):
    for file in files:
        if file.endswith('.h'):
            print(os.path.join(root, file))
            ProcessFile(os.path.join(root, file))
        if file.endswith('.cpp'):
            print(os.path.join(root, file))
            ProcessFile(os.path.join(root, file))

