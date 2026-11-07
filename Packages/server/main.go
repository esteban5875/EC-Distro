package main

import (
	"fmt"
	"log"
	"os"
	"os/exec"
	"path/filepath"
	"server/core"
)

const COMPILE_ERR = -1
const COMPILE_SUCCESS = 0

var REPO_DIR = filepath.Join("..", "..", "Packages", "repo")
var BIN_DIR  = filepath.Join(REPO_DIR, "bin")
var TAR_DIR  = filepath.Join(REPO_DIR, "tarball")

func compile() int {
	var cmd *exec.Cmd;

	cmd = exec.Command("make")
	cmd.Dir = REPO_DIR
	
	output, err := cmd.CombinedOutput()
	
	if err != nil {
		log.Fatalf("Failure compiling: %s", err)
		return COMPILE_ERR
	}

	fmt.Printf("Compile: \n%s", string(output))
	return COMPILE_SUCCESS
}

func getExecutables() ([]string, error) {
    entries, err := os.ReadDir(BIN_DIR)
    if err != nil {
        return nil, err
    }

    var executables []string

    for _, entry := range entries {
        if !entry.IsDir() {
            continue
        }

        program := entry.Name()
        executable := filepath.Join(BIN_DIR, program, program)

        info, err := os.Stat(executable)
        if err != nil || info.IsDir() {
            continue
        }

        executables = append(executables, executable)
    }

    return executables, nil
}

func main ()  {
	var compilation = compile()

	if compilation == COMPILE_ERR {
		return;
	}

	var executables, err = getExecutables();

	if err != nil {
		return;
	}

	for i := 0; i < len(executables); i++ {
		create := core.CreateTarball(TAR_DIR, executables[i]);

		if create != nil {
			return;
		}
	}

	core.InitServer(TAR_DIR)
}