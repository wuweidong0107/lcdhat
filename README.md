# build

```
$ make
$ make install
```

# usage
```
$ lcdhat
usage: ./lcdhat inputdev fbdev font
```

## param:
- inputdev: key device node
- fbdev: lcd device node 
- font: font file

## example
```Bash
# for NanoPi NEO3 with LCD Hat
$ /usr/local/bin/lcdhat /dev/input/event1 /dev/fb0 ./font/SourceHanSansCN-Regular.ot ./font/SourceHanSansCN-Regular.otf
```