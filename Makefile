INCLUDE=-Isrc/ -Isrc/KINDLE -I.

CFLAGS=$(INCLUDE) -D_GNU_SOURCE -DHAVE_CONFIG_H -DDISABLE_DOUBLEBUFFER

C_FILES=$(wildcard src/*.c) $(wildcard src/KINDLE/*.c)
OBJ_FILES=$(patsubst %.c,%.o,$(C_FILES))

all: prboom

src/%.o: src/%.c
	$(CC) -c $(CFLAGS) $< -o $@

prboom: $(OBJ_FILES) Makefile
	$(CC) -static $(CFLAGS) -o prboom $(OBJ_FILES)

extension: prboom
	mv $< extensions/prboom/$<
	zip -r PrBoom_KUAL.zip extensions 

clean:
	rm -rf $(OBJ_FILES)
	rm -f PrBoom_KUAL.zip