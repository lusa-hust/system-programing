# How to compile
To compile:
```bash
make -C /usr/src/linux-headers-(uname -r) M=(pwd) modules
```
To clean:
```bash
make -C /usr/src/linux-headers-(uname -r) M=(pwd) clean
```
Note: Above commands is writen in fish style, if you use built in bash, you should change (uname -r) to \`uname -r\` and (pwd) to \`pwd\`