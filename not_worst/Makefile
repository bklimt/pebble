
all: deploy

./build:
	./waf configure

./build/not_worst.pbw: ./src/not_worst.c ./build
	./waf build

deploy: ./build/not_worst.pbw
	../libpebble/p.py --lightblue --pebble_id 00:17:EC:17:B4:80 reinstall ./build/not_worst.pbw

clean:
	rm -r build/

