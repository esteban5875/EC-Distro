package core

import (
	"fmt"
    "os/exec"
    "os"
    "path/filepath"
)

const COMPILE_ERR = -1
const COMPILE_SUCCESS = 0

var REPO_DIR = filepath.Join("..", "repo")
var BIN_DIR  = filepath.Join(REPO_DIR, "bin")
var TAR_DIR  = filepath.Join(REPO_DIR, "tarball")

func Compile() int {    fmt.Println("[DEBUG] Starting make...")
    cmd := exec.Command("make")
    cmd.Dir = REPO_DIR

    output, err := cmd.CombinedOutput()

    fmt.Printf("[DEBUG] Exit error: %v\n", err)
    fmt.Printf("[DEBUG] Exit code: %d\n", cmd.ProcessState.ExitCode())
    fmt.Printf("[DEBUG] Output:\n%s\n", string(output))

    if err != nil {
        return COMPILE_ERR
    }

    return COMPILE_SUCCESS
}


func GetAll(dir string, isTarOpts ...bool) ([]string, error) {
	isTar := false

	if len(isTarOpts) > 0 {
		isTar = isTarOpts[0]
	}

    entries, err := os.ReadDir(dir)
    if err != nil {
        return nil, err
    }

    var executables []string

    for _, entry := range entries {
        if !entry.IsDir() {
            continue
        }

        program := entry.Name()
		var executable string

		if !isTar {
			executable = filepath.Join(dir, program, program)
		} else {
			executable = filepath.Join(dir, fmt.Sprintf("%s.tar.gz", program))
		}

        info, err := os.Stat(executable)
        if err != nil || info.IsDir() {
            continue
        }

        executables = append(executables, executable)
    }

    return executables, nil
}