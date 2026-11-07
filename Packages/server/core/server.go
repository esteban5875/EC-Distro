package core;

import (
	"log"
	"net/http"
)

func InitServer(DIR string) {
	fs := http.FileServer(http.Dir(DIR))

    http.Handle("/", fs)

    log.Println("Serving on http://localhost:8080")
    log.Fatal(http.ListenAndServe(":8080", nil))
}