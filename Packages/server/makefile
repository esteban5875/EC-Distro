.PHONY: all run clean


all:
	go build -o out/main main.go
run: all
	./out/main
clean:
	rm -rf ./out
	cd ../repo && make clean
