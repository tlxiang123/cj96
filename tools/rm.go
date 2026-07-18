package main

import (
	"os"
	"strings"
)

func main() {
	for _, path := range os.Args[1:] {
		if strings.HasPrefix(path, "-") {
			continue
		}
		_ = os.RemoveAll(path)
	}
}
